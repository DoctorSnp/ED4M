/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/


#include <math.h>
#include <stdio.h>
#include "src/RTS/rts_data.h"


void _playSound(const Locomotive *loco, int soundId, int where )
{
    if (where == 2)
        loco->PostTriggerBoth((unsigned short)soundId);
    else if (where == 1)
        loco->PostTriggerEng((unsigned short)soundId);
    else
        loco->PostTriggerCab((unsigned short)soundId);
}


int _checkSwitch(const Locomotive *loco, unsigned int switchElem, int soundId, int singleSound, int where)
{
    int elemState = loco->Cab()->Switch(switchElem);
    if (soundId >= 0)
    {
        if (elemState > 0)
            _playSound(loco, soundId, where); // устанавливаем звук
        else
        {
            if (!singleSound) // если не PlayOneShot
                _playSound(loco, soundId + 1); // сбрасываем звук
            else
                _playSound(loco, soundId, where); // устанавливаем звук
        }
    }
    return elemState ;
}
