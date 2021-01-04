#ifdef _WIN32
#include <tchar.h>
#endif


#ifndef MAIN_ED4M_H
#define MAIN_ED4M_H


#include "src/private_ED4M.h"
#include "src/ts.h"


#define CURRENT_Q 430.0
#define CURRENT_Q_BF 0.0025
#define TIME_THROTTLE_SWITCH1 1.0
#define TIME_THROTTLE_SWITCH2 2.0

#define BRAKE_STR_RATE 1.8
#define BRAKE_MR_RATIO    0.005
#define BRAKE_PIPE_RATE_CHARGE 1.8
#define BRAKE_UR_RATE_CHARGE   0.25
#define BRAKE_PIPE_RATE 0.5
#define BRAKE_PIPE_APPL_RATE   0.25
#define BRAKE_PIPE_EMERGENCY -1.2
#define PIPE_DISCHARGE_SLOW  -0.005

#define FORCE_SHIFT 30000.0
#define BRAKE_FORCE_SHIFT 10000.0
#define CURRENT_SHUNT_Q 1.2


extern "C" void Q_DECL_EXPORT Switched(const ElectricLocomotive *loco,ElectricEngine *eng,
        unsigned int SwitchID,unsigned int PrevState);

#endif
