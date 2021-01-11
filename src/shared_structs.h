/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef SHARED_STRUCTS_H
#define SHARED_STRUCTS_H

#include "ts.h"


#define SIGNALS_CNT 20
#define MAX_STR_LEN 256

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

// описатель времени в секундан и миллисекундых (Берётся время игры)
typedef struct st_gameTime
{
    int seconds = 0;
    int millis = 0;
}st_gameTime;

typedef struct st_ALSN
{
 float CurrSpeed;
 float PrevSpeed;
 struct SpeedLimitDescr SpeedLimit;
 int correctALSNCode;
 int NumSigForw;
 int NumSigBack;
 int NumSigPassed;
 SignalsInfo signListPassed;
 SignalsInfo ForwardSignalsList[SIGNALS_CNT];
 bool isBackward;
}st_ALSN;


PACKED_END
#ifdef __cplusplus
}
#endif
#endif // SHARED_STRUCTS_H
