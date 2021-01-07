
#include <math.h>
#include <stdio.h>

#include "private_ED4M.h"

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
