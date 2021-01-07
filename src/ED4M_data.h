/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef ED4M_DATA_H
#define ED4M_DATA_H

#include "ts.h"

#include "appliances/klub.h"
#include "appliances/radiostation.h"
#include "src/RTS/rts_data.h"

#define SWITCHES_CNT 790 /*максимальный ID элемента в файле кабины. Как правило cab.sd */
#define TUMBLERS_MAX_ID SWITCHES_CNT
#define ARMS_MAX_ID     SWITCHES_CNT

PACKED_BEGIN

#define CABS_COUNT 2

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
    int seconds;
    float milliseconds;
    unsigned long State;
}st_game;
/**
 * @brief The st_Self struct Структура с собственными параметрами для работы между функциями библиотеки.
 */
struct st_Self
{

  int tumblersArray[TUMBLERS_MAX_ID];
  int armsArray[ARMS_MAX_ID];
  int tempFlags[TUMBLERS_MAX_ID];
  int buttonsArray[SWITCHES_CNT];
  Electric elecrto;
  Pneumo pneumo;
  float prevVelocity;
  bool SL2M_Ticked;
  int BV_STATE;

  st_Radiostation radio;
  int Reverse;
  unsigned int TyagaPosition;
  unsigned int RecuperationPosition;
  int shuntNum;
  st_ALSN alsn;
  st_timeForDebug debugTime;
  st_ServiceInfo service;
  int cabNum;
  st_game game;

  st_KLUB KLUB[CABS_COUNT];
};

PACKED_END

#endif // ED4M_DATA_H
