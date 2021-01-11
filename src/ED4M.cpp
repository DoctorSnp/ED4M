/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#include <stdio.h>
#include <math.h>

#include "utils/utils.h"
#include "equipment/epk.h"
#include "appliances/saut.h"
#include "appliances/klub.h"
#include "src/equipment/brake_395.h"

#include "ED4M.h"

constexpr float  MIN_VOLTAGE = 2500.0;
//constexpr float  TM_MINIMUM_REQUIRED = 5.2;
//constexpr float  UR_MINIMUM_REQUIRED = 8.0;
constexpr float  MIN_FOR_SOT = 0.2;

/*******************************    Закрытые функции    *******************************/
static void HodovayaSound(st_Self *SELF);
static void m_loadElementStates(st_Self *SELF);


/**** Электрика ****/
static void  m_electric(st_Self *SELF );
static float m_tractionForce ( st_Self *SELF);
static int   m_haveElectroEmergency(st_Self *SELF);
static void  m_checkBV(st_Self *SELF);
static void  m_osveshenie(st_Self *SELF);

/**** Пневматика ****/
static void m_pneumatic(st_Self *SELF);
static void m_compressorMain(st_Self *SELF);
static void m_compressorAux(st_Self *SELF);
/*******************/

static void m_checkManagement(st_Self * SELF);
static void m_SetDoorsState(st_Self *SELF, int WhatDoors, int NewState);
static void m_displayPult(st_Self *SELF);

/* вспомогательные функции */

static void m_debugStep(st_Self *self); // отладочная печать раз в секунду
/**************************************************************************************/

static SAUT saut;
static Brake_395 kran395[2];
static EPK epk;

int ED4M_init( st_Self *SELF, Locomotive *loco, Engine *eng)
{
    memset(SELF, 0, sizeof (struct st_Self));

    SELF->game.cabPtr = loco->cab;
    SELF->game.engPtr = (ElectricEngine*)eng;
    SELF->game.locoPtr = (const ElectricLocomotive*)loco;
    SELF->cabNum = -1;

    /*считываем параметры каждой кабины*/
    for (int cab =1; cab<= CABS_COUNT; cab++)
    {
        for (int i =0; i< TUMBLERS_MAX_ID; i++)
            SELF->tumblers[cab][i] = _checkSwitch(loco, i, -1, 1, 0);
        for (int i =0; i< ARMS_MAX_ID; i++)
            SELF->arms[cab][i] = _checkSwitch(loco, i, -1, 1, 0);

        Radiostation_Init(&SELF->radio[cab]);
        KLUB_init(&SELF->KLUB[cab]);

        SELF->elecrto[cab].PantoRaised = 0;
        SELF->arms[cab][Arms::Arm_395] = _checkSwitch(loco, Arms::Arm_395, -1, 1, 0 ) + 1;
        SELF->arms[cab][Arms::Arm_Reverse]  = REVERSE_NEUTRAL;
    }

    //SELF->cabNum = 0;// ED4M_checkCab(SELF);

    loco->HandbrakeValue=0.0;
    eng->HandbrakePercent=0.0;
    eng->DynamicBrakePercent=0;
    eng->MainResRate = 0.0;
    eng->Sanding = 0;
    eng->AuxilaryRate = 0.0;
    eng->ALSNOn  = 0;

    eng->EPTvalue = 0.0;
    eng->Reverse = 0;
    eng->ThrottlePosition = 0;

    SELF->currTime = {};
    SELF->prevTime = {};
    SELF->doors[DOORS_LEFT - 1] =  _checkSwitch(loco, Tumblers::Tmb_leftDoors, -1, 1, 0);
    SELF->doors[DOORS_RIGHT - 1] =  _checkSwitch(loco, Tumblers::Tmb_rightDoors, -1, 1, 0);

    /* Тест тормозов */
    loco->MainResPressure = 0.0;
    loco->TrainPipePressure = 0.0;
    loco->AuxiliaryPressure = 0.0;

    epk.init();
    saut.init();
    kran395[0].init(eng, 1);
    kran395[1].init(eng, 1);

    return 0;
}

void ED4M_ALSN(st_Self *SELF, const Locomotive *loco)
{
    if (SELF->cabNum < 1)
        return;
    if (SELF->game.cabPtr == nullptr)
        return;

    static int SignCodeChecked = 0;
    static int decodeSeconds = 0;

    int code = Svetofor_code(&SELF->alsn[SELF->cabNum]);
    if ( code != SignCodeChecked )
    {
        if (decodeSeconds == 0) // обнаружена разность сигналов. Имитируем декодирование
            decodeSeconds = SELF->game.currTime.seconds;

        if (decodeSeconds + 4 < SELF->game.currTime.seconds)
        {
            SignCodeChecked = code;
            decodeSeconds = 0;
        }
    }


    SELF->alsn[SELF->cabNum].correctALSNCode = SignCodeChecked;
    SELF->alsn[SELF->cabNum].PrevSpeed = SELF->prevVelocity;

    static int prevEpk = SELF->tumblers[SELF->cabNum][Tumblers::Key_EPK];
    if ( prevEpk != SELF->tumblers[SELF->cabNum][Tumblers::Key_EPK])
    {
        if (prevEpk == 0)
            saut.start(loco, loco->Eng(), &SELF->alsn[SELF->cabNum]);
        else
            saut.stop(loco, loco->Eng());
        prevEpk = SELF->tumblers[SELF->cabNum][Tumblers::Key_EPK];
    }
    else
    {
       if (SELF->buttons[SELF->cabNum][Buttons::Btn_RB_POM] || SELF->buttons[SELF->cabNum][Buttons::Btn_RB_D] ||
               SELF->buttons[SELF->cabNum][Buttons::Btn_RB_MASH])
            epk.okey(loco);
        int sautState = saut.step(loco, loco->Eng(), &SELF->alsn[SELF->cabNum]);
        int currEpkState = epk.step(loco, sautState, SELF->game.currTime );
        if ( currEpkState )
        {
            //if (!SELF->flags.EPK_Triggered)
            //    _setBrakePosition(7, loco, loco->Eng(), SELF);
            //SELF->flags.EPK_Triggered = 1;
        }
    }

    SELF->KLUB[SELF->cabNum].cabPtr = SELF->game.cabPtr;
    if ( (SELF->tumblers[SELF->cabNum][Tumblers::Switch_PitALSN_1]) &&
         (SELF->tumblers[SELF->cabNum][Tumblers::Switch_PitALSN_2]) )
    {
        loco->Eng()->ALSNOn = 1;
        if (SELF->tumblers[SELF->cabNum][Tumblers::Key_EPK] )
            SELF->KLUB[SELF->cabNum].isOn = 2;
        else
            SELF->KLUB[SELF->cabNum].isOn = 1;
    }
    else
         SELF->KLUB[SELF->cabNum].isOn = 0;


    SELF->KLUB[SELF->cabNum].currTime = SELF->game.currTime;
    KLUB_Step(&SELF->KLUB[SELF->cabNum], loco->Eng(), SELF->alsn[SELF->cabNum], loco);

}

int ED4M_step(st_Self *SELF )
{
    int isMainSect = 0;
    ElectricEngine *eng = SELF->game.engPtr;
    const ElectricLocomotive *loco = SELF->game.locoPtr;
    SELF->currTime = SELF->game.currTime;

    if(loco->NumSlaves && (loco->LocoFlags&1) )
    {
        isMainSect = 1;
        SELF->cabNum = ED4M_checkCab(SELF);
        if ( SELF->cabNum > CABS_COUNT )
            SELF->cabNum = CABS_COUNT;
    }

    if (!isMainSect)
    {
        eng->Panto = ((unsigned char)SELF->elecrto[SELF->cabNum].PantoRaised);
        eng->ThrottlePosition = SELF->elecrto[SELF->cabNum].TyagaPosition;
        return 1;
    }

    if ((SELF->elecrto[SELF->cabNum].TyagaPosition == 0) &&  (SELF->elecrto[SELF->cabNum].RecuperationPosition == 0))
        SELF->elecrto[SELF->cabNum].lkitTime = SELF->game.currTime.seconds;

    SELF->game.cabPtr = loco->cab;
    if (SELF->cabNum < 1)
        return 1;

    /*Работа систем поезда */
    int brake = 0x0;
    setBitState((char*)&brake, 0, SELF->arms[SELF->cabNum][Arms::Arm_395_TM_Control]);

    // это пока инвертируем, пока неизвестно почему
    setBitState((char*)&brake, 1, !SELF->arms[SELF->cabNum][Arms::Arm_395_NM_Control]);
    kran395[SELF->cabNum-1].step(SELF->arms[SELF->cabNum][Arms::Arm_395], loco, eng, SELF->game.time, brake);

    m_pneumatic(SELF);
    m_electric(SELF);
    m_checkManagement(SELF);
    m_displayPult(SELF);
    m_osveshenie(SELF);

    SELF->radio[SELF->cabNum].isActive = SELF->tumblers[SELF->cabNum][Tumblers::Switch_Radio];
    Radiostation_Step(loco, eng, &SELF->radio[SELF->cabNum]);
    HodovayaSound(SELF);

    /****************************************************************************/

    /*А тепер пихаем из наших МОЗГОВ данные в движок*/
    eng->Reverse = SELF->arms[SELF->cabNum][Arms::Arm_Reverse] - REVERSE_NEUTRAL;
    if (SELF->cabNum != 1) // инвертируем реверс для задней кабины
        eng->Reverse = -eng->Reverse;

    SELF->prevVelocity = SELF->alsn[SELF->cabNum].CurrSpeed;
    float SetForce = m_tractionForce (SELF);

    if(SELF->alsn[SELF->cabNum].CurrSpeed <= 3.01)
        SetForce *= 30000.0;
    else
        SetForce *= 54000.0;

    if (!eng->Reverse)
        SetForce = 0.0;
    for (UINT i =0; i < loco->NumSlaves; i++)
        loco->Slaves[i]->Eng()->Force = (-SetForce); //Reverse* (eng->Reverse);

    m_debugStep(SELF);
    return 1;
}

/**
 * @brief m_currentCabNum Получает тек. номер кабины в составе
 * @param SELF
 * @return Возвращает положительный номер кабины или -1 в случае ошибки.
 */
int ED4M_checkCab(st_Self *SELF)
{
    if (!SELF->game.locoPtr)
    {
        swprintf(SELF->errorText, L"Не загружен объект локомотива из движка игры\n");
        if (SELF->game.engPtr)
            SELF->game.engPtr->ShowMessage(GMM_POST, SELF->errorText);
        return -1;
    }

    if (SELF->cabNum < 0) // первая инициализация
    {
        int cabNum = (int)SELF->game.locoPtr->GetParameter( (wchar_t*)(L"CabNum"), 0.0);
        if (cabNum < 1)
            goto DefaultCabNum;
        SELF->cabNum = cabNum;

    }

    else // возможно была смена кабины
    {
        int cabNum =  (int)SELF->game.locoPtr->GetParameter( (wchar_t*)(L"CabNum"), 0.0);
        if (cabNum < 1)
            goto DefaultCabNum;

        if (SELF->cabNum != cabNum)
        {
            swprintf(SELF->errorText, L"Changed Cabin from %d to %d\n", SELF->cabNum, cabNum);
            if (SELF->game.engPtr)
                SELF->game.engPtr->ShowMessage(GMM_POST, SELF->errorText);

            m_loadElementStates(SELF);
        }
        SELF->cabNum = cabNum;
    }
    return SELF->cabNum;
    DefaultCabNum:
        SELF->cabNum = 1;
        swprintf(SELF->errorText, L"В .eng файле не задан номер кабины. Считается что это кабина %d\n", SELF->cabNum);
        SELF->game.engPtr->ShowMessage(GMM_POST, SELF->errorText);
        return SELF->cabNum;
}


static void m_SetDoorsState(struct st_Self *SELF, int WhatDoors , int NewState)
{
    auto setDoorState =
        [](st_Self *SELF, int doorSide, int newState)
        {
            if (SELF->doors[doorSide-1] == newState ) // состояние не изменилось
                return ;
            SELF->doors[doorSide-1] = newState;
            if (SELF->doors[doorSide-1] == DOORS_CLOSED)
            {
                SELF->game.locoPtr->PostTriggerCab(SoundsID::DoorsClose );
                if (!SELF->game.locoPtr->Eng()->PostGlobalMessage(GM_DOOR_CLOSE, doorSide) )
                    Printer_print(SELF->game.engPtr, GMM_POST, L"Error of closing doors %d", doorSide);
                SELF->game.locoPtr->Eng()->PostGlobalMessage(GM_DOOR_LOCK, doorSide);
            }
            else
            {
                SELF->game.locoPtr->PostTriggerCab(SoundsID::DoorsOpen );
                SELF->game.locoPtr->Eng()->PostGlobalMessage(GM_DOOR_UNLOCK, doorSide);
                if( !SELF->game.locoPtr->Eng()->PostGlobalMessage(GM_DOOR_OPEN, doorSide) )
                    Printer_print(SELF->game.engPtr, GMM_POST, L"Error of opening doors %d", doorSide);
            }
        };

    if ( !SELF->tumblers[SELF->cabNum][Tumblers::Switch_Pitanie_Dverey])
        return;
    int doorsSide = WhatDoors;
    if( SELF->cabNum == 1) // нужно инвертировать двери
    {
        if (doorsSide == DOORS_RIGHT)
            doorsSide = DOORS_LEFT;
        else if (doorsSide == DOORS_LEFT)
            doorsSide = DOORS_RIGHT;
    }

    /* проверяем левые двери*/
    if ( WhatDoors == DOORS_LEFT)
    {
        if (SELF->tumblers[SELF->cabNum][Tumblers::Tmb_manage_leftDoors_TCH_M])
            setDoorState(SELF, doorsSide, NewState);
    }
    else if ( WhatDoors == DOORS_RIGHT)
    {
        if (SELF->tumblers[SELF->cabNum][Tumblers::Tmb_manage_RightDoors_TCH_M])
            setDoorState(SELF, doorsSide, NewState);
    }

}

/**
 * @brief HodovayaSound Озвучка ходовой части
 * @param self
 */
static void HodovayaSound(st_Self *SELF)
{
    static int isFinalStop = 0;
    float currSpeed = fabs(SELF->alsn[SELF->cabNum].CurrSpeed);
    float prevSpeed = fabs(SELF->prevVelocity);

    if ( (prevSpeed > currSpeed) && (currSpeed > 0.3)  ) // типа, быстро так тормозим
    {
        if ((SELF->alsn[SELF->cabNum].CurrSpeed < 3.0) && (isFinalStop == 0)  )
        {
            isFinalStop = 1;
            return;
        }
    }

    if (fabs(SELF->alsn[SELF->cabNum].CurrSpeed) <= 0.00)
    {
        if (isFinalStop )
            isFinalStop = 0;
    }
}

static void m_loadElementStates(st_Self *SELF)
{
    for (int i = 0; i<SWITCHES_CNT; i++ )
    {
        SELF->arms[SELF->cabNum][i] = _checkSwitch( SELF->game.locoPtr, i, -1, 1, 0);
        SELF->tumblers[SELF->cabNum][i] = _checkSwitch( SELF->game.locoPtr, i, -1, 1, 0);
        SELF->buttons[SELF->cabNum][i] = _checkSwitch( SELF->game.locoPtr, i, -1, 1, 0);
    }

}

static float m_tractionForce (st_Self *SELF)
{
    float startForce = 0.0;

    if (SELF->elecrto[SELF->cabNum].PantoRaised == 0)
        return 0.0;

    if (SELF->game.engPtr->MainSwitch == 0)
        return 0.0;

    SELF->game.engPtr->Reverse = SELF->arms[SELF->cabNum][Arms::Arm_Reverse] - REVERSE_NEUTRAL;
    startForce = (float) (( SELF->game.engPtr->Reverse) * SELF->elecrto[SELF->cabNum].TyagaPosition )  ;
    return startForce;
}

static void m_checkManagement(st_Self * SELF)
{
    m_SetDoorsState(SELF, DOORS_LEFT , SELF->tumblers[SELF->cabNum][Tumblers::Tmb_leftDoors]);
    m_SetDoorsState(SELF, DOORS_RIGHT, SELF->tumblers[SELF->cabNum][Tumblers::Tmb_rightDoors]);
}

static void m_osveshenie(st_Self *SELF)
{
    const ElectricLocomotive *loco = SELF->game.locoPtr;
    int canLight = SELF->tumblers[SELF->cabNum][Switch_Projector_I_signaly];
    int projState = SELF->tumblers[SELF->cabNum][Tumblers::MultiTumbler_Projector];
    Cabin *cab = SELF->game.cabPtr;

    if (projState && canLight )
    {
       loco->SwitchLight(en_ExteriorLights::Proj_Half1, 1, 0.0, 0);
       loco->SwitchLight(en_ExteriorLights::Light_Proj_Half1, 1, 0.0, 0);
        if ( (projState == 2) && SELF->game.isNight)
        {
            loco->SwitchLight(en_ExteriorLights::Proj_Full1, 1, 0.0, 0);
            loco->SwitchLight(en_ExteriorLights::Light_Proj_Full1, 1, 0.0, 0);
        }
    }
    else
    {
        loco->SwitchLight(en_ExteriorLights::Light_Proj_Half1, 0, 0.0, 0);
        loco->SwitchLight(en_ExteriorLights::Light_Proj_Full1, 0, 0.0, 0);
    }

    int svetPribor = canLight && SELF->tumblers[SELF->cabNum][Tumblers::Switch_SvetPult];
    cab->SwitchLight(en_InteriorLights::Svet_Pribor1, svetPribor );
    cab->SwitchLight(en_InteriorLights::Svet_Pribor2, svetPribor );

    int canBuffer =  SELF->tumblers[SELF->cabNum][Tumblers::Switch_BufferFonar_I_Dvorniki];
    loco->SwitchLight(en_ExteriorLights::Light_BufferRight,
                      canBuffer && SELF->tumblers[SELF->cabNum][Tumblers::Switch_BufRight], 0.0, 0);
    loco->SwitchLight(en_ExteriorLights::Light_BufferLeft,
                      canBuffer && SELF->tumblers[SELF->cabNum][Tumblers::Switch_BufLeft], 0.0, 0);

    int redSignals = SELF->tumblers[SELF->cabNum][Tumblers::Switch_SigUp] &&
            SELF->tumblers[SELF->cabNum][Tumblers::Switch_Projector_I_signaly];

    loco->SwitchLight(en_ExteriorLights::Light_SigUp1, redSignals, 0.0, 0);
    loco->SwitchLight(en_ExteriorLights::Light_SigUp2, redSignals, 0.0, 0);

    redSignals = SELF->tumblers[SELF->cabNum][Tumblers::Switch_SigDown] &&
                     SELF->tumblers[SELF->cabNum][Tumblers::Switch_Projector_I_signaly];

    loco->SwitchLight(en_ExteriorLights::Light_SigDownLeft , redSignals, 0.0, 0);

}


static void m_electric(st_Self *SELF )
{
    const ElectricLocomotive *loco = SELF->game.locoPtr;
    SELF->elecrto[SELF->cabNum].LineVoltage =  loco->LineVoltage;

    m_checkBV(SELF);

    int canManage = SELF->tumblers[SELF->cabNum][Tumblers::Switch_VU] &&
            SELF->tumblers[SELF->cabNum][Tumblers::Switch_AutomatUpr];

    if (SELF->tumblers[SELF->cabNum][Tumblers::Switch_Panto] &&
            SELF->tumblers[SELF->cabNum][Tumblers::Tmb_PantoUP] )
    {
        if (canManage)
        {
            if (!SELF->elecrto[SELF->cabNum].PantoRaised)
            {
                SELF->elecrto[SELF->cabNum].PantoRaised = 0xff;
                loco->PostTriggerCab(SoundsID::TP_UP);
            }
        }
    }
    else if (SELF->tumblers[SELF->cabNum][Tumblers::Tmb_PantoDown] ||
             !SELF->tumblers[SELF->cabNum][Tumblers::Switch_Panto] )
    {
        if (SELF->elecrto[SELF->cabNum].PantoRaised)
        {
            SELF->elecrto[SELF->cabNum].PantoRaised = 0x0;
            loco->PostTriggerCab(SoundsID::TP_DOWN);
            SELF->game.engPtr->MainSwitch = 0;
            SELF->BV_STATE[SELF->cabNum] = 0;
        }
    }
}

/**
 * @brief m_checkBV Проверка БВ
 */
static void m_checkBV(st_Self *SELF)
{
    if (m_haveElectroEmergency(SELF) )
    {
        if (SELF->game.engPtr->MainSwitch)
        {
            SELF->game.engPtr->MainSwitch = 0;
            SELF->BV_STATE[SELF->cabNum] = 0;
        }
    }

    if (SELF->tumblers[SELF->cabNum][Tumblers::Tmb_vozvrZash])
    {
        if (SELF->elecrto[SELF->cabNum].PantoRaised)
        {
            if (SELF->BV_STATE[SELF->cabNum] < 1)
                SELF->BV_STATE[SELF->cabNum] = 1;
        }
    }
    else
    {
        if (!SELF->tumblers[SELF->cabNum][Tumblers::Tmb_vozvrZash])
            if ( SELF->BV_STATE[SELF->cabNum] == 1 )
                SELF->game.engPtr->MainSwitch = 1;
    }
}

/**
 * @brief m_haveElectroEmergency Если какие-то неисправности в электрике
 * @return
 */
static int m_haveElectroEmergency(st_Self *SELF)
{
    return 0;
}

static void m_pneumatic(st_Self *SELF)
{
    m_compressorMain(SELF);
    m_compressorAux(SELF);
}

static void m_compressorMain(st_Self *SELF)
{
    const ElectricLocomotive *loco = (SELF->game.locoPtr);
    ElectricEngine *eng = (SELF->game.engPtr);

    int isEnabled = (SELF->tumblers[SELF->cabNum][Tumblers::Switch_VspomCompr] &&
                     SELF->tumblers[SELF->cabNum][Tumblers::Tmb_vspomCompr] &&
                     SELF->tumblers[SELF->cabNum][Tumblers::Tmb_Tormozhenie]);

    int isCanWork = isEnabled && eng->MainSwitch && ( loco->LineVoltage >= MIN_VOLTAGE);


    if ( (eng->UR < SELF->game.locoPtr->MainResPressure ) &&  isCanWork )
    {   // включаем компрессов
        if (SELF->pneumo[SELF->cabNum].CompressorMainWork == 0)
        {
            loco->PostTriggerCab(SoundsID::MainCompressorWork );
            SELF->pneumo[SELF->cabNum].CompressorMainWork = 1;
        }
    }
    else
    {
        if (SELF->pneumo[SELF->cabNum].CompressorMainWork)
        {
            // выключаем работающий компрессор
            loco->PostTriggerCab(SoundsID::MainCompressorWork + 1);
            loco->PostTriggerCab(SoundsID::MainCompressorStop);
            SELF->pneumo[SELF->cabNum].CompressorMainWork = 0;
        }
    }
}

static void m_compressorAux(st_Self *SELF)
{

    const ElectricLocomotive *loco = (SELF->game.locoPtr);
    ElectricEngine *eng = (SELF->game.engPtr);

    int enabledControl = SELF->tumblers[SELF->cabNum][Tumblers::Switch_VspomCompr];
    int compressorCanWork = SELF->tumblers[SELF->cabNum][Tumblers::Tmb_vspomCompr] &&
            enabledControl && ( eng->BatteryCharge );

}

/**
 * @brief _debugStep
 */
static void m_debugStep(st_Self *SELF)
{
    SELF->currTime = SELF->game.currTime;
    if ((SELF->prevTime.seconds + 1) > SELF->currTime.seconds)
        return;
    //Printer_print(SELF->game.engPtr, GMM_POST, L"MainRes %f ChargingPipe: %f UR: %f",
   //     SELF->game.locoPtr->MainResPressure, SELF->game.locoPtr->ChargingPipePressure, SELF->game.engPtr->UR );
    SELF->prevTime.seconds = SELF->currTime.seconds;
}

/**
 * @brief _displayPult Показывает манометры и лампы на пульте
 */
static void m_displayPult(st_Self *SELF)
{
    const ElectricLocomotive *loco = SELF->game.locoPtr;
    ElectricEngine *eng = SELF->game.engPtr;
    Cabin *cab = loco->cab;

    cab->SetDisplayValue(Sensors::Sns_Voltage, loco->LineVoltage );
    cab->SetDisplayValue(Sensors::Sns_BrakeCil, loco->BrakeCylinderPressure );
    cab->SetDisplayValue(Sensors::Sns_SurgeTank, eng->UR);
    cab->SetDisplayValue(Sensors::Sns_BrakeLine, loco->TrainPipePressure);
    cab->SetDisplayValue(Sensors::Sns_PressureLine, loco->ChargingPipePressure);

    int canLight = SELF->tumblers[SELF->cabNum][Tumblers::Switch_VU] &&
            SELF->tumblers[SELF->cabNum][Tumblers::Switch_AutomatUpr];
    cab->SetDisplayState(Lamps::Lmp_SOT, canLight && (loco->BrakeCylinderPressure >= MIN_FOR_SOT) );
    cab->SetDisplayState(Lamps::Lmp_SOT_X, canLight && (loco->BrakeCylinderPressure >= MIN_FOR_SOT) );

    int lampEPT = SELF->tumblers[SELF->cabNum][Tumblers::Switch_EPT] && canLight;
    cab->SetDisplayState(Lamps::Lmp_K, lampEPT && (SELF->elecrto[SELF->cabNum].RecuperationPosition == 0) );
    cab->SetDisplayState(Lamps::Lmp_O, lampEPT && (SELF->elecrto[SELF->cabNum].RecuperationPosition > 0) );
    cab->SetDisplayState(Lamps::Lmp_T, lampEPT && (loco->BrakeCylinderPressure >= MIN_FOR_SOT));

    int NoVybeg = (SELF->elecrto[SELF->cabNum].RecuperationPosition) || (SELF->elecrto[SELF->cabNum].TyagaPosition);
    int LKiT = (SELF->elecrto[SELF->cabNum].lkitTime + 3 > SELF->game.currTime.seconds);

    cab->SetDisplayState(Lamps::Lmp_LKiT, canLight  && LKiT && NoVybeg);
    cab->SetDisplayState(Lamps::Lmp_RN, canLight  && !eng->MainSwitch);
    cab->SetDisplayState(Lamps::Lmp_VspomCepi, canLight && !eng->MainSwitch );
    cab->SetDisplayState(Lamps::Lmp_RNDK, canLight  && !eng->MainSwitch );
    cab->SetDisplayState(Lamps::Lmp_Preobr, canLight && !eng->MainSwitch );
    cab->SetDisplayState(Lamps::Lmp_BV,  canLight && !eng->MainSwitch );

    int doorsIsClosed = ( SELF->doors[0] == DOORS_CLOSED) && (SELF->doors[1] == DOORS_CLOSED);
    cab->SetDisplayState(Lamps::Lmp_Doors, ( canLight && doorsIsClosed) );
}
