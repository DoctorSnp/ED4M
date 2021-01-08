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

/*Звуки КЛУБа*/
enum KLUB_sounds
{
    KLUB_Beep = 1401,
    KLUB_Pick = 1402,
    KLUB_Beep_error = 1403,
};

static void _display(st_KLUB* _KLUB, st_ALSN &alsn, Engine *eng);
static void _setEnabled(st_KLUB *_KLUB, int isEnabled );

static void _displayCurrentMode(st_KLUB *_KLUB, st_ALSN &alsn);
static void _displaySignals(st_KLUB* _KLUB, int code);
static void _displayTime(st_KLUB* _KLUB, int isDisplay);
static void  _displayPressure(st_KLUB* _KLUB, int isDisplay);
static void _displayReg(st_KLUB* _KLUB, int isDisplay);

/*Ввод и обработка команд */
static void _checkInput(st_KLUB* _KLUB);
static void _addToCmd(st_KLUB* _KLUB, int cifer);
static int _execCmd(st_KLUB *_KLUB);
/**************************/

int KLUB_init(st_KLUB* _KLUB)
{
    memset(_KLUB, 0, sizeof (st_KLUB));
    _KLUB->__prevTime  = {};
   // ftime(&_KLUB->prevTime);
   // ftime(&_KLUB->currTime);
    _KLUB->cmdForExec = 0;
    _KLUB->prevSignCode = -1;
    _KLUB->currSignCode = -1;
    return 1;
}

void KLUB_setState(st_KLUB *_KLUB, int newState)
{
    if (_KLUB->isOn != newState)
    {
        _KLUB->cmdForExec = 0;
        _setEnabled(_KLUB, newState);
    }
}

void KLUB_Step(st_KLUB* _KLUB,  Engine *eng, st_ALSN& alsn, const Locomotive *loco)
{
    //ftime(&_KLUB->currTime);
    if ( _KLUB->currTime.seconds < _KLUB->__prevTime.seconds + 1 )
        if ( _KLUB->currTime.millis  < _KLUB->__prevTime.millis + 500 )
            return;

    _KLUB->locoPtr = loco;
    _KLUB->cabPtr = loco->cab;
    _KLUB->enginePtr = eng;

    if ( _KLUB->mode == KLUB_POEZDNOE ) {
         _KLUB->speedLimit =  (int)alsn.SpeedLimit.Limit;
        _KLUB->distanceToCell = svetoforDistance(alsn, 1);
    }
    else if (_KLUB->mode == KLUB_MANEVR)
    {
        _KLUB->speedLimit = 60;
        _KLUB->distanceToCell = -1;
    }

    _display(_KLUB, alsn, eng);
    _KLUB->__prevTime.seconds = _KLUB->currTime.seconds;
    _KLUB->__prevTime.millis = _KLUB->currTime.millis;

    if (_KLUB->isOn < 2)
        return;

    _checkInput( _KLUB);
}


static void _display(st_KLUB* _KLUB, st_ALSN& alsn, Engine *eng)
{
    wchar_t currSpeed[32];
    wchar_t speedLimit[32];

    if (_KLUB->isOn < 1)
    {

     _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_RED_TRIANGLE, 1 );
     // _KLUB->cabPtr->SetDisplayState(Sensors::TEST_KLUB_BIL2_LAMP, 1);

    //_KLUB->cabPtr->SetScreenValue(Sensors::TEST_KLUB_BIL_SECTOR_SCREEN, 1 , 160.0);
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

        return;
    }

    _displayReg(_KLUB, _KLUB->isOn);
    _displayTime(_KLUB, _KLUB->isOn);
    _displayPressure(_KLUB, _KLUB->isOn);

    swprintf(currSpeed, L"%03d", (int)alsn.CurrSpeed);
    if (_KLUB->isOn >= 1 )
    {
        if (_KLUB->isOn == 1)
        {
            int newState = !_KLUB->cabPtr->ScreenState(Sensors::Sns_KLUB_Speed1, 0);
            if (newState == 0)
                swprintf(currSpeed, L"");
            _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_Speed1, 0, newState);
            swprintf(speedLimit ,L"");
        }
        else
           swprintf(speedLimit ,L"%03d", _KLUB->speedLimit);
        _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_Speed1, 0, currSpeed);
        _KLUB->cabPtr->SetScreenValue(Sensors::Sns_KLUB_SpeedGreenCircle, 0 , alsn.CurrSpeed);
        _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_Speed1, 1, speedLimit);
    }

    wchar_t currKilometer[32];
    swprintf(currKilometer, L"%4.3f", eng->CurrentMilepost);
    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_KM, 0, currKilometer);
     _displayCurrentMode(_KLUB, alsn);
    _displaySignals(_KLUB, m_getSignCode(&alsn));
}

static void _setEnabled(st_KLUB *_KLUB, int isEnabled )
{
    _KLUB->isOn = isEnabled;
    if (_KLUB->mode == KLUB_UNSET)
        _KLUB->mode = KLUB_POEZDNOE;
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Speed1, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_KM, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RassDoCeli, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_SigName, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Station, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Time, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_SpeedGreenCircle, isEnabled);

}

/**
 * @brief _setBillColour Устанавливает цвета КЛУБа
 * @param _KLUB
 * @param code
 */
static void _displaySignals(st_KLUB* _KLUB, int code)
{
    static int timeForCheckCode = 0;
    if ( code != _KLUB->prevSignCode )
    {
            if (timeForCheckCode++ >= 4)
            {
                _KLUB->currSignCode = code;
                _KLUB->prevSignCode = _KLUB->currSignCode;
                if ( !IS_GREEN(code) )
                {
                    _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Beep);
                }

                timeForCheckCode = 0;
            }
    }

    if ( _KLUB->isOn < 2  )
    {
        int setState = 0;
        if (code == 0xffffff)
            setState = 1;

        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green4, setState);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green3, setState);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green2, setState);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green1, setState);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Yellow, setState);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RedYellow, setState);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Red, setState);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_White, setState);
        return;
    }

     SignColors colour = SignColors::COLOR_WHITE;
    if (_KLUB->mode == KLUB_POEZDNOE)
        colour = m_getSignColor(_KLUB->currSignCode);

    if (colour == SignColors::COLOR_GREEN)
    {
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green4, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green3, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green2, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green1, 1);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Yellow, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RedYellow, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Red, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_White, 0);
    }
    else if (colour == SignColors::COLOR_YELLOW)
    {
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green4, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green3, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green2, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green1, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Yellow, 1);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RedYellow, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Red, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_White, 0);
    }

    else if (colour == SignColors::COLOR_RD_YEL)
    {
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green4, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green3, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green2, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green1, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Yellow, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RedYellow, 1);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Red, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_White, 0);
    }
    else if (colour == SignColors::COLOR_RED)
    {
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green4, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green3, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green2, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green1, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Yellow, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RedYellow, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Red, 1);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_White, 0);
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
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_White, 1);
    }
}

/**
 * @brief _displayTime Показывает или скрывает время КЛУБа
 * @param _KLUB Указатель на структуру КЛУБа
 * @param isDisplay 1 показывать время, 0 - не показывать
 */
static void _displayTime(st_KLUB* _KLUB, int isDisplay)
{
    if (isDisplay == 0)
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

static void  _displayPressure(st_KLUB* _KLUB, int isDisplay)
{
    if (isDisplay == 0)
    {
        _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_TM, 0, 0);
        _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_UR, 0, 0);
        return;
    }

    if (_KLUB->cabPtr->ScreenState(Sensors::Sns_KLUB_TM, 0) < 1)
        _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_TM, 0, 1);
    if (_KLUB->cabPtr->ScreenState(Sensors::Sns_KLUB_UR, 0) < 1)
        _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_UR, 0, 1);

    wchar_t davlTM [32];
    wchar_t davlUR [32];
    swprintf(davlTM, L"%2.1f", _KLUB->locoPtr->TrainPipePressure);
    swprintf(davlUR, L"%2.1f", _KLUB->enginePtr->UR);

  _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_TM, 0, davlTM);
  _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_UR, 0, davlUR);

}

static void _displayReg(st_KLUB* _KLUB, int isDisplay)
{
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Kasseta, isDisplay );
}


static void _displayCurrentMode(st_KLUB *_KLUB, st_ALSN &alsn)
{
    wchar_t distanceToCell[32];

    swprintf(_KLUB->stName ,L"");
    swprintf(_KLUB->signName, L"");
    swprintf(distanceToCell ,L"");

    if (_KLUB->isOn >= 2)
    {
        if ( _KLUB->mode == KLUB_POEZDNOE )
        {
            _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_Poezdn, 0, 1);
            _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_Manevr, 0, 0);
            const wchar_t  *stNameStr = stationName(_KLUB->locoPtr, 1000.0);
            if (stNameStr)
                swprintf(_KLUB->stName ,L"%s", stNameStr);
            swprintf(_KLUB->signName, L"%s", svetoforName(alsn));
            swprintf(distanceToCell ,L"%03d", _KLUB->distanceToCell);
        }
        else
        {
            _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_Poezdn, 0, 0);
            _KLUB->cabPtr->SetScreenState(Sensors::Sns_KLUB_Manevr, 0, 1);
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
            _addToCmd(_KLUB, (_KLUB->inputKey - KLUB_0));
        else if (_KLUB->inputKey == KLUB_UP)
        {
            if (_KLUB->cmdForExec)
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
            _KLUB->cmdForExec = 0;
            _KLUB->pressed_K = 0;
        }
        _KLUB->inputKey = 0;
        _KLUB->locoPtr->PostTriggerCab(KLUB_sounds::KLUB_Pick);
    }
}

static void _addToCmd(st_KLUB *_KLUB, int cifer)
{
    if (_KLUB->pressed_K == 0)
        return;
    if (_KLUB->cmdForExec > 0 )
        _KLUB->cmdForExec = _KLUB->cmdForExec * 10;
    _KLUB->cmdForExec = (_KLUB->cmdForExec + cifer);

    //Printer_print(_KLUB->enginePtr, GMM_POST, L"Entered cifer %d cmd %d\n", cifer, _KLUB->cmdForExec);
}

static int _execCmd(st_KLUB *_KLUB)
{
    int retCode = -1;
    if (_KLUB->cmdForExec == 0)
        retCode = 0;
    if (_KLUB->cmdForExec == 799)
    {
        _KLUB->mode = KLUB_MANEVR;
        retCode = 1;
    }
    if (_KLUB->cmdForExec == 800)
    {
        _KLUB->mode = KLUB_POEZDNOE;
        retCode = 1;
    }

    _KLUB->cmdForExec = 0;
    return retCode;
}
