/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#include <math.h>
#include <stdio.h>
#include <string>
#include <stdio.h>
#include <wchar.h>
#include "klub.h"
#include "src/elements.h"
#include "src/shared_structs.h"
#include "src/utils/utils.h"

#define KLUB_UNSET 0
#define KLUB_POEZDNOE 1
#define KLUB_MANEVR 2
#define KLUB_DOUBLE_TYAGA 3

constexpr int Limit_Yellow = 60;
constexpr int Limit_YellowYellow = 40;
//constexpr int Limit_YellowGreen = 80;
constexpr int START_PADENIE = 600; // за сколко метров активировать точку падения
constexpr int SBOY_KODOV = 40;

#define TO_MEGA_PH 10.197162

/*Звуки КЛУБа*/
enum KLUB_sounds
{
    KLUB_Beep = 1401,
    KLUB_Pick = 1402,
    KLUB_Beep_error = 1403,
    KLUB_Beep_loop = 1404,
};

/**** Логические функции ****/
static int m_speedLimit(st_KLUB* _KLUB, st_ALSN &alsn);


/*** Отображение показателей ***/
static void m_display(st_KLUB* _KLUB, st_ALSN &alsn, Engine *eng);
static void m_displaySpeed(st_KLUB* _KLUB);
static void m_displayReg(st_KLUB* _KLUB);
static void m_displayTime(st_KLUB* _KLUB);
static void m_displaySignals(st_KLUB* _KLUB, st_ALSN &alsn);
static void m_displayPressure(st_KLUB* _KLUB);
static void m_displayCurrentMode(st_KLUB *_KLUB, st_ALSN &alsn);


/*** Ввод и обработка команд ***/
static void _checkInput(st_KLUB* _KLUB);
static void _checkInputedCmd(st_KLUB* _KLUB);
static int _isCmdWithAsk(int cmd, int answ);
static void _addToCmd(st_KLUB* _KLUB, int cifer);
static int _execCmd(st_KLUB *_KLUB);
static void m_debug(st_KLUB *_KLUB, st_ALSN &alsn);


/**************************/

int KLUB_init(st_KLUB* _KLUB)
{
    memset(_KLUB, 0, sizeof (st_KLUB));
    _KLUB->__prevTime  = {};
    _KLUB->cmdForExec[0] = 0;
    _KLUB->cmdForExec[1] = 0;
    _KLUB->cmdForExec[2] = 0;
    _KLUB->cmdForExec[3] = 0;
    _KLUB->whiteSpeed = 60;
    _KLUB->prevSignCode = -1;
    _KLUB->isMegaPaskali = true;
    return 0;
}


void KLUB_Step(st_KLUB* _KLUB, Engine *eng, st_ALSN& alsn,  const Locomotive *loco)
{
    if ( _KLUB->currTime.seconds < _KLUB->__prevTime.seconds + 1 )
        if ( _KLUB->currTime.millis  < _KLUB->__prevTime.millis + 500 )
            return;

    _KLUB->locoPtr = loco;
    _KLUB->cabPtr = loco->cab;
    _KLUB->enginePtr = eng;

    m_display(_KLUB, alsn, _KLUB->enginePtr);
    _KLUB->__prevTime.seconds = _KLUB->currTime.seconds;
    _KLUB->__prevTime.millis = _KLUB->currTime.millis;

    m_debug(_KLUB, alsn);
    if (_KLUB->isOn < 2)
        return;

    _checkInput( _KLUB);
}

static int m_speedLimit(st_KLUB* _KLUB, st_ALSN &alsn)
{
    int limit = (int)alsn.SpeedLimit.Limit;
    int currLimit = limit;
    int padenie  = 0;

    if (IS_WHITE(alsn.correctALSNCode )) // Сбой кодов!
        return SBOY_KODOV;
    if (IS_GREEN(alsn.correctALSNCode))
    {
        // сюда нужно добавить логику на местные ограничения скорости.
        return currLimit;
    }

    if (alsn.correctALSNCode == SIGASP_APPROACH_1)
        padenie = Limit_Yellow;
    else if (alsn.correctALSNCode == SIGASP_APPROACH_2 || alsn.correctALSNCode == SIGASP_APPROACH_3)
        padenie = Limit_YellowYellow;
    else if (alsn.correctALSNCode == SIGASP_STOP_AND_PROCEED)
        padenie = 5;
    else
        padenie = 0;

    if (padenie && (_KLUB->distanceToCell <= START_PADENIE))
    {
        int mustDown = currLimit - padenie; // на сколько должен упасть лимит за START_PADENIE метров.
        int currDown = ( (START_PADENIE - _KLUB->distanceToCell) / 50) * (mustDown/ 12) ;

       limit  = currLimit - currDown ;
       if ( (limit < padenie) || (_KLUB->distanceToCell <= 50))
            limit = padenie;
    }
    return limit;
}

/**
 * @brief m_display Отображение Блока индикации
 * @param _KLUB
 * @param alsn
 * @param eng
 */
static void m_display(st_KLUB* _KLUB, st_ALSN& alsn, Engine *eng)
{

    _KLUB->currSpeed = (int)alsn.CurrSpeed;

    m_displayReg(_KLUB);
    m_displayTime(_KLUB);
    m_displayPressure(_KLUB);
    m_displaySpeed(_KLUB);

    wchar_t currKilometer[32];
    swprintf(currKilometer, L"%4.3f", eng->CurrentMilepost);
    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_KM, 0, currKilometer);
    m_displayCurrentMode(_KLUB, alsn);
    m_displaySignals(_KLUB, alsn);
}

/**
 * @brief m_displaySpeed Отображение текущей и максимальной скоростей
 * @param _KLUB
 */
static void m_displaySpeed(st_KLUB* _KLUB)
{

    wchar_t currSpeed[32];
    wchar_t speedLimit[32];
    swprintf(speedLimit ,L"");
    swprintf(currSpeed, L"%03d", _KLUB->currSpeed);

    if (_KLUB->mode == KLUB_UNSET)
        _KLUB->mode = KLUB_POEZDNOE;

    static bool mustBlink = false;   // нужно ли мигать
    static bool mustPip = false;     // нужно ли пищать
    if ( (_KLUB->currSpeed + 3 >= _KLUB->speedLimit) || (_KLUB->isOn == 1))
    {
        mustBlink = !mustBlink;
        if ((_KLUB->currSpeed + 1) >= _KLUB->speedLimit)
        {
             if (!mustPip)
             {
                mustPip = true;
                _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Beep_loop);
             }
        }
        else
        {
            if (mustPip)
            {
                mustPip = false;
                _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Beep_loop + 1);
                _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Beep_error);
            }
        }
    }
    else
        mustBlink = false;


    if (mustBlink)
            swprintf(currSpeed, L"");

    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Speed1, _KLUB->isOn );
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Time, _KLUB->isOn);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_SpeedGreenCircle, _KLUB->isOn);
    /****    Если включен ЭПК     ****/
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_KM, _KLUB->isOn >= 1);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RassDoCeli, _KLUB->isOn >= 2);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_SigName, _KLUB->isOn >= 2);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Station, _KLUB->isOn >= 2);
    /*********************************/


    if (_KLUB->isOn > 1)
        swprintf(speedLimit ,L"%03d", _KLUB->speedLimit);

    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_Speed1, 0, currSpeed);
    if (!_KLUB->currSpeed )
        _KLUB->cabPtr->SetScreenValue(Sensors::Sns_KLUB_SpeedGreenCircle, 0 , 1);
    _KLUB->cabPtr->SetScreenValue(Sensors::Sns_KLUB_SpeedGreenCircle, 0 , _KLUB->currSpeed);
    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_Speed1, 1, speedLimit);

}

/**
 * @brief m_displayReg Показывает состояние активности регистрации
 * @param _KLUB
 * @param isDisplay
 */
static void m_displayReg(st_KLUB* _KLUB)
{
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Kasseta, _KLUB->isOn );

    //_KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP, 1);
    /*_KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_41, 1);
     _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_42, 1);
     _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_43, 1);
     _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_44, 1);
     _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_45, 1);
     _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_46, 1);
    _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_47, 1);
    _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_48, 1);
    _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_49, 1);
    _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_51, 1);
   _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_64_LAMP_50, 1);*/
  //  _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_SOME_LAMP_52, 1);
  // _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BILL_SOME_LAMP_55, 1);

}

/**
 * @brief _displayTime Показывает или скрывает время КЛУБа
 * @param _KLUB Указатель на структуру КЛУБа
 * @param isDisplay 1 показывать время, 0 - не показывать
 */
static void m_displayTime(st_KLUB* _KLUB)
{
    if (_KLUB->isOn == 0)
    {
        _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_Time, 0, 0);
        return;
    }

    if (_KLUB->cabPtr->ScreenState(Sensors::Sns_KLUB_Time, 0) < 1)
        _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_Time, 0, 1);

    int sec  = _KLUB->currTime.seconds % 60;
    int min  = (_KLUB->currTime.seconds / 60) % 60;
    int hour = (_KLUB->currTime.seconds / 3600) % 24;

    wchar_t tempText[12];
    swprintf(tempText ,L"%02d.%02d.%02d", hour, min, sec);
    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_Time, 0, tempText);
}

/**
 * @brief _setBillColour Показывает цвет светофора
 * @param _KLUB Указатель на структуру КЛУБа
 */
static void m_displaySignals(st_KLUB* _KLUB, st_ALSN &alsn)
{
    if (_KLUB->mode == KLUB_POEZDNOE)
    {
        if (alsn.correctALSNCode != _KLUB->prevSignCode)
        {
            _KLUB->prevSignCode = alsn.correctALSNCode;
            _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Beep);
        }
    }
    else
    {
        if (_KLUB->prevSignCode != SIGASP_RESTRICTING)
        {
            _KLUB->prevSignCode = SIGASP_RESTRICTING;
            _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Beep);
        }
        alsn.correctALSNCode = SIGASP_RESTRICTING;
    }

    if ( _KLUB->isOn >= 2  )
    {
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green4, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green3, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green2, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green1, IS_GREEN(alsn.correctALSNCode));
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Yellow, IS_YELLOW(alsn.correctALSNCode));
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RedYellow, IS_KG(alsn.correctALSNCode));
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Red, IS_RED(alsn.correctALSNCode));
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_White, IS_WHITE(alsn.correctALSNCode));
    }
    else
    {
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green4, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green3, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green2, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green1, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Yellow, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RedYellow, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Red, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_White, 0);
    }
}

/**
 * @brief m_displayPressure Показывает Давление
 * @param _KLUB  Указатель на структуру КЛУБа
 * @param isDisplay
 */
static void  m_displayPressure(st_KLUB* _KLUB)
{
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_TM, _KLUB->isOn);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_UR, _KLUB->isOn);
    if (_KLUB->isOn == 0)
            return;

    if (_KLUB->cabPtr->ScreenState(Sensors::Sns_KLUB_TM, 0) < 1)
        _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_TM, 0, 1);
    if (_KLUB->cabPtr->ScreenState(Sensors::Sns_KLUB_UR, 0) < 1)
        _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_UR, 0, 1);

    wchar_t davlTM [32];
    wchar_t davlUR [32];
    if  (!_KLUB->isMegaPaskali)
    {
        swprintf(davlTM, L"%02.1f", _KLUB->locoPtr->TrainPipePressure);
        swprintf(davlUR, L"%02.1f", _KLUB->enginePtr->UR);
    }
    else
    {
        swprintf(davlTM, L"%01.2f", _KLUB->locoPtr->TrainPipePressure/TO_MEGA_PH);
        swprintf(davlUR, L"%01.2f", _KLUB->enginePtr->UR/TO_MEGA_PH);
    }

  _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_TM, 0, davlTM);
  _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_UR, 0, davlUR);
}

/**
 * @brief m_displayCurrentMode Отображение в текущем режиме работы (ПОЕЗДНОЙ, МАНЕВРОВЫЙ)
 * @param _KLUB Указатель на структуру КЛУБа
 * @param alsn
 */
static void m_displayCurrentMode(st_KLUB *_KLUB, st_ALSN &alsn)
{
    wchar_t distanceToCell[32];
    swprintf(_KLUB->signName, L"");
    swprintf(distanceToCell, L"");
    swprintf(_KLUB->stName, L"");



    if (_KLUB->isOn >= 2)
    {
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Poezdn, _KLUB->mode ==  KLUB_POEZDNOE);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Manevr, _KLUB->mode == KLUB_MANEVR);
        if (_KLUB->mode == KLUB_POEZDNOE)
        {
            _KLUB->distanceToCell = Svetofor_Distance(alsn, 1);
            _KLUB->speedLimit = m_speedLimit(_KLUB, alsn);
            //stationName(_KLUB->locoPtr, 3000.0, _KLUB->stName, sizeof (_KLUB->stName));
            swprintf(_KLUB->signName, L"%s %s", Svetofor_Name(alsn), _KLUB->displayCmdText);
            swprintf(distanceToCell ,L"%03d", _KLUB->distanceToCell);
        }
        else if (_KLUB->mode == KLUB_MANEVR)
        {
            swprintf(_KLUB->signName, L"    %s", _KLUB->displayCmdText);
            _KLUB->speedLimit = _KLUB->whiteSpeed;
            _KLUB->distanceToCell = -1;
        }

    }
    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_Station, 0, _KLUB->stName);
    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_SigName, 0, _KLUB->signName);
    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_RassDoCeli, 0, distanceToCell);
}


static void _checkInput(st_KLUB *_KLUB)
{
    if (_KLUB->canReadInput == 0)
        return;

    if (_KLUB->inputKey != 0)
    {
        if (_KLUB->inputKey == KLUB_CMD_K)
            _KLUB->pressed_K = 1;
        else if ( (_KLUB->inputKey >= KLUB_0 ) && (_KLUB->inputKey <= KLUB_9 ) )
        {
            _addToCmd(_KLUB, (_KLUB->inputKey - KLUB_0));
        }
        else if (_KLUB->inputKey == KLUB_UP)
        {
            if (!_KLUB->askFlag) // не было вопросов ещё
            {
                if (!_isCmdWithAsk(_KLUB->cmdForExec[0], _KLUB->cmdForExec[1]))
                    _KLUB->pressed_K = 0;
                else
                    _KLUB->askFlag = 1;
            }
            else
            {
                if ( _KLUB->cmdForExec[1] )
                    _KLUB->askFlag = 0;
            }


            if (_KLUB->cmdForExec[0] &&  !_KLUB->askFlag)
            {
                int result = _execCmd(_KLUB);
                if (result == 1 )
                    _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Beep);
                else
                {
                    if (result == -1 )
                         _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Beep_error);
                }
            }

        }
        else if (_KLUB->inputKey == KLUB_DOWN)
        {
            _KLUB->cmdForExec[0] = 0;
            _KLUB->cmdForExec[1] = 0;
            _KLUB->askFlag = 0;
            _KLUB->pressed_K = 0;
        }
        _KLUB->inputKey = 0;
        _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Pick);
    }
    _checkInputedCmd(_KLUB);
}

static void _checkInputedCmd(st_KLUB* _KLUB)
{
    if ( _KLUB->pressed_K == 0)
    {
         swprintf(_KLUB->displayCmdText, L"");
         return;
    }

    if( !_KLUB->askFlag)
    {
        if (_KLUB->cmdForExec[0] )
            swprintf(_KLUB->displayCmdText, L"ВВЕДИТЕ КОМАНДУ  %d", _KLUB->cmdForExec[0]);
        else
            swprintf(_KLUB->displayCmdText, L"ВВЕДИТЕ КОМАНДУ");
    }

    else
    {
        if (_KLUB->cmdForExec[0] == 799 )
        {
            if (_KLUB->cmdForExec[1])
                swprintf(_KLUB->displayCmdText, L"СКОР НА БЕЛЫЙ  %d",  _KLUB->cmdForExec[1]);
            else
                swprintf(_KLUB->displayCmdText, L"СКОР НА БЕЛЫЙ");
        }
        else
             swprintf(_KLUB->displayCmdText, L"НЕИЗВ КОМАНДА  %d",  _KLUB->cmdForExec[1]);
    }

}

static int _isCmdWithAsk(int cmd, int answ)
{
    if (cmd == 799)
    {
        if (answ)
            return 0;
        return 1;
    }
    return 0;
}

static void _addToCmd(st_KLUB *_KLUB, int cifer)
{
    if (_KLUB->pressed_K == 0)
        return;

    int cmdIndex = 0;
    if (_KLUB->askFlag)
        cmdIndex = 1;

    if (_KLUB->cmdForExec[cmdIndex] > 0 )
        _KLUB->cmdForExec[cmdIndex] = _KLUB->cmdForExec[cmdIndex] * 10;
    _KLUB->cmdForExec[cmdIndex] = _KLUB->cmdForExec[cmdIndex] + cifer;
}

static int _execCmd(st_KLUB *_KLUB)
{
    int retCode = 1;
    if (_KLUB->cmdForExec[0] == 123)
        _KLUB->isMegaPaskali = !_KLUB->isMegaPaskali;
    else if (_KLUB->cmdForExec[0] == 799)
    {
        _KLUB->mode = KLUB_MANEVR;
        _KLUB->whiteSpeed = _KLUB->cmdForExec[1];
    }
    else if (_KLUB->cmdForExec[0] == 800)
    {
        _KLUB->mode = KLUB_POEZDNOE;
        _KLUB->whiteSpeed = 40;
    }
    else
        retCode = 0;

    _KLUB->cmdForExec[0] = 0;
    _KLUB->cmdForExec[1] = 0;
    return retCode;
}


static void m_debug(st_KLUB *_KLUB, st_ALSN &alsn)
{
  const SignalsInfo *sigInfo = __svetoforStruct(alsn, 1);
  Printer_print(_KLUB->enginePtr, GMM_POST, L"Aspect: %u Flags: %u Tile0 %hu Tile1 %hu, Name: %s",
                sigInfo->Aspect[0],
                sigInfo->Flags,
                sigInfo->SignalInfo->Tile[0],
                sigInfo->SignalInfo->Tile[1],
                sigInfo->SignalInfo->Name
               // sigInfo->SignalInfo->Pos[0],sigInfo->SignalInfo->Pos[1],sigInfo->SignalInfo->Pos[2]
                );

  //Printer_print(_KLUB->enginePtr, GMM_POST, L"KLUB: Speed: %d SigCode %d\n", _KLUB->currSpeed, _KLUB->speedLimit);
}
