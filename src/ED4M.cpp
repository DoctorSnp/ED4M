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
static void SL2m_step(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self *self);
static void HodovayaSound(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self *self);
static int m_getDest(st_Self *self);

static float m_tractionForce ( st_Self *self);
static void _osveshenie(const ElectricLocomotive * loco, st_Self *self);
static void _checkBrake(const ElectricLocomotive * loco, ElectricEngine *eng, st_Self *self);
static void _setBrakePosition(int newPos, const Locomotive * loco, Engine *eng, st_Self *self);
static void _debugStep(const Locomotive *loco, Engine *eng, st_Self *self);
/************************/

static SAUT saut;
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
    SELF->pneumo.Arm_395 = 7 - _checkSwitchWithSound(loco, Arms::Arm_395, -1, 1, 0 );


    SELF->brake395_pos = SELF->pneumo.Arm_395;
    ftime(&SELF->debugTime.prevTime);
    SELF->debugTime.currTime = SELF->debugTime.prevTime;

    SELF->SL2M_Ticked = false;
    SELF->Reverse = REVERSE_NEUTRAL;

    ED4M_set_destination(SELF, SECTION_DEST_BACKWARD);
    saut.init();

    KLUB_init(&SELF->KLUB, loco);

    Radiostation_Init(&SELF->radio);
    Radiostation_setEnabled(&SELF->radio, 1);

    SELF->alsn.SpeedLimit.Limit = 0.0;
    SELF->alsn.SpeedLimit.Distance = 0.0;

    return true;
}

void ED4M_set_destination(st_Self *SELF, int SectionDest)
{
    SELF->secdionCabDest = SectionDest;
}

void ED4M_ALSN(st_Self *SELF, const Locomotive *loco)
{
    SELF->alsn.PrevSpeed = SELF->prevVelocity;
    static int prevEpk = SELF->EPK;
    if ( prevEpk != SELF->EPK)
    {
        if (prevEpk == 0)
            saut.start(loco, loco->Eng(), &SELF->alsn);
        else
            saut.stop(loco, loco->Eng());
        prevEpk = SELF->EPK;
    }
    else
    {
        if (SELF->RB)
            epk.okey(loco);
        int sautState = saut.step(loco, loco->Eng(), &SELF->alsn);
        int currEpkState = epk.step(loco, sautState );
        if ( currEpkState )
        {
            if (!SELF->flags.EPK_Triggered)
                _setBrakePosition(7, loco, loco->Eng(), SELF);
            SELF->flags.EPK_Triggered = 1;
        }
    }
}

int ED4M_Step(st_Self *SELF, const ElectricLocomotive *loco, ElectricEngine *eng, float gameTime )
{

    Cabin *cab = loco->cab;
    cab->SetDisplayValue(Sensors::Sns_BrakeLine, 3.0);
    cab->SetDisplayValue(Sensors::Sns_PressureLine, 3.0);
    cab->SetDisplayValue(Sensors::Sns_BrakeCil, 16.0 );


   // cab->SetDisplayValue(Sensors::SnsSpeed1, 24 );

    //cab->SetScreenLabel(Sensors::SnsSpeed1,1, w_speed);
    SELF->KLUB.isOn = SELF->tumblersArray[Tumblers::Key_EPK];
    KLUB_Step(&SELF->KLUB, eng, SELF->alsn);
       /*cab->SetDisplayState(Lamps::Lmp_Doors, 1 );
    cab->SetDisplayState(Lamps::Lmp_RN, 1 );
    cab->SetDisplayState(Lamps::Lmp_RP, 1 );
    cab->SetDisplayState(Lamps::Lmp_SOT, 1 );*/


     int isMainSect = 0;
     ElectricLocomotive *BackSec=NULL;
     if(loco->NumSlaves&&(loco->LocoFlags&1))
     {
       BackSec=(ElectricLocomotive *)loco->Slaves[0];
       isMainSect = 1;
     }

     if (isMainSect)
     {
         /*Грузим данные из движка себе в МОЗГИ*/
         SELF->elecrto.LineVoltage =  loco->LineVoltage;
        // Cabin *cab = loco->cab;

         /*Работаем в своём соку*/
         _checkBrake(loco, eng, SELF);
         _osveshenie(loco, SELF);
         Radiostation_Step(loco, eng, &SELF->radio);
         SL2m_step(loco, eng, SELF);
         HodovayaSound(loco, eng, SELF);

         /*А тепер пихаем из наших МОЗГОВ данные в движок*/
         eng->Panto = ((unsigned char)SELF->elecrto.PantoRaised);
         eng->ThrottlePosition = SELF->TyagaPosition;
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
             if ( (i>= 0) && (i<=3))
                loco->Slaves[i]->Eng()->Force = SetForce;
             else
                loco->Slaves[i]->Eng()->Force = -SetForce;
         }

     }
    eng->ChargingRate = (SELF->brake395_pos - 1) * 100;
    _debugStep(loco, eng, SELF);


    // SELF->IndependentBrakeValue = eng->IndependentBrakeValue;
   //  SELF->BrakeForce = eng->BrakeForce;
     //SELF->Force = eng->Force;
   //  SELF->EngineForce = *eng->EngineForce;
   //  SELF->EngineCurrent  = *eng->EngineCurrent;
   //  SELF->Power = eng->Power;

    // так как у нас всегда всё нормалёк - возвращаем 1
    return 1;
}

/**
 * @brief SL2m_step Шаг работы Скоростемера
 * @param loco
 * @param eng
 * @param self
 */
static void SL2m_step(const ElectricLocomotive *loco, ElectricEngine *eng, st_Self *self)
{
    if (self->alsn.CurrSpeed >= 5.0)
    {
        if (self->SL2M_Ticked == false )
        {
            loco->PostTriggerCab(Equipment::SL_2M);
            self->SL2M_Ticked = true;
        }
    }
    else
    {
        if (self->SL2M_Ticked == true )
        {
            loco->PostTriggerCab(Equipment::SL_2M + 1);
            self->SL2M_Ticked = false;
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
        return SECTION_DEST_FORWARD * self->secdionCabDest;
    else if (self->Reverse < REVERSE_NEUTRAL )
        return SECTION_DEST_BACKWARD * self->secdionCabDest;
    else
        return 0;
}

static float m_tractionForce ( st_Self *self)
{
    float startForce = 0.0;
    //if (self->BV_STATE == 0)
    //    return 0.0;
    //if (self->elecrto.PantoRaised == 0)
    //    return 0.0;
   startForce = (float) (( self->Reverse - REVERSE_NEUTRAL) * self->TyagaPosition )  ;
    //startForce = self->TyagaPosition ;
    //startForce *= float(m_getDest(self));
    return startForce;
}

static void _checkBrake(const ElectricLocomotive * loco, ElectricEngine *eng, st_Self *self)
{
    /*корректируем показатели */
    if (self->pneumo.Arm_254 < 1)
        self->pneumo.Arm_254 = 1;

    if (self->pneumo.Arm_395 < 1)
        self->pneumo.Arm_395 = 1;

    if ( self->brake395_pos == self->pneumo.Arm_395)
        return;

    if (self->flags.EPK_Triggered )
    {    if (self->pneumo.Arm_395 == 7)
        {
            _setBrakePosition(self->pneumo.Arm_395, loco, eng, self);
            self->flags.EPK_Triggered = 0;
        }
        else
            return;
    }
    else
        _setBrakePosition(self->pneumo.Arm_395, loco, eng, self);
}

static void _setBrakePosition(int newPos, const Locomotive *loco, Engine *eng, st_Self *self)
{
    if (self->brake395_pos == 7)
        loco->PostTriggerCab(SoundsID::Kran395_Extrennoe +1);

    if ((self->brake395_pos == 5) || (self->brake395_pos == 6))
        loco->PostTriggerCab(SoundsID::Kran395_Slugebnoe +1);
    if (self->brake395_pos == 2)
        loco->PostTriggerCab(SoundsID::Kran395_Poezdnoe +1);

    if (self->brake395_pos == 1)
        loco->PostTriggerCab(SoundsID::Kran395_Otpusk +1);

    if (newPos == 7)
        loco->PostTriggerCab(SoundsID::Kran395_Extrennoe);
    else if ( (newPos == 5) || (newPos == 5) )
        loco->PostTriggerCab(SoundsID::Kran395_Slugebnoe);

    else if ((newPos == 4) || (newPos == 3))
        loco->PostTriggerCab(SoundsID::Kran395_Slugebnoe + 1);

    else if (newPos == 2)
        loco->PostTriggerCab(SoundsID::Kran395_Poezdnoe);

    else if (newPos == 1)
        loco->PostTriggerCab(SoundsID::Kran395_Otpusk);
    else
        loco->PostTriggerCab(SoundsID::Kran395_Otpusk + 1);
    self->brake395_pos = newPos;
    eng->BrakeForce = (float) ( (self->brake395_pos - 1) * 2.0 );
    eng->EPTvalue = eng->BrakeForce;
    eng->TrainPipeRate = -eng->BrakeForce;
}


static void _osveshenie(const ElectricLocomotive * loco, st_Self *self)
{
    loco->SwitchLight(en_Lights::Light_Proj_Half1, self->tumblers.projHalf, 0.0, 0);
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
    loco->SwitchLight(en_Lights::Light_Proj_Full8, self->tumblers.projFull, 0.0, 0);
}

/**
 * @brief _debugPrint
 * @param loco
 * @param eng
 * @param self
 */
static void _debugStep(const Locomotive *loco, Engine *eng, st_Self *self)
{
    ftime(&self->debugTime.currTime);
    if ((self->debugTime.prevTime.time + 1) > self->debugTime.currTime.time)
        return;

    Printer_print(eng, GMM_POST, L"BrakeForce: %f SAUT:%s  Tyaga: %f Brake 395 pos: %d\n",
                eng->BrakeForce, saut.stateString(),
                eng->Force, self->brake395_pos);
    self->debugTime.prevTime = self->debugTime.currTime;
}
