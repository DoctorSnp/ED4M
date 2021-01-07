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
    int cabNum;
    float currTime;
    float prevTIme;
};


#endif // RTS_DATA_H