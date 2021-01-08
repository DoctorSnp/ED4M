/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef SHARED_STRUCTS_H
#define SHARED_STRUCTS_H

#include "sys/timeb.h"
#include "ts.h"

#define SIGNALS_CNT 20

#ifdef __cplusplus
extern "C" {
#endif


typedef enum SignColors
{
  UNSET = -1,
  COLOR_WHITE = 0,
  COLOR_RED = 1,
  COLOR_RD_YEL = 2,
  COLOR_YELLOW = 3,
  COLOR_GREEN = 4,

}SignColors;



PACKED_BEGIN


typedef struct st_ALSN
{
 float CurrSpeed;
 float PrevSpeed;
 struct SpeedLimitDescr SpeedLimit;
 int NumSigForw;
 int NumSigBack;
 int NumSigPassed;
 SignalsInfo signListBack;
 SignalsInfo signListPassed;
 SignalsInfo ForwardSignalsList[SIGNALS_CNT];
 //wchar_t signalName[MAX_STRING_NAME];
 bool isBackward;
}st_ALSN;


PACKED_END
#ifdef __cplusplus
}
#endif
#endif // SHARED_STRUCTS_H
