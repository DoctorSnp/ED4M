/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef KLUB_H
#define KLUB_H

#include "ts.h"
#include "src/shared_structs.h"


typedef struct st_KLUB
{
    int mode;
    int currSpeed;
    int speedLimit;
    int signalLimit;
    int distanceToCell;
    int cmdForExec[4];
    int pressed_K;
    int canReadInput;
    int inputKey;
    int isOn;
    st_gameTime __prevTime;
    st_gameTime currTime;
    Cabin *cabPtr;
    Engine *enginePtr;
    const Locomotive *locoPtr;
    int prevSignCode;
    wchar_t signName[MAX_STR_LEN];
    wchar_t stName[MAX_STR_LEN/4];
    wchar_t displayCmdText[MAX_STR_LEN/2];
    wchar_t askText[MAX_STR_LEN/2];
    int askFlag;
    int whiteSpeed = 60;
    bool isMegaPaskali;
}st_KLUB;

/**
 * @brief KLUB_init Инициализация КЛУБ-У
 * @param _KLUB Указатель на структуру данных КЛУБ-У
 * @return  Возвращает 0 в случае успеха и -1 в случае ошибки.
 */
int KLUB_init(st_KLUB* _KLUB);

void KLUB_Step(st_KLUB *_KLUB, Engine *eng, st_ALSN& alsn, const Locomotive *loco);

#endif // KLUB_H
