#ifndef KLUB_H
#define KLUB_H

#include "sys/timeb.h"
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
    struct timeb prevTime;
    struct timeb currTime;
    Cabin *cabPtr;
    Engine *enginePtr;
    const Locomotive *locoPtr;
}st_KLUB;

int KLUB_init(st_KLUB* _KLUB);

void KLUB_setState(st_KLUB *_KLUB, int newState);
void KLUB_Step(st_KLUB *_KLUB, Engine *eng, st_ALSN& alsn, const Locomotive *loco);

#endif // KLUB_H
