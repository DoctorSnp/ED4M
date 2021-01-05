
#include <windows.h>
#include <math.h>
#include <stdio.h>

#include <cmath>

/*#define RTS_NODIRECTOBJLINKS*/
#include "src/utils/utils.h"
#include "src/private_ED4M.h"
#include "src/ED4M_datatypes/cab/section1/elements.h"
#include "../ED4M.h"

#include "ED4M.h"

#define MAX_LOCOS 12 // макс. кол-во локов - с учётом перспективы на МВПС

#pragma hdrstop
#pragma argsused

#define TR_CURRENT_C 272.0
#define TO_KM_PH (3.6)

static struct st_Self SELF;

struct st_Selfservice
{
    int state;
};

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
    return 1;
}

/****************************
 * Открытые функции библиотеки
 *******************************/

/**
 * Инициализация локомотива.
 * Возвращает true в случае успеха и false в случае ошибки.
 */
extern "C" bool Q_DECL_EXPORT Init
 (ElectricEngine *eng,ElectricLocomotive *loco,unsigned long State,
        float time,float AirTemperature)
{
    if ( ED4M_init(&SELF, loco, eng ) == false)
        return 0;


/*
 eng->var[0]=0;
 eng->var[1]=0.0;
 eng->var[2]=0.0;
 eng->var[4]=0.0;
 eng->var[5]=0.0;
 eng->var[6]=0.0;
 eng->var[7]=0.0;
 eng->var[8]=0.0;
 eng->var[9]=0.0;
 eng->var[10]=0.0;
 eng->var[11]=0.0;
 eng->var[12]=0.0;
 eng->var[13]=0.0;
 eng->var[14]=25.0;
 eng->var[15]=0.0;
 eng->var[16]=0.0;
 eng->var[17]=0.0;*/

 SELF.service.LocoState = 0;
 Cabin *cab=eng->cab;
 UINT *Flags=(UINT *)&eng->var[0];

 /*
 switch(State&0xFF)
 {
 case 0:
     SELF.service.LocoState = 0;

     if((State>>8)&1){
      loco->LocoFlags|=1;
     };
     loco->MainResPressure=7.0;
     loco->TrainPipePressure=5.0;
     loco->AuxiliaryPressure=5.2;
     eng->UR=4.0;
     eng->Power=0;
     eng->Force=0;
     loco->BrakeCylinderPressure=0.0;
     eng->IndependentBrakeValue=4.0;
     eng->TrainPipeRate=0.0;
     eng->ThrottlePosition=0;
     eng->Reverse = REVERSE_NEUTRAL;
     eng->ALSNOn=0;
     eng->MainSwitch=0;
     eng->Panto=0;
     *Flags|=32<<8;

     break;
 case 1:
     SELF.service.LocoState = 1;
     eng->UR=5.5;
     loco->IndependentBrakePressure=0.0;
     loco->TrainPipePressure=0.0;
     loco->AuxiliaryPressure=0.0;
     loco->BrakeCylinderPressure=0.0;
     loco->MainResPressure=0.0;
     eng->IndependentBrakeValue=0.0;
     break;
 }*/

 SELF.dest = -1;
 return 1;
}


extern "C" void Q_DECL_EXPORT ALSN ( Locomotive *loco, SignalsInfo *sigAhead, UINT NumSigAhead,
    SignalsInfo *sigBack,UINT NumSigBack, SignalsInfo *sigPassed, UINT NumPassed,
        float SpeedLimit, float NextLimit,
        float DistanceToNextLimit, bool Backwards )
{

    if( (loco->NumSlaves < 1) ) // это не голова! // || (loco->LocoFlags&1) )
        return;

    SELF.alsn.SpeedLimit.Distance = DistanceToNextLimit;
    SELF.alsn.SpeedLimit.Limit = (float)(SpeedLimit * TO_KM_PH);
    SELF.alsn.SpeedLimit.NextLimit = (float)(NextLimit * TO_KM_PH);
    SELF.alsn.CurrSpeed = (float)( fabs(double(loco->Velocity)) * TO_KM_PH);

    SELF.isBackward = (int)Backwards;

    /*загружаем информацию по сигналам */
    SELF.alsn.NumSigForw = NumSigAhead;
    SELF.alsn.NumSigBack = NumSigBack;
    SELF.alsn.NumSigPassed = NumPassed;

    if (sigAhead)
    {
        swprintf(SELF.alsn.signalName, sizeof(SELF.alsn.signalName),  L"%s", sigAhead->SignalInfo->Name);
        SELF.alsn.SpeedLimit.NextLimit = sigAhead[0].SpeedLimit;
        for (UINT i=0; i< NumSigAhead; i++)
        {
            SELF.alsn.ForwardSignalsList[i] = sigAhead[i];
        }
    }
    else
        swprintf(SELF.alsn.signalName, sizeof(SELF.alsn.signalName),  L"UNSET");
    if (sigBack)
        SELF.alsn.signListBack =  *sigBack;

    //Printer_print(loco->Eng(), GMM_POST, L"Distance: %f\n", DistanceToNextLimit );
    ED4M_ALSN(&SELF, loco );
}


extern "C" void Q_DECL_EXPORT Run
 (ElectricEngine *eng,const ElectricLocomotive *loco, unsigned long State,
        float time,float AirTemperature)
{
     SELF.tumblersArray[Tumblers::Switch_Panto] = _checkSwitchWithSound(loco, Tumblers::Switch_Panto, -1, 1, 0);

     ED4M_Step(&SELF, loco, eng, time);
}


extern "C" void Q_DECL_EXPORT  ChangeLoco
(Locomotive *loco,const Locomotive *Prev, unsigned long State)
{
 if(Prev)
 {
  if(!Prev->Eng()->Reverse)
   if(!Prev->Cab()->Switch(132) ||
        (Prev->Cab()->Switch(55)+Prev->Cab()->Switch(58)==2))
   loco->LocoFlags|=1;
 }else
  loco->LocoFlags|=1;
}

extern "C" void  Q_DECL_EXPORT  LostMaster
(Locomotive *loco,const Locomotive *Prev, unsigned long State)
{
 UINT &Flags=*(UINT *)&loco->locoA->var[0];
 Engine *eng=loco->Eng();
 UINT &AsyncFlags=*(UINT *)&eng->var[4];
 AsyncFlags&=~14016;
 AsyncFlags&=~(2<<16);
 //eng->var[11]=0.0;
 if(eng->ThrottlePosition>36)
  eng->ThrottlePosition-=9;
 //Flags&=~1272;
}

extern "C" bool Q_DECL_EXPORT CanWorkWith(const Locomotive *loco, const wchar_t *Type)
{
 //if(!lstrcmpiW(Type,L"ed4m"))
    return true;
 //return false;
}


extern "C" bool Q_DECL_EXPORT  CanSwitch(const ElectricLocomotive *loco, const ElectricEngine *eng,
        unsigned int SwitchID, unsigned int SetState)
{

 if(SwitchID==Arm_Reverse)
 {
    if ( (loco->Velocity > 0.01) || (loco->Eng()->Force > 0.01))
        return false;
    if (eng->ThrottlePosition > 1)
        return false;
    return true;
 }
 if (SwitchID==Arm_Controller)
 {
     if (SELF.Reverse != REVERSE_NEUTRAL)
         return true;
     return false;
 }
 return true;
}


/**
 * Реакция на переключение элементов
 */
extern "C" void Q_DECL_EXPORT Switched(const ElectricLocomotive *loco,ElectricEngine *eng,
        unsigned int SwitchID,unsigned int PrevState)
{

    /* Проверка кранов/рычагов, реверса и т.д. */
    if ( SwitchID == Arms::Arm_395)
        SELF.pneumo.Arm_395 = _checkSwitchWithSound(loco, Arms::Arm_395, SoundsID::Kran_395, 1, 2);
    else if  ( SwitchID ==  Arms::Arm_Reverse )
       SELF.Reverse = _checkSwitchWithSound(loco,  Arms::Arm_Reverse, SoundsID::Revers, 1, 2);
    else if  ( SwitchID ==  Arms::Arm_Controller )
    {
       int ControllerPos = _checkSwitchWithSound(loco,  Arms::Arm_Controller, SoundsID::Controller, 1, 2);
       if (ControllerPos >= NEUTRAL_CONTROLLER_POS )
       {
           SELF.RecuperationPosition = 0;
           SELF.TyagaPosition = ControllerPos - NEUTRAL_CONTROLLER_POS;
       }
       else
       {
           SELF.TyagaPosition = 0;
           SELF.RecuperationPosition = NEUTRAL_CONTROLLER_POS - ControllerPos;
       }
    }

    /* проверка тумблеров */
    else if ( ( SwitchID >= Tmb_AvarEPT ) &&  ( SwitchID <= Tmb_vspomKompr  ))
       SELF.tumblersArray[SwitchID] =  _checkSwitchWithSound(loco, SwitchID, Default_Tumbler, 1, 0);
    else if  ( SwitchID == Tmb_lightCab_Dimly)
       SELF.tumblersArray[Tmb_lightCab_Dimly] = _checkSwitchWithSound(loco, Tmb_lightCab_Dimly, Default_Tumbler, 1, 0);
    else if ( SwitchID == Tumblers::Key_EPK )
        SELF.tumblersArray[Tumblers::Key_EPK] = _checkSwitchWithSound(loco, Tumblers::Key_EPK, SoundsID::EPK_INIT, 1, 0);

    else if ( SwitchID == Tumblers::Tmb_AutomatUpr )
        SELF.tumblersArray[Tumblers::Tmb_AutomatUpr] = _checkSwitchWithSound(loco, Tumblers::Tmb_AutomatUpr, SoundsID::Avtomat, 1, 0);
    else if ( SwitchID == Tumblers::Tmb_VU )
        SELF.tumblersArray[Tumblers::Tmb_VU] = _checkSwitchWithSound(loco, Tumblers::Tmb_VU, SoundsID::VU, 1, 0);

    else if ( SwitchID == Tumblers::Switch_PitALSN_1 )
        SELF.tumblersArray[Tumblers::Switch_PitALSN_1] = _checkSwitchWithSound(loco, Tumblers::Switch_PitALSN_1, SoundsID::Switch, 1, 0);
    else if ( SwitchID == Tumblers::Switch_PitALSN_2 )
        SELF.tumblersArray[Tumblers::Switch_PitALSN_2] = _checkSwitchWithSound(loco, Tumblers::Switch_PitALSN_2, SoundsID::Switch, 1, 0);
    else if ( SwitchID == Tumblers::Switch_Panto)
        SELF.tumblersArray[Tumblers::Switch_Panto] = _checkSwitchWithSound(loco, Tumblers::Switch_Panto, SoundsID::Switch, 1, 0);

    /*проверка тумблеров, пакетников и кнопок на задней панели*/

    /*если маленький тумблер (свитч) */
    else if (( SwitchID >= Switch_LightSalon) && ( SwitchID <= Switch_EPT) )
        SELF.tumblersArray[SwitchID] = _checkSwitchWithSound(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID == Switch_Vent_I_Otoplenie) ||  ( SwitchID == Switch_ObogrevZerkal) || ( SwitchID == Switch_BufRight) )
        SELF.tumblersArray[SwitchID] = _checkSwitchWithSound(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Switch_SvetPult ) &&  ( SwitchID <= Switch_XZ_2) )
        SELF.tumblersArray[SwitchID] = _checkSwitchWithSound(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Switch_StekloobogrOkon_Lob) && ( SwitchID <= Switch_StekloobogrevOkon_Bok) )
        SELF.tumblersArray[SwitchID] = _checkSwitchWithSound(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Switch_PitALSN_1 ) &&  ( SwitchID <= Switch_PitanieStekloobogrevMarshrut) )
        SELF.tumblersArray[SwitchID] = _checkSwitchWithSound(loco, SwitchID, SoundsID::Switch, 1, 0);

    /*если обычный тумблер или пакетник */
    else if (( SwitchID >= Tumbler_VspomCompressos ) &&  ( SwitchID <= Tumbler_ObogrevCabIntensiv) )
        SELF.tumblersArray[SwitchID] = _checkSwitchWithSound(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);
    else if (( SwitchID >= Tmb_Tormozhenie ) &&  ( SwitchID <= Tmb_Dvornik_Pomoshnik) )
        SELF.tumblersArray[SwitchID] = _checkSwitchWithSound(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);
    else if (( SwitchID == Packetnik_ObogrevCabDop ) ||  ( SwitchID == Packetnik_ObogrevMaslo) )
        SELF.tumblersArray[SwitchID] = _checkSwitchWithSound(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);


    /***************************************************************/
    /*проверка кнопок */
    else if ( ( SwitchID ==Buttons::Btn_TifonMash ) ||  (SwitchID ==Buttons::Btn_Tifon2) )
        _checkSwitchWithSound(loco, Buttons::Btn_TifonMash, sounds::Tifon, 0, 0);
    else if ( ( SwitchID ==Buttons::Btn_SvistokMash)  || (SwitchID ==Buttons::Btn_Svistok2) || (SwitchID ==Buttons::Btn_Svistok3 ))
        _checkSwitchWithSound(loco, Buttons::Btn_SvistokMash, sounds::Svistok, 0, 0);
    else if  ( SwitchID ==Buttons::Btn_RB )
        _checkSwitchWithSound(loco, Buttons::Btn_RB, sounds::RB, 0, 0);
    else if  ( SwitchID ==Buttons::Btn_RB_D )
        _checkSwitchWithSound(loco, Buttons::Btn_RB_D, sounds::RB, 0, 0);
}



extern "C" void Q_DECL_EXPORT SpeedLimit(const Locomotive *loco,
        SpeedLimitDescr Route,SpeedLimitDescr Signal,SpeedLimitDescr Event)
{

    //Printer_print(loco->Eng(), GMM_POST, L"Speed Limit!\n");
    //SELF.sautData.SpeedLimit =  Event;
}
