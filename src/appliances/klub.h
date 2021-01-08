/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef KLUB_H
#define KLUB_H

#include "ts.h"
#include "src/shared_structs.h"


typedef struct st_KLUB
{
    int mode;
    int speedLimit;
    int distanceToCell;
    int cmdForExec;
    int pressed_K;
    int canReadInput;
    int inputKey;
    int isOn;
    st_gameTime __prevTime;
    st_gameTime currTime;
    Cabin *cabPtr;
    Engine *enginePtr;
    const Locomotive *locoPtr;
    int prevSignCode;
    int currSignCode;
    wchar_t signName[32];
    wchar_t stName[32];
}st_KLUB;

int KLUB_init(st_KLUB* _KLUB);

void KLUB_setState(st_KLUB *_KLUB, int newState);
void KLUB_Step(st_KLUB *_KLUB, Engine *eng, st_ALSN& alsn, const Locomotive *loco);

#endif // KLUB_H
