//#define __STDIO_H
#include <stdio.h>

#include <math.h>
#include "utils/utils.h"
#include "equipment/epk.h"
#include "appliances/saut.h"
#include "appliances/klub.h"
#include "private_ED4M.h"
#include "src/equipment/brake_395.h"

#include "ED4M.h"


/*** Закрытые функции ***/
static void HodovayaSound(st_Self *self);

static float m_tractionForce ( st_Self *self);
static void _osveshenie(st_Self *self, const ElectricLocomotive * loco);
static int _haveElectroEmergency(st_Self *self);
static void _checkBV(st_Self *self);

static void _checkManagement(st_Self * self, const ElectricLocomotive *loco, ElectricEngine *eng);
static void _SetDoorsState(struct st_Self *SELF, int WhatDoors, int NewState );

static void _displayPult(st_Self *self, const ElectricLocomotive *loco, ElectricEngine *eng);
static void _debugStep(st_Self *self, const ElectricLocomotive *loco, ElectricEngine *eng);

/************************/

//static SAUT saut;

static Brake_395 kran395;
static EPK epk;

int ED4M_init( st_Self *SELF, Locomotive *loco, Engine *eng)
{
    memset(SELF, 0, sizeof (struct st_Self));

    loco->HandbrakeValue=0.0;
    eng->HandbrakePercent=0.0;
    eng->DynamicBrakePercent=0;
    eng->MainResRate=0.0;
    eng->Sanding=0;

    eng->BrakeForce=0.0;
    eng->ChargingRate =0;
    eng->TrainPipeRate =0;
    eng->BrakeSystemsEngaged = 1;

    eng->AuxilaryRate=0.0;
    eng->ALSNOn=0;

    eng->EPTvalue = 0.0;
    eng->Reverse = 0;
    SELF->Reverse  = REVERSE_NEUTRAL;
    eng->ThrottlePosition = 0;

    SELF->pneumo.Arm_395 = _checkSwitchWithSound(loco, Arms::Arm_395, -1, 1, 0 ) + 1;
    SELF->elecrto.PantoRaised = 0;

    eng->var[3]= (float)GetTickCount();

    ftime(&SELF->debugTime.prevTime);
    SELF->debugTime.currTime = SELF->debugTime.prevTime;

    SELF->SL2M_Ticked = false;
    SELF->Reverse = REVERSE_NEUTRAL;

    /* Тест тормозов */
    loco->MainResPressure = 8.0;
    loco->TrainPipePressure = 5.0;
    loco->AuxiliaryPressure = 5.2;
    //saut.init();
    kran395.init();
    KLUB_init(&SELF->KLUB);
    Radiostation_Init(&SELF->radio);
    eng->var[3] = GetTickCount();
    return 0;
}

void ED4M_ALSN(st_Self *SELF, const Locomotive *loco)
{
    SELF->alsn.PrevSpeed = SELF->prevVelocity;

    static int prevEpk = SELF->EPK;
    if ( prevEpk != SELF->EPK)
    {
      /*  if (prevEpk == 0)
            saut.start(loco, loco->Eng(), &SELF->alsn);
        else
            saut.stop(loco, loco->Eng());*/
        prevEpk = SELF->EPK;
    }
    else
    {
       // if (SELF->RB)
            //epk.okey(loco);
       // int sautState = saut.step(loco, loco->Eng(), &SELF->alsn);
      /*  int currEpkState = epk.step(loco, sautState );
        if ( currEpkState )
        {
            if (!SELF->flags.EPK_Triggered)
                _setBrakePosition(7, loco, loco->Eng(), SELF);
            SELF->flags.EPK_Triggered = 1;
        }*/
    }

    if ( (SELF->tumblersArray[Tumblers::Switch_PitALSN_1]) && (SELF->tumblersArray[Tumblers::Switch_PitALSN_2]) )
    {
        loco->Eng()->ALSNOn = 1;
        if (SELF->tumblersArray[Tumblers::Key_EPK] )
            KLUB_setState(&SELF->KLUB, 2);
        else
            KLUB_setState(&SELF->KLUB, 1);
    }
    else
         KLUB_setState(&SELF->KLUB, 0);

    KLUB_Step(&SELF->KLUB, loco->Eng(), SELF->alsn, loco);

}

int ED4M_Step(st_Self *SELF )
{
    int isMainSect = 0;
    ElectricEngine *eng = SELF->game.engPtr;
    const ElectricLocomotive *loco = SELF->game.locoPtr;

    kran395.step(SELF->pneumo.Arm_395, loco, eng, SELF->game.time);

    if(loco->NumSlaves&&(loco->LocoFlags&1))
    {
        isMainSect = 1;
        wchar_t parameter[32];
        swprintf_s(parameter, sizeof (parameter), L"%s", "cabNum");
        SELF->service.cabNum =  loco->GetParameter(parameter, 44);
    }

    if (!isMainSect)
    {
        eng->Panto = ((unsigned char)SELF->elecrto.PantoRaised);
        eng->ThrottlePosition = SELF->TyagaPosition;
        return 1;
    }

    _checkBV(SELF);
    SELF->radio.isActive = SELF->tumblersArray[Tumblers::Switch_Radio];

    /*Грузим данные из движка себе в МОЗГИ*/
    SELF->elecrto.LineVoltage =  loco->LineVoltage;

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
        }
    }

    /*Работаем в своём соку*/
    _checkManagement(SELF, loco, eng);
    _displayPult(SELF, loco, eng);

    _osveshenie(SELF, loco);
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

    for (UINT i =0; i < loco->NumSlaves-1; i++)
    {
        //if ( i<=3)
            loco->Slaves[i]->Eng()->Force = (-SetForce) * (eng->Reverse);
        //else
        //    loco->Slaves[i]->Eng()->Force = -SetForce;
    }
    _debugStep(SELF, loco, eng);
    return 1;
}

static void _SetDoorsState(struct st_Self *SELF, int WhatDoors, int NewState)
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
        if (SELF->tumblersArray[dorrsArrayPos])
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
 * @param loco
 * @param eng
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
    /*if (self->game.engPtr->Reverse > 0)
        self->destination = SECTION_DEST_FORWARD;
    else if (self->Reverse < REVERSE_NEUTRAL)
        self->destination = SECTION_DEST_BACKWARD;
    else
        self->destination = 0;*/

    startForce = (float) (( self->game.engPtr->Reverse) * self->TyagaPosition )  ;
    return startForce;
}

static void _checkManagement(st_Self * self, const ElectricLocomotive *loco, ElectricEngine *eng)
{
   // Cabin *cab = loco->cab;

    if ( self->tempFlags[Tumblers::Tmb_leftDoors] != FLAG_DISABLED )
    {
        _SetDoorsState(self, DOORS_LEFT, self->tempFlags[Tumblers::Tmb_leftDoors]);
        self->tempFlags[Tumblers::Tmb_leftDoors] = FLAG_DISABLED;
    }
}

static void _osveshenie( st_Self *self, const ElectricLocomotive * loco)
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

static void _checkBV(st_Self *self)
{
    if (_haveElectroEmergency(self) )
    {
        if (self->game.engPtr->MainSwitch)
        {
            self->game.engPtr->MainSwitch = 0;
            self->BV_STATE = 0;
        }
    }

    if (self->tumblersArray[Tumblers::Tmb_vozvrZash])
    {
        if (self->BV_STATE < 1)
            self->BV_STATE = 1;
    }
    else
    {
        if (!self->tumblersArray[Tumblers::Tmb_vozvrZash])
            if ( self->BV_STATE == 1 )
                self->game.engPtr->MainSwitch = 1;
    }
}

static int _haveElectroEmergency(st_Self *self)
{
    return 0;
}

/**
 * @brief _debugPrint
 * @param loco
 * @param eng
 * @param self
 */
static void _debugStep(st_Self *self, const ElectricLocomotive *loco, ElectricEngine *eng)
{
    ftime(&self->debugTime.currTime);
    if ((self->debugTime.prevTime.time + 1) > self->debugTime.currTime.time)
        return;

    Printer_print(eng, GMM_POST, L"Time1 %f,  Time2 %f, Time3 %f, Time4 %f \n",
                  eng->var[3], eng->var[4], eng->var[5], eng->var[13]);
    self->debugTime.prevTime = self->debugTime.currTime;
}

/**
 * @brief _displayPult Показывает манометры и лампы на пульте
 * @param self
 * @param loco
 * @param eng
 */
static void _displayPult(st_Self *self, const ElectricLocomotive *loco, ElectricEngine *eng)
{
    Cabin *cab = loco->cab;

    cab->SetDisplayValue(Sensors::Sns_Voltage, loco->LineVoltage );
    cab->SetDisplayValue(Sensors::Sns_BrakeCil, loco->BrakeCylinderPressure );
    cab->SetDisplayValue(Sensors::Sns_SurgeTank, eng->UR);
    cab->SetDisplayValue(Sensors::Sns_BrakeLine, loco->TrainPipePressure);
    cab->SetDisplayValue(Sensors::Sns_PressureLine, loco->ChargingPipePressure);

    int canLight = self->tumblersArray[Tumblers::Switch_VU] && self->tumblersArray[Tumblers::Switch_AutomatUpr];
    cab->SetDisplayState(Lamps::Lmp_SOT, canLight );
    cab->SetDisplayState(Lamps::Lmp_SOT_X, canLight );
    cab->SetDisplayState(Lamps::Lmp_RN, canLight );
    cab->SetDisplayState(Lamps::Lmp_VspomCepi, canLight );
    cab->SetDisplayState(Lamps::Lmp_RNDK, canLight );
    cab->SetDisplayState(Lamps::Lmp_Preobr, canLight );
    cab->SetDisplayState(Lamps::Lmp_BV, ( canLight && !eng->MainSwitch) );

    int doorsIsClosed = 1;
    cab->SetDisplayState(Lamps::Lmp_Doors, ( canLight && doorsIsClosed) );
}
