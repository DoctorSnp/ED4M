/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef BRAKE_DATA_H
#define BRAKE_DATA_H

#include "ts.h"

PACKED_BEGIN

/** Структура-описатель текущего состояния 395 крана
  *
  */
typedef struct st_Brake_395
{
    int m_currPos = -1;
    int m_prevPos = -1;

}st_Brake_395;
PACKED_END

#endif // BRAKE_DATA_H
