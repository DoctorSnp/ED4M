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


/*** Закрытые функции ***/
static void HodovayaSound(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self *self);
static int m_getDest(st_Self *self);

static float m_tractionForce ( st_Self *self);
static void _osveshenie(const ElectricLocomotive * loco, st_Self *self);
static void _checkBrake(const ElectricLocomotive * loco, ElectricEngine *eng, st_Self *self);

static void _checkManagement(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self * self);


static void _setBrakePosition(int newPos, const Locomotive * loco, Engine *eng, st_Self *self);
static void _debugStep(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self *self);
/************************/

//static SAUT saut;
static EPK epk;

bool ED4M_init( st_Self *SELF, Locomotive *loco, Engine *eng)
{
    memset(SELF, 0, sizeof (struct st_Self));

    loco->HandbrakeValue=0.0;
    eng->HandbrakePercent=0.0;
    eng->DynamicBrakePercent=0;
    eng->MainResRate=0.0;
    eng->Sanding=0;
    eng->BrakeSystemsEngaged=2;
    eng->BrakeForce=0.0;
    eng->ChargingRate=0;
    eng->TrainPipeRate=0;
    eng->UR=0.0;
    eng->AuxilaryRate=0.0;
    eng->ALSNOn=0;
    eng->EPTvalue = 0.0;
    SELF->Reverse = eng->Reverse = REVERSE_NEUTRAL;
    eng->ThrottlePosition = 0;
    loco->MainResPressure=1.2;
    SELF->pneumo.Arm_395 = _checkSwitchWithSound(loco, Arms::Arm_395, -1, 1, 0 ) + 1;
    SELF->elecrto.PantoRaised = 0;

    ftime(&SELF->debugTime.prevTime);
    SELF->debugTime.currTime = SELF->debugTime.prevTime;

    SELF->SL2M_Ticked = false;
    SELF->Reverse = REVERSE_NEUTRAL;

    //saut.init();

    KLUB_init(&SELF->KLUB);

    Radiostation_Init(&SELF->radio);
    Radiostation_setEnabled(&SELF->radio, 1);

    SELF->alsn.SpeedLimit.Limit = 0.0;
    SELF->alsn.SpeedLimit.Distance = 111.0;
    return true;
}

void ED4M_ALSN(st_Self *SELF, const Locomotive *loco)
{
    SELF->alsn.PrevSpeed = SELF->prevVelocity;
   #if 0
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
#endif
    if ( (SELF->tumblersArray[Tumblers::Switch_PitALSN_1]) && (SELF->tumblersArray[Tumblers::Switch_PitALSN_2]) )
    {
        loco->Eng()->ALSNOn = 1;
        KLUB_setState(&SELF->KLUB, 1);
        if (SELF->tumblersArray[Tumblers::Key_EPK] )
            KLUB_setState(&SELF->KLUB, 2);
    }
    else
         KLUB_setState(&SELF->KLUB, 0);

    //Printer_print(loco->Eng(), GMM_POST, L"KLUB: %d ALSN sig: %d \n", SELF->KLUB.isOn, m_getSignCode( &SELF->alsn));
    KLUB_Step(&SELF->KLUB, loco->Eng(), SELF->alsn, loco);

}

int ED4M_Step(st_Self *SELF, const ElectricLocomotive *loco, ElectricEngine *eng, float gameTime )
{
    Cabin *cab = loco->cab;
    int isMainSect = 0;

    eng->ChargingRate = (SELF->pneumo.Arm_395 - 1) * 100;

    if(loco->NumSlaves&&(loco->LocoFlags&1))
        isMainSect = 1;

    if (!isMainSect)
    {
        eng->Panto = ((unsigned char)SELF->elecrto.PantoRaised);
        eng->ThrottlePosition = SELF->TyagaPosition;
        return 1;
    }

    /*Грузим данные из движка себе в МОЗГИ*/
    SELF->elecrto.LineVoltage =  loco->LineVoltage;
    cab->SetDisplayValue(Sensors::Sns_BrakeLine, loco->TrainPipePressure);
    cab->SetDisplayValue(Sensors::Sns_PressureLine, loco->ChargingPipePressure);
    cab->SetDisplayValue(Sensors::Sns_BrakeCil, loco->BrakeCylinderPressure );
    cab->SetDisplayValue(Sensors::Sns_Voltage, loco->LineVoltage );

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

    eng->UR = 8.0;
    eng->AuxilaryRate = 8.0;
    eng->BrakeSystemsEngaged = 1;
    /*Работаем в своём соку*/
    _checkManagement(loco, eng, SELF);
    _checkBrake(loco, eng, SELF);
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
        //if (CHECK_BIT(loco->Slaves[i]->Eng()->Flags, 1))
        //        Printer_print(eng, GMM_POST, L"Flags: %lu\n", loco->Slaves[i]->Eng()->Flags);

        //if ( i<=3)
            loco->Slaves[i]->Eng()->Force = -SetForce;
        //else
        //    loco->Slaves[i]->Eng()->Force = -SetForce;
    }
   // _debugStep(loco, eng, SELF);
    return 1;
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
            loco->PostTriggerCab(SoundsID::FinalStop );
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
        self->secdionCabDest = SECTION_DEST_FORWARD;
    else if (self->Reverse < REVERSE_NEUTRAL )
        self->secdionCabDest = SECTION_DEST_FORWARD;
    else
         self->secdionCabDest = 0;
    return self->secdionCabDest;
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

}

static void _setBrakePosition(int newPos, const Locomotive *loco, Engine *eng, st_Self *self)
{

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
