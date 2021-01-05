
#include <math.h>
#include <stdio.h>
#include <string>
#include <stdio.h>
#include <wchar.h>
#include "ED4M_datatypes/cab/section1/elements.h"
#include "shared_structs.h"
#include "klub.h"
#include "utils/utils.h"

static void _display(st_KLUB* _KLUB, st_ALSN &alsn, Engine *eng);
static void _setEnabled(st_KLUB *_KLUB, int isEnabled );
static void _setBillColour(st_KLUB* _KLUB, int code);


int KLUB_init(st_KLUB* _KLUB)
{
    memset(_KLUB, 0, sizeof (st_KLUB));
    ftime(&_KLUB->prevTime);
    ftime(&_KLUB->currTime);
    return 1;
}

void KLUB_setState(st_KLUB *_KLUB, int newState)
{
    if (_KLUB->isOn != newState)
        _setEnabled(_KLUB, newState);
}

void KLUB_Step(st_KLUB* _KLUB,  Engine *eng, st_ALSN& alsn, const Locomotive *loco)
{
    ftime(&_KLUB->currTime);
    if ( _KLUB->currTime.time <  _KLUB->prevTime.time + 1  )
        return;

    _KLUB->cabPtr = loco->cab;

    _display(_KLUB, alsn, eng);
    _KLUB->prevTime.time = _KLUB->currTime.time;
}


static void _display(st_KLUB* _KLUB, st_ALSN& alsn, Engine *eng)
{
    wchar_t tempText[32];

    swprintf(tempText,L"%03d", (int)alsn.CurrSpeed);
    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_Speed1, 0, tempText);

    if (_KLUB->isOn >= 2 )
        swprintf(tempText ,L"%03d", (int)alsn.SpeedLimit.Limit);
    else
        swprintf(tempText ,L"");

    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_Speed1, 1, tempText);


    swprintf(tempText,L"км%dпк%d", int(eng->CurrentMilepost), int((eng->CurrentMilepost * 10)) % 10) ;
    _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_KM, 0, tempText);

    if (_KLUB->isOn > 0 )
    {
        swprintf(tempText ,L"%s", "СтанциЯ");
        upperCase(tempText);
        _KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_Station, 0, tempText);
        _setBillColour(_KLUB, m_getSignCode(&alsn));
    }

    //swprintf(tempText ,L"%s", alsn.signalName);

    //_KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_SigName, 0, tempText);

     //swprintf(tempText ,L"%03d", alsn.SpeedLimit.Distance);
     //_KLUB->cabPtr->SetScreenLabel(Sensors::Sns_KLUB_RassDoCeli, 0, tempText);

}

static void _setEnabled(st_KLUB *_KLUB, int isEnabled )
{
    _KLUB->isOn = isEnabled;

    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Speed1, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_KM, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RassDoCeli, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_SigName, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Station, isEnabled);
    _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Time, isEnabled);
    //if (_KLUB->isOn < 2)
   // {
   //     _setBillColour(_KLUB, -1);
  //  }

}

/**
 * @brief _setBillColour Устанавливает цвета КЛУБа
 * @param _KLUB
 * @param code
 */
static void _setBillColour(st_KLUB* _KLUB, int code)
{
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

    en_SignColors colour = m_getSignColor(code);

    if (colour == en_SignColors::COLOR_GREEN)
    {
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green4, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green3, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green2, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Green1, 1);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Yellow, 1);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_RedYellow, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_Red, 0);
        _KLUB->cabPtr->SetDisplayState(Sensors::Sns_KLUB_White, 0);
    }
    else if (colour == en_SignColors::COLOR_YELLOW)
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

    else if (colour == en_SignColors::COLOR_RD_YEL)
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
    else if (colour == en_SignColors::COLOR_RED)
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
