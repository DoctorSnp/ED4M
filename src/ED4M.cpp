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
#include "private_ED4M.h"
#include "src/equipment/brake_395.h"

#include "ED4M.h"

constexpr float MIN_FOR_SOT = 0.2;

/*******************************    Закрытые функции    *******************************/
static void HodovayaSound(st_Self *self);

static void m_osveshenie(st_Self *self, const ElectricLocomotive * loco);

/**** Электрика ****/
static void  m_checkElectro(st_Self *SELF );
static float m_tractionForce ( st_Self *self);
static int   m_haveElectroEmergency(st_Self *self);
static void  m_checkBV(st_Self *self);
/*******************/

static void m_checkManagement(st_Self * self);
static void m_SetDoorsState(st_Self *SELF, int WhatDoors, int NewState );

static void m_displayPult(st_Self *self);

// отладочная печать раз в секунду
static void m_debugStep(st_Self *self);
/**************************************************************************************/


static SAUT saut;

static Brake_395 kran395;
static EPK epk;

int ED4M_init( st_Self *SELF, Locomotive *loco, Engine *eng)
{
    memset(SELF, 0, sizeof (struct st_Self));

    loco->HandbrakeValue=0.0;
    eng->HandbrakePercent=0.0;
    eng->DynamicBrakePercent=0;
    eng->MainResRate = 0.0;
    eng->Sanding = 0;

    eng->BrakeForce= 0.0;
    eng->ChargingRate = 0;
    eng->TrainPipeRate = 0;

    eng->AuxilaryRate = 0.0;
    eng->ALSNOn  =0;

    eng->EPTvalue = 0.0;
    eng->Reverse = 0;
    SELF->Reverse  = REVERSE_NEUTRAL;
    eng->ThrottlePosition = 0;

    SELF->pneumo.Arm_395 = _checkSwitch(loco, Arms::Arm_395, -1, 1, 0 ) + 1;
    SELF->elecrto.PantoRaised = 0;

    ftime(&SELF->debugTime.prevTime);
    SELF->debugTime.currTime = SELF->debugTime.prevTime;

    SELF->SL2M_Ticked = false;
    SELF->Reverse = REVERSE_NEUTRAL;

    /* Тест тормозов */
    loco->MainResPressure = 8.0;
    loco->TrainPipePressure = 5.0;
    loco->AuxiliaryPressure = 5.2;
    epk.init();
    saut.init();
    kran395.init(eng, 1);

    KLUB_init(&SELF->KLUB[0]);
    KLUB_init(&SELF->KLUB[1]);

    Radiostation_Init(&SELF->radio);

    for (int i =0; i< TUMBLERS_MAX_ID; i++)
        SELF->tumblersArray[i] = _checkSwitch(loco, i, -1, 1, 0);

    for (int i =0; i< ARMS_MAX_ID; i++)
        SELF->armsArray[i] = _checkSwitch(loco, i, -1, 1, 0);

    return 0;
}

void ED4M_ALSN(st_Self *SELF, const Locomotive *loco)
{
    SELF->alsn.PrevSpeed = SELF->prevVelocity;

    static int prevEpk = SELF->tumblersArray[Tumblers::Key_EPK];
    if ( prevEpk != SELF->tumblersArray[Tumblers::Key_EPK])
    {
        if (prevEpk == 0)
            saut.start(loco, loco->Eng(), &SELF->alsn);
        else
            saut.stop(loco, loco->Eng());
        prevEpk = SELF->tumblersArray[Tumblers::Key_EPK];
    }
    else
    {
       if (SELF->buttonsArray[Buttons::Btn_RB] || SELF->buttonsArray[Buttons::Btn_RB_D])
            epk.okey(loco);
        int sautState = saut.step(loco, loco->Eng(), &SELF->alsn);
        int currEpkState = epk.step(loco, sautState );
        if ( currEpkState )
        {
            //if (!SELF->flags.EPK_Triggered)
            //    _setBrakePosition(7, loco, loco->Eng(), SELF);
            //SELF->flags.EPK_Triggered = 1;
        }
    }

    if ( (SELF->tumblersArray[Tumblers::Switch_PitALSN_1]) && (SELF->tumblersArray[Tumblers::Switch_PitALSN_2]) )
    {
        loco->Eng()->ALSNOn = 1;
        if (SELF->tumblersArray[Tumblers::Key_EPK] )
            KLUB_setState(&SELF->KLUB[SELF->cabNum - 1], 2);
        else
            KLUB_setState(&SELF->KLUB[SELF->cabNum -1], 1);
    }
    else
         KLUB_setState(&SELF->KLUB[SELF->cabNum], 0);

    SELF->KLUB[SELF->cabNum -1].seconds = SELF->game.seconds;
    SELF->KLUB[SELF->cabNum -1].milliseconds = SELF->game.milliseconds;
    KLUB_Step(&SELF->KLUB[SELF->cabNum -1], loco->Eng(), SELF->alsn, loco);

}

int ED4M_Step(st_Self *SELF )
{
    int isMainSect = 0;
    ElectricEngine *eng = SELF->game.engPtr;
    const ElectricLocomotive *loco = SELF->game.locoPtr;

    if(loco->NumSlaves&&(loco->LocoFlags&1))
    {
        isMainSect = 1;
        int cabNum =  (int)loco->GetParameter( (wchar_t*)(L"CabNum"), 0.0);
        if (SELF->cabNum != cabNum)
        {
            if (SELF->cabNum != 0)
                Printer_print(eng, GMM_POST, L"Change Cabin from %d to %d", SELF->cabNum);
            SELF->cabNum = cabNum;
        }
        if (SELF->cabNum < 1)
            SELF->cabNum = 1;
        if ( SELF->cabNum > CABS_COUNT )
            SELF->cabNum = CABS_COUNT;
    }

    if (!isMainSect)
    {
        eng->Panto = ((unsigned char)SELF->elecrto.PantoRaised);
        eng->ThrottlePosition = SELF->TyagaPosition;
        return 1;
    }

    int brake = 0;
    setBitState((char*)&brake, 0, SELF->armsArray[Arms::Arm_395_TM_Control]);
    setBitState((char*)&brake, 1, !SELF->armsArray[Arms::Arm_395_NM_Control]);
    kran395.step(SELF->pneumo.Arm_395, loco, eng, SELF->game.milliseconds, brake);

    /*Грузим данные из движка себе в МОЗГИ*/

    m_checkElectro(SELF);
    /*Работаем в своём соку*/
    m_checkManagement(SELF);
    m_displayPult(SELF);
    m_osveshenie(SELF, loco);

    SELF->radio.isActive = SELF->tumblersArray[Tumblers::Switch_Radio];
    Radiostation_Step(loco, eng, &SELF->radio);
    HodovayaSound(SELF);

    /*А тепер пихаем из наших МОЗГОВ данные в движок*/
    eng->Reverse = SELF->Reverse - REVERSE_NEUTRAL;
    SELF->prevVelocity = SELF->alsn.CurrSpeed;
    float SetForce = m_tractionForce (SELF);

    if(SELF->alsn.CurrSpeed <= 3.01)
        SetForce *= 20000.0;
    else
        SetForce *= 34000.0;

    for (UINT i =0; i < loco->NumSlaves; i++)
        loco->Slaves[i]->Eng()->Force = (-SetForce) * (eng->Reverse);

    m_debugStep(SELF);
    return 1;
}

static void m_SetDoorsState(struct st_Self *SELF, int WhatDoors, int NewState)
{
    int dorrsArrayPos = Tumblers::Tmb_leftDoors;

    /* инвертируем двери */
    const ElectricLocomotive *loco = SELF->game.locoPtr;
    if( RTSGetInteger(loco ,RTS_TRAINDIR, 0) == 0)
    {
        if (WhatDoors == DOORS_RIGHT)
            WhatDoors = DOORS_LEFT;
        else if (WhatDoors == DOORS_LEFT)
            WhatDoors = DOORS_RIGHT;
    }

    if (WhatDoors == DOORS_RIGHT)
        dorrsArrayPos = Tumblers::Tmb_rightDoors;

    if (SELF->tumblersArray[dorrsArrayPos] != NewState)
    {
        SELF->tumblersArray[dorrsArrayPos] = NewState;
        if (SELF->tumblersArray[dorrsArrayPos] == 0)
        {
            loco->PostTriggerCab(SoundsID::DoorsOpen );
            loco->Eng()->PostGlobalMessage(GM_DOOR_UNLOCK, WhatDoors);
            loco->Eng()->PostGlobalMessage(GM_DOOR_OPEN, WhatDoors);
        }
        else
        {
            loco->PostTriggerCab(SoundsID::DoorsClose );
            loco->Eng()->PostGlobalMessage(GM_DOOR_CLOSE, WhatDoors);
            loco->Eng()->PostGlobalMessage(GM_DOOR_LOCK, WhatDoors);
        }
    }
}

/**
 * @brief HodovayaSound Озвучка ходовой части
 * @param self
 */
static void HodovayaSound(st_Self *self)
{
    static int isFinalStop = 0;
    float currSpeed = fabs(self->alsn.CurrSpeed);
    float prevSpeed = fabs(self->prevVelocity);

    if ( (prevSpeed > currSpeed) && (currSpeed > 0.3)  ) // типа, быстро так тормозим
    {
        if ((self->alsn.CurrSpeed < 3.0) && (isFinalStop == 0)  )
        {
            isFinalStop = 1;
            return;
        }
    }

    if (fabs(self->alsn.CurrSpeed) <= 0.00)
    {
        if (isFinalStop )
            isFinalStop = 0;
    }
}

static float m_tractionForce ( st_Self *self)
{
    float startForce = 0.0;

    if (self->elecrto.PantoRaised == 0)
        return 0.0;

    if (self->game.engPtr->MainSwitch == 0)
        return 0.0;

    self->game.engPtr->Reverse = self->Reverse - REVERSE_NEUTRAL;

    startForce = (float) (( self->game.engPtr->Reverse) * self->TyagaPosition )  ;
    return startForce;
}

static void m_checkManagement(st_Self * self)
{
    if ( self->tempFlags[Tumblers::Tmb_leftDoors] != FLAG_DISABLED )
    {
        m_SetDoorsState(self, DOORS_LEFT, self->tempFlags[Tumblers::Tmb_leftDoors]);
        self->tempFlags[Tumblers::Tmb_leftDoors] = FLAG_DISABLED;
    }
}

static void m_osveshenie( st_Self *self, const ElectricLocomotive * loco)
{
    /*loco->SwitchLight(en_Lights::Light_Proj_Half1, self->tumblers.projHalf, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Half2, self->tumblers.projHalf, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Half3, self->tumblers.projHalf, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Half4, self->tumblers.projHalf, 0.0, 0);

    loco->SwitchLight(en_Lights::Light_Proj_Full1, self->tumblers.projFull, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Full2, self->tumblers.projFull, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Full3, self->tumblers.projFull, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Full4, self->tumblers.projFull, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Full5, self->tumblers.projFull, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Full6, self->tumblers.projFull, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Full7, self->tumblers.projFull, 0.0, 0);
    loco->SwitchLight(en_Lights::Light_Proj_Full8, self->tumblers.projFull, 0.0, 0);*/

    loco->SwitchLight(en_Lights::Light_BufferRight, self->tumblersArray[Tumblers::Switch_BufRight], 0.0, 0);
    loco->SwitchLight(en_Lights::Light_BufferLeft, self->tumblersArray[Tumblers::Switch_BufLeft], 0.0, 0);\

    loco->SwitchLight(en_Lights::Light_SigUp1, self->tumblersArray[Tumblers::Switch_SigUp], 0.0, 0);
    loco->SwitchLight(en_Lights::Light_SigUp2, self->tumblersArray[Tumblers::Switch_SigUp], 0.0, 0);
    //loco->SwitchLight(en_Lights::Light_SigDown, self->tumblersArray[Tumblers::Switch_SigUp], 0.0, 0);

}


static void m_checkElectro(st_Self *SELF )
{
    const ElectricLocomotive *loco = SELF->game.locoPtr;
    SELF->elecrto.LineVoltage =  loco->LineVoltage;

    m_checkBV(SELF);

    if (SELF->tumblersArray[Tumblers::Switch_Panto] && SELF->tumblersArray[Tumblers::Tmb_PantoUP] )
    {
        if (!SELF->elecrto.PantoRaised)
        {
            SELF->elecrto.PantoRaised = 0xff;
            loco->PostTriggerCab(SoundsID::TP_UP);
        }
    }
    else if (SELF->tumblersArray[Tumblers::Tmb_PantoDown] || !SELF->tumblersArray[Tumblers::Switch_Panto] )
    {
        if (SELF->elecrto.PantoRaised)
        {
            SELF->elecrto.PantoRaised = 0x0;
            loco->PostTriggerCab(SoundsID::TP_DOWN);
            SELF->game.engPtr->MainSwitch = 0;
            SELF->BV_STATE = 0;
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
            SELF->BV_STATE = 0;
        }
    }

    if (SELF->tumblersArray[Tumblers::Tmb_vozvrZash])
    {
        if (SELF->BV_STATE < 1)
            SELF->BV_STATE = 1;
    }
    else
    {
        if (!SELF->tumblersArray[Tumblers::Tmb_vozvrZash])
            if ( SELF->BV_STATE == 1 )
                SELF->game.engPtr->MainSwitch = 1;
    }
}

/**
 * @brief m_haveElectroEmergency Если какие-то неисправности в электрике
 * @return
 */
static int m_haveElectroEmergency(st_Self *self)
{
    return 0;
}

/**
 * @brief _debugPrint Здесь можно
 */
static void m_debugStep(st_Self *self)
{
    ftime(&self->debugTime.currTime);
    if ((self->debugTime.prevTime.time + 1) > self->debugTime.currTime.time)
        return;
    if (wcslen(self->KLUB[self->cabNum -1].stName) )
        Printer_print(self->game.engPtr, GMM_POST, L"Station %s\n", self->KLUB[self->cabNum -1].stName);
    self->debugTime.prevTime = self->debugTime.currTime;
}

/**
 * @brief _displayPult Показывает манометры и лампы на пульте
 */
static void m_displayPult(st_Self *self)
{
    const ElectricLocomotive *loco = self->game.locoPtr;
    ElectricEngine *eng = self->game.engPtr;
    Cabin *cab = loco->cab;


    cab->SetDisplayValue(Sensors::Sns_Voltage, loco->LineVoltage );
    cab->SetDisplayValue(Sensors::Sns_BrakeCil, loco->BrakeCylinderPressure );
    cab->SetDisplayValue(Sensors::Sns_SurgeTank, eng->UR);
    cab->SetDisplayValue(Sensors::Sns_BrakeLine, loco->TrainPipePressure);
    cab->SetDisplayValue(Sensors::Sns_PressureLine, loco->ChargingPipePressure);

    int canLight = self->tumblersArray[Tumblers::Switch_VU] && self->tumblersArray[Tumblers::Switch_AutomatUpr];
    cab->SetDisplayState(Lamps::Lmp_SOT, canLight && (loco->BrakeCylinderPressure >= MIN_FOR_SOT) );
    cab->SetDisplayState(Lamps::Lmp_SOT_X, canLight && (loco->BrakeCylinderPressure >= MIN_FOR_SOT) );
    cab->SetDisplayState(Lamps::Lmp_RN, canLight );
    cab->SetDisplayState(Lamps::Lmp_VspomCepi, canLight );
    cab->SetDisplayState(Lamps::Lmp_RNDK, canLight );
    cab->SetDisplayState(Lamps::Lmp_Preobr, canLight && !eng->MainSwitch);
    cab->SetDisplayState(Lamps::Lmp_BV, ( canLight && !eng->MainSwitch) );

    int doorsIsClosed = 1;
    cab->SetDisplayState(Lamps::Lmp_Doors, ( canLight && doorsIsClosed) );
}
