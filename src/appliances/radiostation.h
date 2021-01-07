#ifndef RADIOSTATION_H
#define RADIOSTATION_H

#include <sys\timeb.h>
#include "ts.h"

PACKED_BEGIN
typedef struct st_Radiostation
{
    int isActive;
    struct timeb prevTime;
    struct timeb currTime;
}st_Radiostation;
PACKED_END

void Radiostation_Init(st_Radiostation *radio);
void Radiostation_Step(const ElectricLocomotive *loco, ElectricEngine *eng, st_Radiostation *radio);
void Radiostation_setEnabled(st_Radiostation *radio, int isOn);

#endif // RADIOSTATION_H
