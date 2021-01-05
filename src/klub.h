#ifndef KLUB_H
#define KLUB_H

#include "sys/timeb.h"
#include "ts.h"
#include "shared_structs.h"


int KLUB_init(st_KLUB* _KLUB);

void KLUB_setState(st_KLUB *_KLUB, int newState);
void KLUB_Step(st_KLUB *_KLUB, Engine *eng, st_ALSN& alsn, const Locomotive *loco);

#endif // KLUB_H
