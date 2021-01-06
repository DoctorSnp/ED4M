#include <sys/timeb.h>
#include <stdio.h>
#include <wchar.h>
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>  // For _CrtSetReportMode
#include <errno.h>

#include "utils/utils.h"
#include "epk.h"
#include "saut.h"
#include "klub.h"
#include "math.h"
#include "ED4M.h"

#define BRAKE_STR_RATE 1.8
#define TR_CURRENT_C 272.0
#define BRAKE_MR_RATIO    0.005
#define BRAKE_PIPE_RATE_CHARGE 2.5
#define BRAKE_UR_RATE_CHARGE   0.3
#define BRAKE_PIPE_RATE 0.4
#define BRAKE_PIPE_EMERGENCY -1.2
#define PIPE_DISCHARGE_SLOW -0.005
#define UR_DISCHARGE2     0.003


/*** Закрытые функции ***/
static void HodovayaSound(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self *self);
static int m_getDest(st_Self *self);

static float m_tractionForce ( st_Self *self);
static void _osveshenie(const ElectricLocomotive * loco, st_Self *self);
static void _checkBrake(const ElectricLocomotive * loco, ElectricEngine *eng, st_Self *self);

static void _checkManagement(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self * self);

static void _debugStep(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self *self);
/************************/

//static SAUT saut;
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
    SELF->Reverse = eng->Reverse = REVERSE_NEUTRAL;
    eng->ThrottlePosition = 0;

    SELF->pneumo.Arm_395 = _checkSwitchWithSound(loco, Arms::Arm_395, -1, 1, 0 ) + 1;
    SELF->elecrto.PantoRaised = 0;

    eng->var[3]=GetTickCount();

    ftime(&SELF->debugTime.prevTime);
    SELF->debugTime.currTime = SELF->debugTime.prevTime;

    SELF->SL2M_Ticked = false;
    SELF->Reverse = REVERSE_NEUTRAL;

    /* Тест тормозов */
    loco->MainResPressure=8.0;
    loco->TrainPipePressure=5.0;
    loco->AuxiliaryPressure=5.2;
    //saut.init();

    KLUB_init(&SELF->KLUB);

    Radiostation_Init(&SELF->radio);
    Radiostation_setEnabled(&SELF->radio, 1);

    SELF->alsn.SpeedLimit.Limit = 0.0;
    SELF->alsn.SpeedLimit.Distance = 111.0;

    return 1;
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

int ED4M_Step(st_Self *SELF, const ElectricLocomotive *loco, ElectricEngine *eng, float gameTime )
{
    Cabin *cab = loco->cab;
    SELF->service.gameTime = gameTime;

    int isMainSect = 0;

    _checkBrake(loco, eng, SELF);

    if(loco->NumSlaves&&(loco->LocoFlags&1))
    {
        isMainSect = 1;
    }

    if (!isMainSect)
    {
        eng->Panto = ((unsigned char)SELF->elecrto.PantoRaised);
        eng->ThrottlePosition = SELF->TyagaPosition;
        return 1;
    }

    //if(!loco->Velocity)
    //    RTSSetIntegerG(loco,RTS_TRAINDIR,loco->LibParam==1?-1:1);

    /*Грузим данные из движка себе в МОЗГИ*/
    SELF->elecrto.LineVoltage =  loco->LineVoltage;

    cab->SetDisplayValue(Sensors::Sns_Voltage, loco->LineVoltage );

    cab->SetDisplayValue(Sensors::Sns_BrakeCil, loco->BrakeCylinderPressure );
    cab->SetDisplayValue(Sensors::Sns_SurgeTank, eng->UR);
    cab->SetDisplayValue(Sensors::Sns_BrakeLine, loco->TrainPipePressure);
    cab->SetDisplayValue(Sensors::Sns_PressureLine, loco->ChargingPipePressure);


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
    _checkManagement(loco, eng, SELF);

    _osveshenie(loco, SELF);
    Radiostation_Step(loco, eng, &SELF->radio);
    HodovayaSound(loco, eng, SELF);

    /*А тепер пихаем из наших МОЗГОВ данные в движок*/
    eng->Reverse = SELF->Reverse;
    eng->IndependentBrakeValue= ( SELF->pneumo.Arm_254 - 1) * 1.0;
    SELF->prevVelocity = SELF->alsn.CurrSpeed;
    float SetForce = m_tractionForce (SELF);

    if(SELF->alsn.CurrSpeed <= 3.01)
        SetForce *= 20000.0;
    else
        SetForce *= 34000.0;

    for (UINT i =0; i < loco->NumSlaves-1; i++)
    {
        //if ( i<=3)
            loco->Slaves[i]->Eng()->Force = -SetForce;
        //else
        //    loco->Slaves[i]->Eng()->Force = -SetForce;
    }
   // _debugStep(loco, eng, SELF);
    return 1;
}

void ED4M_SetDoorsState(struct st_Self *SELF, int WhatDoors, int NewState, const ElectricLocomotive *loco )
{
    int dorrsArrayPos = Tumblers::Tmb_leftDoors;

    /* инвертируем двери */
    if( RTSSetIntegerG(loco,RTS_TRAINDIR,loco->LibParam == 0))
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
static void HodovayaSound(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self *self)
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

static int m_getDest( st_Self *self)
{
    if (self->Reverse > REVERSE_NEUTRAL)
        self->destination = SECTION_DEST_FORWARD;
    else if (self->Reverse < REVERSE_NEUTRAL )
        self->destination = SECTION_DEST_FORWARD;
    else
         self->destination = 0;
    return self->destination;
}

static float m_tractionForce ( st_Self *self)
{
    float startForce = 0.0;

    if (self->elecrto.PantoRaised == 0)
        return 0.0;
    startForce = (float) (( self->Reverse - REVERSE_NEUTRAL) * self->TyagaPosition )  ;
    startForce *= float(m_getDest(self));
    return startForce;
}

static void _checkBrake(const ElectricLocomotive * loco, ElectricEngine *eng, st_Self *self)
{
    eng->MainResRate=0.04*(11.0-loco->MainResPressure);

    //Train brake
    eng->TrainPipeRate=0.0;
    if( !(loco->LocoFlags&1))
        return;

    //Printer_print(eng, GMM_POST, L"Braking: %d\n GameTime %f", self->pneumo.Arm_395, self->service.gameTime);

    switch(self->pneumo.Arm_395){
    case 0:
        if(eng->var[5]<45.0)
        {
            //if(!cab->Switches[3].SetState)
            eng->UR += BRAKE_UR_RATE_CHARGE * self->service.gameTime;
            if(eng->UR>loco->MainResPressure)
                eng->UR=loco->MainResPressure;
            if(loco->TrainPipePressure < eng->UR)
                eng->TrainPipeRate=(eng->UR-loco->TrainPipePressure)*1.5;

        }
        break;
    case 1:
        if(eng->var[5]<45.0)
        {
            if(eng->UR<5.0)
            {
                float rate=(loco->MainResPressure-eng->UR)*2.0;
                if(rate<0.0)rate=0.0;
                if(rate>BRAKE_UR_RATE_CHARGE)rate=BRAKE_UR_RATE_CHARGE;
                eng->UR+=rate * self->service.gameTime;
            }
            else
                if(loco->BrakeCylinderPressure>0.0&&(eng->UR-loco->TrainPipePressure)<0.1)
                    eng->UR+=0.15 * self->service.gameTime;
            if(eng->UR>loco->MainResPressure)
                eng->UR=loco->MainResPressure;
            if(eng->UR>loco->TrainPipePressure)
                eng->UR-=0.003 * self->service.gameTime;
            if(loco->TrainPipePressure<eng->UR-0.01)
                eng->TrainPipeRate=(eng->UR-loco->TrainPipePressure)/BRAKE_PIPE_RATE_CHARGE;
            else if(loco->TrainPipePressure>eng->UR)
            {
                eng->TrainPipeRate=(eng->UR-loco->TrainPipePressure)/BRAKE_PIPE_RATE_CHARGE;
                if(eng->TrainPipeRate<-BRAKE_PIPE_RATE)
                    eng->TrainPipeRate=-BRAKE_PIPE_RATE;
            }
        }
        break;
    case 2:
        if(eng->UR>loco->MainResPressure)
            eng->UR=loco->MainResPressure;
        if(loco->TrainPipePressure>eng->UR)
            eng->TrainPipeRate=eng->UR-loco->TrainPipePressure;
        if(eng->TrainPipeRate<-BRAKE_PIPE_RATE)
            eng->TrainPipeRate=-BRAKE_PIPE_RATE;
        if(eng->TrainPipeRate>PIPE_DISCHARGE_SLOW)
            eng->TrainPipeRate=PIPE_DISCHARGE_SLOW;

        break;
    case 3:
        if(eng->UR>loco->MainResPressure)
            eng->UR=loco->MainResPressure;
        if(loco->TrainPipePressure>eng->UR)
            eng->TrainPipeRate=eng->UR-loco->TrainPipePressure;
        else if(eng->UR-loco->TrainPipePressure>0.1)
            eng->TrainPipeRate=0.05;
        if(eng->TrainPipeRate<-BRAKE_PIPE_RATE)
            eng->TrainPipeRate=-BRAKE_PIPE_RATE;

        break;
    case 4:
        //if(cab->Switches[3].SetState!=4)
        // break;
        eng->UR-=0.3  * self->service.gameTime;
        if(eng->UR>loco->MainResPressure)
            eng->UR=loco->MainResPressure;
        if(eng->UR<0)
            eng->UR=0;
        eng->TrainPipeRate=-0.25;


        break;
    case 5:
        eng->UR+=BRAKE_PIPE_EMERGENCY  *1.2 * self->service.gameTime;
        if(eng->UR > loco->MainResPressure)
            eng->UR=loco->MainResPressure;
        if(eng->UR<0)
            eng->UR=0;
        eng->TrainPipeRate = BRAKE_PIPE_EMERGENCY;
        break;
    default:
        break;
    }
}

static void _checkManagement(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self * self)
{
    Cabin *cab = loco->cab;
    int isVU =  self->tumblersArray[Tumblers::Tmb_VU];
    cab->SetDisplayState(Lamps::Lmp_SOT, isVU );
    cab->SetDisplayState(Lamps::Lmp_SOT_X, isVU );
    cab->SetDisplayState(Lamps::Lmp_RN, isVU );
    cab->SetDisplayState(Lamps::Lmp_VspomCepi, isVU );
    cab->SetDisplayState(Lamps::Lmp_RNDK, isVU );
    cab->SetDisplayState(Lamps::Lmp_Preobr, isVU );
    cab->SetDisplayState(Lamps::Lmp_BV, isVU );
}

static void _osveshenie(const ElectricLocomotive * loco, st_Self *self)
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

/**
 * @brief _debugPrint
 * @param loco
 * @param eng
 * @param self
 */
static void _debugStep(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self *self)
{
    ftime(&self->debugTime.currTime);
    if ((self->debugTime.prevTime.time + 1) > self->debugTime.currTime.time)
        return;

    Printer_print(eng, GMM_POST, L"BrakeForce: %f Tyaga: %f 395 pos: %d Dist: %d Volt %f \n",
                eng->BrakeForce, eng->Force,
                self->pneumo.Arm_395, self->alsn.SpeedLimit.Distance, loco->LineVoltage );
    self->debugTime.prevTime = self->debugTime.currTime;
}
