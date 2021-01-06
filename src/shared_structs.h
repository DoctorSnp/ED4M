#ifndef SHARED_STRUCTS_H
#define SHARED_STRUCTS_H

#include "sys/timeb.h"
#include "shared_code.h"
#include "ts.h"

#define SIGNALS_CNT 20

#ifdef __cplusplus
extern "C" {
#endif


typedef enum en_SignColors
{
  UNSET = -1,
  COLOR_WHITE = 0,
  COLOR_RED = 1,
  COLOR_RD_YEL = 2,
  COLOR_YELLOW = 3,
  COLOR_GREEN = 4,

}en_SignColors;



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
 wchar_t signalName[MAX_STRING_NAME];
}st_ALSN;



typedef struct st_KLUB
{
    int mode;
    int speedLimit;
    //int currCmd;
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


PACKED_END
#ifdef __cplusplus
}
#endif
#endif // SHARED_STRUCTS_H
