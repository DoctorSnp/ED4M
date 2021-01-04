
#include <math.h>
#include <stdio.h>
#include <string>
#include <wchar.h>
#include "ED4M_datatypes/cab/section1/elements.h"
#include "shared_structs.h"
#include "klub.h"

static void _disable(st_KLUB* _KLUB);

int KLUB_init(st_KLUB* _KLUB, const Locomotive *loco)
{
    memset(_KLUB, 0, sizeof (st_KLUB));
    ftime(&_KLUB->prevTime);
    ftime(&_KLUB->currTime);
    _KLUB->cab = loco->cab;
    return 1;
}

void KLUB_Step(st_KLUB* _KLUB,  ElectricEngine *eng, st_ALSN& alsn)
{
    ftime(&_KLUB->currTime);
    if ( _KLUB->currTime.time <  _KLUB->prevTime.time + 1  )
        return;

    if (_KLUB->isOn == 0)
    {
        if (_KLUB->internalState != 0)
        {
            _KLUB->internalState = 0;
           // _disable(_KLUB);

        }
        return;
    }

    if (_KLUB->internalState == 0)
        _KLUB->internalState = 1;

    wchar_t w_speed[32];

    _KLUB->cab->SetDisplayState(Sensors::SnsSpeed1, 1);

    swprintf(w_speed,L"%03d", (int)alsn.CurrSpeed);
    _KLUB->cab->SetScreenLabel(Sensors::SnsSpeed1, 0, w_speed);
    swprintf(w_speed ,L"%03d", (int)alsn.SpeedLimit.Limit);
    _KLUB->cab->SetScreenLabel(Sensors::SnsSpeed1, 1, w_speed);
    _KLUB->prevTime.time = _KLUB->currTime.time;
}


static void _disable(st_KLUB* _KLUB)
{
    _KLUB->cab->SetDisplayState(Sensors::SnsSpeed1, 1);

}
