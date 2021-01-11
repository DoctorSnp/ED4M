/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/


#ifndef RTS_DATA_H
#define RTS_DATA_H

#include "ts.h"
#include "src/shared_structs.h"
#define MAX_STRING_LEN 256


#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define Q_DECL_EXPORT __declspec(dllexport)
#  define Q_DECL_IMPORT __declspec(dllimport)
#else
#  define Q_DECL_EXPORT     __attribute__((visibility("default")))
#  define Q_DECL_IMPORT     __attribute__((visibility("default")))
#endif

#if defined(RTS_LIBRARY)
#  define RTS_EXPORT Q_DECL_EXPORT
#else
#  define RTS_EXPORT Q_DECL_IMPORT
#endif


void _playSound(const Locomotive *loco, int soundId, int where = 0 );


/**
 * @brief _checkSwitchWithSound Возвращает состояние элемента и проигрывает его звук
 * @param loco
 * @param switchElem
 * @param soundId
 * @return
 */
int _checkSwitch(const Locomotive *loco, unsigned int switchElem, int soundId, int singleSound, int where);

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))


// структура RTS
typedef struct st_game
{
    const ElectricLocomotive *locoPtr;
    ElectricEngine *engPtr;
    Cabin *cabPtr;
    float AirTemperature;
    float gameTimeBuffer;
    float time;
    st_gameTime currTime;
    unsigned long State;
    int isNight;
}st_game;

#endif // RTS_DATA_H
