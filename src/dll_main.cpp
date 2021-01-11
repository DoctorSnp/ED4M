/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

/**
  * Библиотека ЭД4М для симулятора RTrainSim
  * Версия 0.001
  * Дата Январь 2021
  */

#include "src/utils/utils.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>

//#define RTS_NODIRECTOBJLINKS

//#include "src/private_ED4M.h"
#include "src/elements.h"
#include "ED4M.h"

#pragma hdrstop
#pragma argsused

#define TR_CURRENT_C 272.0
#define TO_KM_PH (3.6)


struct st_Self SELF;

/**
 * @brief DllEntryPoint Стандартная функция подключения библиотеки
 * @param hinst
 * @param reason
 * @param lpReserved
 * @return
 */
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
    switch( reason )
        {
            case DLL_PROCESS_ATTACH:
             // Initialize once for each new process.
             // Return FALSE to fail DLL load.
                break;
            case DLL_THREAD_ATTACH:
             // Do thread-specific initialization.
                break;
            case DLL_THREAD_DETACH:
             // Do thread-specific cleanup.
                break;
            case DLL_PROCESS_DETACH:
             // Perform any necessary cleanup.
                break;
        }
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
    if (SELF.cabNum < 0)
        return;

    if( (loco->NumSlaves < 1) ) // это не голова!
        return;

    SELF.alsn[SELF.cabNum].isBackward = Backwards;
    SELF.alsn[SELF.cabNum].SpeedLimit.Distance = DistanceToNextLimit;
    SELF.alsn[SELF.cabNum].SpeedLimit.Limit = (float)(SpeedLimit * TO_KM_PH);
    SELF.alsn[SELF.cabNum].SpeedLimit.NextLimit = (float)(NextLimit * TO_KM_PH);
    SELF.alsn[SELF.cabNum].CurrSpeed = (float)( fabs(double(loco->Velocity)) * TO_KM_PH);

    /*загружаем информацию по сигналам */
    SELF.alsn[SELF.cabNum].NumSigForw = NumSigAhead;
    SELF.alsn[SELF.cabNum].NumSigBack = NumSigBack;
    SELF.alsn[SELF.cabNum].NumSigPassed = NumPassed;

    const SignalsInfo *sigPtr = NULL;
    UINT NumSigs = 0;
    if (SELF.cabNum == 1)
    {
        if (SELF.game.engPtr->Reverse >= 0)
        {
            sigPtr = sigAhead;
            NumSigs = NumSigAhead;
        }
        else
        {
            sigPtr = sigBack;
            NumSigs = NumSigBack;
        }
    }
    else
    {
        if (SELF.game.engPtr->Reverse < 0) // >=
        {
            sigPtr = sigBack;
            NumSigs = NumSigBack;
        }
        else
        {
            sigPtr = sigAhead;
            NumSigs = NumSigAhead;
        }
    }

    if (sigPtr)
    {
        SELF.alsn[SELF.cabNum].SpeedLimit.NextLimit = sigPtr[0].SpeedLimit;
        for (UINT i = 0; i< NumSigs; i++)
            SELF.alsn[SELF.cabNum].ForwardSignalsList[i] = sigPtr[i];
    }

   ED4M_ALSN(&SELF, loco );
}

/**
 * Фнукция работы библиотеки. Вызывается движком RTS постоянно
 */
extern "C" void Q_DECL_EXPORT Run
 (ElectricEngine *eng,const ElectricLocomotive *loco, unsigned long State,
        float time, float AirTemperature)
{
     SELF.tumblers[SELF.cabNum][Tumblers::Switch_Panto] = _checkSwitch(loco, Tumblers::Switch_Panto, -1, 1, 0);
     SELF.game.AirTemperature = AirTemperature;

     /**************** Переводим время игры в секунды и миллисекунды относительно начала времени игры */
     SELF.game.currTime.seconds = RTSGetInteger(loco, RTS_VAR_TIME , 1);
     SELF.game.isNight =  RTSGetInteger(loco, RTS_ISNIGHT, 1 );

     SELF.game.time = time;
     SELF.game.gameTimeBuffer += time;
     SELF.game.currTime.millis = (int (SELF.game.gameTimeBuffer * 100)) % 1000;

     SELF.game.State = State;
     SELF.game.locoPtr = loco;
     SELF.game.engPtr = eng;
     SELF.game.cabPtr = loco->cab;
     SELF.game.State = State;
     /***************************************/

     ED4M_step(&SELF);
}


extern "C" void Q_DECL_EXPORT  ChangeLoco
(Locomotive *loco,const Locomotive *Prev, unsigned long State)
{
  /*  if(Prev)
    {
        if(!Prev->Eng()->Reverse)
            if(!Prev->Cab()->Switch(132) ||
                    (Prev->Cab()->Switch(55)+Prev->Cab()->Switch(58)==2))
                loco->LocoFlags|=1;
    }
    else
        loco->LocoFlags|=1;*/

 loco->LocoFlags |=1;
 SELF.game.locoPtr = (ElectricLocomotive*)loco;
 SELF.game.cabPtr = loco->cab;
 SELF.game.engPtr = (ElectricEngine*)loco->Eng();

 ED4M_checkCab(&SELF);
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
    if (SELF.cabNum < 0)
        return true;

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
        if (SELF.arms[SELF.cabNum][Arms::Arm_Reverse] != REVERSE_NEUTRAL)
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
    if (SELF.cabNum < 1)
        return;

    /* Проверка кранов/рычагов, реверса и т.д. */
    if ( SwitchID == Arms::Arm_395)
        SELF.arms[SELF.cabNum][Arms::Arm_395] = _checkSwitch(loco, Arms::Arm_395, -1, 1, 2);
    else if (SwitchID == Arm_395_NM_Control)
        SELF.arms[SELF.cabNum][Arm_395_NM_Control] = _checkSwitch(loco, Arms::Arm_395_NM_Control, -1 , 1, 2);
    else if (SwitchID == Arm_395_TM_Control)
        SELF.arms[SELF.cabNum][Arm_395_TM_Control] = _checkSwitch(loco, Arms::Arm_395_TM_Control, -1 , 1, 2);

    else if  ( SwitchID ==  Arms::Arm_Reverse )
       SELF.arms[SELF.cabNum][Arm_Reverse] = _checkSwitch(loco,  Arms::Arm_Reverse, SoundsID::Revers, 1, 2);
    else if  ( SwitchID ==  Arms::Arm_Controller )
    {
       int ControllerPos = _checkSwitch(loco,  Arms::Arm_Controller, SoundsID::Controller, 1, 2);
       if (ControllerPos >= NEUTRAL_CONTROLLER_POS )
       {
           SELF.elecrto[SELF.cabNum].RecuperationPosition = 0;
           SELF.elecrto[SELF.cabNum].TyagaPosition = ControllerPos - NEUTRAL_CONTROLLER_POS;
       }
       else
       {
           SELF.elecrto[SELF.cabNum].TyagaPosition = 0;
           SELF.elecrto[SELF.cabNum].RecuperationPosition = NEUTRAL_CONTROLLER_POS - ControllerPos;
       }
    }

    /* проверка тумблеров */
    else if ( ( SwitchID >= Tmb_AvarEPT ) &&  ( SwitchID <= Tmb_vspomCompr  ))
       SELF.tumblers[SELF.cabNum][SwitchID] =  _checkSwitch(loco, SwitchID, Default_Tumbler, 1, 0);
    else if  ( SwitchID == Tmb_lightCab_Dimly)
       SELF.tumblers[SELF.cabNum][Tmb_lightCab_Dimly] = _checkSwitch(loco, Tmb_lightCab_Dimly, Default_Tumbler, 1, 0);
    else if ( SwitchID == Tumblers::Key_EPK )
        SELF.tumblers[SELF.cabNum][Tumblers::Key_EPK] = _checkSwitch(loco, Tumblers::Key_EPK, SoundsID::EPK_INIT, 1, 0);
    else if ( SwitchID == Tumblers::Switch_AutomatUpr )
        SELF.tumblers[SELF.cabNum][Tumblers::Switch_AutomatUpr] = _checkSwitch(loco, Tumblers::Switch_AutomatUpr, SoundsID::Avtomat, 1, 0);
    else if ( SwitchID == Tumblers::Switch_VU )
        SELF.tumblers[SELF.cabNum][Tumblers::Switch_VU] = _checkSwitch(loco, Tumblers::Switch_VU, SoundsID::VU, 1, 0);
    else if ( ( SwitchID == Tumblers::Tmb_leftDoors ) || ( SwitchID == Tumblers::Tmb_rightDoors ))
       SELF.tumblers[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);

    else if ( SwitchID == Tumblers::Switch_PitALSN_1 )
        SELF.tumblers[SELF.cabNum][Tumblers::Switch_PitALSN_1] = _checkSwitch(loco, Tumblers::Switch_PitALSN_1, SoundsID::Switch, 1, 0);
    else if ( SwitchID == Tumblers::Switch_PitALSN_2 )
        SELF.tumblers[SELF.cabNum][Tumblers::Switch_PitALSN_2] = _checkSwitch(loco, Tumblers::Switch_PitALSN_2, SoundsID::Switch, 1, 0);
    else if ( SwitchID == Tumblers::Switch_Panto)
        SELF.tumblers[SELF.cabNum][Tumblers::Switch_Panto] = _checkSwitch(loco, Tumblers::Switch_Panto, SoundsID::Switch, 1, 0);

    /*проверка тумблеров, пакетников и кнопок на задней панели*/

    /*если маленький тумблер (свитч) */
    else if (( SwitchID >= Switch_LightSalon) && ( SwitchID <= Switch_EPT) )
        SELF.tumblers[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID == Switch_Vent_I_Otoplenie) ||  ( SwitchID == Switch_ObogrevZerkal) || ( SwitchID == Switch_BufRight) )
        SELF.tumblers[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Switch_SvetPult ) &&  ( SwitchID <= Switch_XZ_2) )
        SELF.tumblers[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Switch_StekloobogrOkon_Lob) && ( SwitchID <= Switch_StekloobogrevOkon_Bok) )
        SELF.tumblers[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Switch_PitALSN_1 ) &&  ( SwitchID <= Switch_PitanieStekloobogrevMarshrut) )
        SELF.tumblers[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);

    /*если обычный тумблер или пакетник */
    else if (( SwitchID >= Switch_VspomCompr ) &&  ( SwitchID <= Tumbler_ObogrevCabIntensiv) )
        SELF.tumblers[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Switch, 1, 0);
    else if (( SwitchID >= Tmb_Tormozhenie ) &&  ( SwitchID <= Tmb_Dvornik_Pomoshnik) )
        SELF.tumblers[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);
    else if (( SwitchID == Packetnik_ObogrevCabDop ) ||  ( SwitchID == Packetnik_ObogrevMaslo) )
        SELF.tumblers[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, SoundsID::Default_Tumbler, 1, 0);


    /**************** проверка кнопок ***********************************************/
    else if ( ( SwitchID == Buttons::Btn_TifonMash ) ||  (SwitchID ==Buttons::Btn_Tifon2) )
      SELF.buttons[SELF.cabNum][SwitchID] = _checkSwitch(loco, Buttons::Btn_TifonMash, sounds::Tifon, 0, 0);
    else if ( ( SwitchID == Buttons::Btn_SvistokMash)  || (SwitchID ==Buttons::Btn_Svistok2) || (SwitchID ==Buttons::Btn_Svistok3 ))
      SELF.buttons[SELF.cabNum][SwitchID] = _checkSwitch(loco, Buttons::Btn_SvistokMash, sounds::Svistok, 0, 0);
    else if  ( SwitchID == Buttons::Btn_RB_MASH || SwitchID == Buttons::Btn_RB_D || SwitchID == Buttons::Btn_RB_POM)
       SELF.buttons[SELF.cabNum][SwitchID] = _checkSwitch(loco, SwitchID, sounds::RB, 0, 0);
    else if (SwitchID == Tumblers::KLUB_enable_input )
        SELF.KLUB[SELF.cabNum].canReadInput = _checkSwitch(loco, SwitchID, -1, 1, 0);
    else if ( (SwitchID >= Tumblers::KLUB_0) && ( SwitchID <= Tumblers::KLUB_CMD_K) )
       SELF.KLUB[SELF.cabNum].inputKey = SwitchID;
    /**********************************************************************************/
}



extern "C" void Q_DECL_EXPORT SpeedLimit(const Locomotive *loco,
        SpeedLimitDescr Route,SpeedLimitDescr Signal,SpeedLimitDescr Event)
{
    if (SELF.cabNum < 0)
        return;
}
