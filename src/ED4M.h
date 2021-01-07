#ifndef ED4M_LOGIC_H
#define ED4M_LOGIC_H

#include "ts.h"
#include "appliances/klub.h"
#include "appliances/radiostation.h"
#include "appliances/saut_datatype.h"
#include "src/elements.h"
#include "src/RTS/rts_data.h"
#include "ED4M_data.h"

#define NEUTRAL_CONTROLLER_POS 5
#define REVERSE_NEUTRAL 2
/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#define SECTION_DEST_FORWARD 1
#define SECTION_DEST_BACKWARD -1

#define DOORS_LEFT 1
#define DOORS_RIGHT 2

#define FLAG_DISABLED -1
#define FLAG_SET 1
#define FLAG_UNSET 0


/**
 * @brief ED4M_init Инициализация
 * @param SELF Указатель на структуру со своими параметрами.
 * @param loco Указатель на локомотив
 * @param eng Указатель на ходовую часть
 * @return Возвращает 0, если инициализирована успешно и -1 в случае ошибки
 */
int ED4M_init(struct st_Self *SELF, Locomotive *loco, Engine *eng);

/**
 * @brief ED4M_ALSN Шаг работы АЛСН.
 * @param SELF Указатель на структуру со своими параметрами.
 * @param loco Указатель на локомотив
 */
void ED4M_ALSN(struct st_Self *SELF, const Locomotive *loco);

/**
 * @brief ED4M_Step Шаг работы электрички.
 * @param SELF Указатель на структуру со своими параметрами.
 * @param loco Указатель на локомотив
 * @param eng Указатель на ходовую часть
 * @return Возвращает 1, если всё в норме и -1, в случае ошибок.
 */
int ED4M_Step(struct st_Self *SELF);


#endif // VL15_LOGIC_H
