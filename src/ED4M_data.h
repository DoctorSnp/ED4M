/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef ED4M_DATA_H
#define ED4M_DATA_H

#include "appliances/klub.h"
#include "appliances/radiostation.h"
#include "src/RTS/rts_data.h"

#define SWITCHES_CNT 790 /*максимальный ID элемента в файле кабины. Как правило cab.sd */
#define TUMBLERS_MAX_ID SWITCHES_CNT
#define ARMS_MAX_ID     SWITCHES_CNT
#define CABS_COUNT 2

/* нулевой индекс массива нужен для "абстрактных данных кабины".
 * Т.е. общих данных на поезд и чтобы проще было гулять по массиву */
#define CABS_ARRAY_SIZE CABS_COUNT + 1

PACKED_BEGIN

// Пневматика
typedef struct st_Pneumo
{
    int CompressorMainWork;
    int CompressorAuxWork;
}Pneumo;

// Электрика
typedef struct st_Electric
{
    unsigned char PantoRaised;
    float power;
    float LineVoltage;
    unsigned int    TyagaPosition;
    unsigned int    RecuperationPosition;
    int lkitTime;
}Electric;

/**
 * @brief The st_Self struct Структура с собственными параметрами для работы между функциями библиотеки.
 */
struct st_Self
{
  st_gameTime currTime;
  st_gameTime prevTime;
  int cabNum;
  st_game game;
  wchar_t errorText[MAX_STRING_LEN];

  int tumblers[CABS_ARRAY_SIZE][TUMBLERS_MAX_ID];
  int arms[CABS_ARRAY_SIZE][ARMS_MAX_ID];
  int buttons[CABS_ARRAY_SIZE][SWITCHES_CNT];

  Electric elecrto[CABS_ARRAY_SIZE];
  Pneumo  pneumo[CABS_ARRAY_SIZE];
  float   prevVelocity;
  int     doors[2];
  int     BV_STATE[CABS_ARRAY_SIZE];
  st_Radiostation radio[CABS_ARRAY_SIZE];
  st_ALSN alsn[CABS_ARRAY_SIZE];
  st_KLUB KLUB[CABS_ARRAY_SIZE];
};

PACKED_END

#endif // ED4M_DATA_H
