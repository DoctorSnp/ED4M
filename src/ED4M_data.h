#ifndef ED4M_DATA_H
#define ED4M_DATA_H

#include "ts.h"

#include "appliances/klub.h"
#include "appliances/radiostation.h"
#include "src/RTS/rts_data.h"


#define TUMBLERS_MAX_ID 790
PACKED_BEGIN


typedef struct st_Electric
{
    unsigned char PantoRaised;
    float power;
    float LineVoltage;
}Electric;

typedef struct st_Tumblers
{
    int projHalf;
    int projFull;
    int AvarEPT;
}st_Tumblers;

typedef struct st_game
{
    const ElectricLocomotive *locoPtr;
    ElectricEngine *engPtr;
    const Cabin *cabPtr;
    float AirTemperature;
    float time;
    unsigned long State;
}st_game;
/**
 * @brief The st_Self struct Структура с собственными параметрами для работы между функциями библиотеки.
 */
struct st_Self
{
  //int destination;
  int tumblersArray[TUMBLERS_MAX_ID];
  int tempFlags[TUMBLERS_MAX_ID];
  Electric elecrto;
  Pneumo pneumo;
  float prevVelocity;
  bool SL2M_Ticked;
  int BV_STATE;
  int EPK;
  int RB;
  int MK;
  int MV_low;
  st_Radiostation radio;
  int Reverse;
  unsigned int TyagaPosition;
  unsigned int RecuperationPosition;
  int shuntNum;
  st_ALSN alsn;
  st_timeForDebug debugTime;
  st_ServiceInfo service;
  st_game game;
  st_KLUB KLUB;
};

PACKED_END

#endif // ED4M_DATA_H
