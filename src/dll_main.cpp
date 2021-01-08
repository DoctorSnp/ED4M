/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

/**
  * Библиотека ЭД4М для симулятора RTrainSim
  * Версия 0.001 (разработка)
  * Дата Январь 2021
  */

#include <windows.h>
#include <math.h>
#include <stdio.h>

/*#define RTS_NODIRECTOBJLINKS*/
#include "src/utils/utils.h"
#include "src/private_ED4M.h"
#include "src/elements.h"
#include "ED4M.h"

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

/**
 * Инициализация локомотива.
 * Возвращает true в случае успеха и false в случае ошибки.
 */
extern "C" bool Q_DECL_EXPORT Init
(ElectricEngine *eng, ElectricLocomotive *loco,
 unsigned long State, float time, float AirTemperature)
{
    SELF.game.time = time;
    SELF.game.AirTemperature = AirTemperature;
    SELF.game.State = State;
    if ( ED4M_init(&SELF, loco, eng ) == -1)
        return false;
    return true;
}

/**
 * Работа АЛСН. Вызывается движком RTS постоянно , если в eng файле указано SignalizationScripted (1)
 */
extern "C" void Q_DECL_EXPORT ALSN ( Locomotive *loco, SignalsInfo *sigAhead, UINT NumSigAhead,
    SignalsInfo *sigBack,UINT NumSigBack, SignalsInfo *sigPassed, UINT NumPassed,
        float SpeedLimit, float NextLimit,
        float DistanceToNextLimit, bool Backwards )
{

    if( (loco->NumSlaves < 1) ) // это не голова!
        return;

    SELF.alsn.isBackward = Backwards;
    SELF.alsn.SpeedLimit.Distance = DistanceToNextLimit;
    SELF.alsn.SpeedLimit.Limit = (float)(SpeedLimit * TO_KM_PH);
    SELF.alsn.SpeedLimit.NextLimit = (float)(NextLimit * TO_KM_PH);
    SELF.alsn.CurrSpeed = (float)( fabs(double(loco->Velocity)) * TO_KM_PH);

    /*загружаем информацию по сигналам */
    SELF.alsn.NumSigForw = NumSigAhead;
    SELF.alsn.NumSigBack = NumSigBack;
    SELF.alsn.NumSigPassed = NumPassed;

    if (sigAhead)
    {
        SELF.alsn.SpeedLimit.NextLimit = sigAhead[0].SpeedLimit;
        for (UINT i=0; i< NumSigAhead; i++)
        {
            SELF.alsn.ForwardSignalsList[i] = sigAhead[i];
        }
    }
    if (sigBack)
        SELF.alsn.signListBack =  *sigBack;

    ED4M_ALSN(&SELF, loco );
}

/**
 * Фнукция работы библиотеки. Вызывается движком RTS постоянно
 */
extern "C" void Q_DECL_EXPORT Run
 (ElectricEngine *eng,const ElectricLocomotive *loco, unsigned long State,
        float time, float AirTemperature)
{
     SELF.tumblersArray[Tumblers::Switch_Panto] = _checkSwitch(loco, Tumblers::Switch_Panto, -1, 1, 0);
     SELF.game.AirTemperature = AirTemperature;

     /**************** Переводим время игры в секунды и миллисекунды относительно начала времени игры */
     SELF.game.currTIme.seconds = RTSGetInteger(loco, RTS_VAR_TIME , 1);
     SELF.game.time = time;
     SELF.game.gameTimeBuffer += time;
     SELF.game.currTIme.millis = (int (SELF.game.gameTimeBuffer * 100)) % 1000;

     SELF.game.locoPtr = loco;
     SELF.game.engPtr = eng;
     SELF.game.State = State;
    /***************************************/


    // Printer_print(SELF.game.engPtr, GMM_POST, L"Millis %d\n", SELF.game.milliseconds );
     ED4M_Step(&SELF);
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
    Engine *eng=loco->Eng();
    UINT &AsyncFlags=*(UINT *)&eng->var[4];
    AsyncFlags&=~14016;
    AsyncFlags&=~(2<<16);

    if(eng->ThrottlePosition>36)
        eng->ThrottlePosition-=9;
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
    if(SwitchID == Arm_Reverse)
    {
        if ( (loco->Velocity > 0.01) || (loco->Eng()->Force > 0.01))
            return false;
        if (eng->ThrottlePosition > 1)
            return false;
        return true;
    }
    if (SwitchID == Arm_Controller)
    {
        if (SELF.Reverse != REVERSE_NEUTRAL)
            return true;
        return false;
    }
    return true;
}


/**
 * Реакция на переключение элементов
 * loco - Указатель на объект подвижного состава (ПС).
 * loco - Указатель на объект ходовой части ПС.
 * SwitchID - Элемент, состояние которого изменилось
 */
extern "C" void Q_DECL_EXPORT Switched(const ElectricLocomotive *loco, ElectricEngine *eng,
        unsigned int SwitchID, unsigned int PrevState)
{

    /* Проверка кранов/рычагов, реверса и т.д. */
    if ( SwitchID == Arms::Arm_395)
        SELF.pneumo.Arm_395 = _checkSwitch(loco, Arms::Arm_395, -1, 1, 2);
    else if (SwitchID == Arm_395_NM_Control)
        SELF.armsArray[Arm_395_NM_Control] = _checkSwitch(loco, Arms::Arm_395_NM_Control, -1 , 1, 2);
    else if (SwitchID == Arm_395_TM_Control)
        SELF.armsArray[Arm_395_TM_Control] = _checkSwitch(loco, Arms::Arm_395_TM_Control, -1 , 1, 2);

    else if  ( SwitchID ==  Arms::Arm_Reverse )
       SELF.Reverse = _checkSwitch(loco,  Arms::Arm_Reverse, SoundsID::Revers, 1, 2);
    else if  ( SwitchID ==  Arms::Arm_Controller )
    {
       int ControllerPos = _checkSwitch(loco,  Arms::Arm_Controller, SoundsID::Controller, 1, 2);
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
       SELF.tumblersArray[SwitchID] =  _checkSwitch(loco, SwitchID, Default_Tumbler, 1, 0);
    else if  ( SwitchID == Tmb_lightCab_Dimly)
       SELF.tumblersArray[Tmb_lightCab_Dimly] = _checkSwitch(loco, Tmb_lightCab_Dimly, Default_Tumbler, 1, 0);
    else if ( SwitchID == Tumblers::Key_EPK )
        SELF.tumblersArray[Tumblers::Key_EPK] = _checkSwitch(loco, Tumblers::Key_EPK, SoundsID::EPK_INIT, 1, 0);

    else if ( SwitchID == Tumblers::Switch_AutomatUpr )
        SELF.tumblersArray[Tumblers::Switch_AutomatUpr] = _checkSwitch(loco, Tumblers::Switch_AutomatUpr, SoundsID::Avtomat, 1, 0);
    else if ( SwitchID == Tumblers::Switch_VU )
        SELF.tumblersArray[Tumblers::Switch_VU] = _checkSwitch(loco, Tumblers::Switch_VU, SoundsID::VU, 1, 0);

    else if ( ( SwitchID == Tumblers::Tmb_leftDoors ) || ( SwitchID == Tumblers::Tmb_rightDoors ))
       SELF.tempFlags[SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);

    else if ( SwitchID == Tumblers::Switch_PitALSN_1 )
        SELF.tumblersArray[Tumblers::Switch_PitALSN_1] = _checkSwitch(loco, Tumblers::Switch_PitALSN_1, SoundsID::Switch, 1, 0);
    else if ( SwitchID == Tumblers::Switch_PitALSN_2 )
        SELF.tumblersArray[Tumblers::Switch_PitALSN_2] = _checkSwitch(loco, Tumblers::Switch_PitALSN_2, SoundsID::Switch, 1, 0);
    else if ( SwitchID == Tumblers::Switch_Panto)
        SELF.tumblersArray[Tumblers::Switch_Panto] = _checkSwitch(loco, Tumblers::Switch_Panto, SoundsID::Switch, 1, 0);

    /*проверка тумблеров, пакетников и кнопок на задней панели*/

    /*если маленький тумблер (свитч) */
    else if (( SwitchID >= Switch_LightSalon) && ( SwitchID <= Switch_EPT) )
        SELF.tumblersArray[SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID == Switch_Vent_I_Otoplenie) ||  ( SwitchID == Switch_ObogrevZerkal) || ( SwitchID == Switch_BufRight) )
        SELF.tumblersArray[SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Switch_SvetPult ) &&  ( SwitchID <= Switch_XZ_2) )
        SELF.tumblersArray[SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Switch_StekloobogrOkon_Lob) && ( SwitchID <= Switch_StekloobogrevOkon_Bok) )
        SELF.tumblersArray[SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Switch_PitALSN_1 ) &&  ( SwitchID <= Switch_PitanieStekloobogrevMarshrut) )
        SELF.tumblersArray[SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);

    /*если обычный тумблер или пакетник */
    else if (( SwitchID >= Tumbler_VspomCompressos ) &&  ( SwitchID <= Tumbler_ObogrevCabIntensiv) )
        SELF.tumblersArray[SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);
    else if (( SwitchID >= Tmb_Tormozhenie ) &&  ( SwitchID <= Tmb_Dvornik_Pomoshnik) )
        SELF.tumblersArray[SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);
    else if (( SwitchID == Packetnik_ObogrevCabDop ) ||  ( SwitchID == Packetnik_ObogrevMaslo) )
        SELF.tumblersArray[SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);


    /**************** проверка кнопок ***********************************************/

    else if ( ( SwitchID == Buttons::Btn_TifonMash ) ||  (SwitchID ==Buttons::Btn_Tifon2) )
      SELF.buttonsArray[SwitchID] = _checkSwitch(loco, Buttons::Btn_TifonMash, sounds::Tifon, 0, 0);
    else if ( ( SwitchID == Buttons::Btn_SvistokMash)  || (SwitchID ==Buttons::Btn_Svistok2) || (SwitchID ==Buttons::Btn_Svistok3 ))
      SELF.buttonsArray[SwitchID] = _checkSwitch(loco, Buttons::Btn_SvistokMash, sounds::Svistok, 0, 0);

    else if  ( SwitchID == Buttons::Btn_RB )
       SELF.buttonsArray[Buttons::Btn_RB] = _checkSwitch(loco, Buttons::Btn_RB, sounds::RB, 0, 0);
    else if  ( SwitchID == Buttons::Btn_RB_D )
       SELF.buttonsArray[Buttons::Btn_RB_D] = _checkSwitch(loco, Buttons::Btn_RB_D, sounds::RB, 0, 0);

    else if (SwitchID == Tumblers::KLUB_enable_input )
        SELF.KLUB[SELF.cabNum -1].canReadInput = _checkSwitch(loco, SwitchID, -1, 1, 0);
    else if ( (SwitchID >= Tumblers::KLUB_0) && ( SwitchID <= Tumblers::KLUB_CMD_K) )
       SELF.KLUB[SELF.cabNum -1].inputKey = SwitchID;

    /**********************************************************************************/
}



extern "C" void Q_DECL_EXPORT SpeedLimit(const Locomotive *loco,
        SpeedLimitDescr Route,SpeedLimitDescr Signal,SpeedLimitDescr Event)
{

    //Printer_print(loco->Eng(), GMM_POST, L"Speed Limit!\n");
    //SELF.sautData.SpeedLimit =  Event;
}
