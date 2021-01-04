#ifndef KLUB_H
#define KLUB_H

#include "sys/timeb.h"
#include "ts.h"
#include "shared_structs.h"


int KLUB_init(st_KLUB* _KLUB, const Locomotive *loco);

void KLUB_Step(st_KLUB *_KLUB, ElectricEngine *eng, st_ALSN& alsn);

#endif // KLUB_H
