/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef RTS_DATA_H
#define RTS_DATA_H

#include <sys/timeb.h>

typedef struct st_timeForDebug
{
    struct timeb prevTime;
    struct timeb currTime;
}st_timeForDebug;

typedef struct st_Pneumo
{
  //int Arm_254;
  int Arm_395;
  int Blok_367;

}Pneumo;

struct st_ServiceInfo
{
    float currTime;
    float prevTIme;
};


#endif // RTS_DATA_H
