/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef SAUT_H
#define SAUT_H

#include "ts.h"
#include "saut_datatype.h"

#define SAUT_NORMAL 1
#define SAUT_DISABLED 0
#define EPK_ALARM_FOR_EPK 2

typedef enum en_Saut
{
    DISABLE_TYAGA = 1200,
    PLATFORM = 1201,
    STATION = 1202,
    Drive_Backward = 1203,
    Start_Drive = 1204,
    NEITRAL_STATE= 1205,
    GREEN = 1206,
    YELLOW = 1207,
    KG = 1208,
    RED = 1209,
    WHITE = 1210,
    SAUT_OFF = 1211,
    TRY_BRAKE  = 1212,
    STO = 1213,
    PEREEZD = 1214,
    PICK = 1215,
    SIGNAL = 1216,
    TRAIN_STOPPED = 1217,
    ReversDown = 1218,
    ReversUp = 1219,
    Force_Is_Disabled = 1220,
    SPEED_LIMIT = 1221
}
SAUT_sounds;


typedef struct st_intermnalSAUT
{
    int timeForDisableForce;
    int timeForPick;
    int timeForEPKstart;
    int timeForEPKbrake;
    struct timeb prevTime;
    struct timeb currTime;
    float Speed;
    int SpeedLimit;
    int Distance;
    const Locomotive *locoPtr;
    Engine *engPtr;

    SignColors m_siglNext =  SignColors::COLOR_WHITE;
    SignColors m_siglPrev =  SignColors::COLOR_WHITE;

}st_intermnalSAUT;

class SAUT
{
public:
    SAUT();
    int init() noexcept;
    int start(const Locomotive *loco, Engine *eng, const st_ALSN *alsn) noexcept;
    int stop(const Locomotive *loco, Engine *eng) noexcept;
    int step(const Locomotive *loco, Engine *eng, const st_ALSN *alsn) noexcept;
    int dist() { return  m_SELF.Distance; }
    const wchar_t * stateString() noexcept { return m_stateString; }

private:
    /**
     * @brief m_getSignCode Получает код сигнала впереди
     * @param alsn указатель на АЛСН.
     * @return  код сигнала в формате RTS или -1 , если не удалось распознать сигнал
     */
    int m_getSignCode(const st_ALSN *alsn) noexcept;
    SignColors m_getSignColor(int sig) noexcept;
    void m_playColor(const Locomotive *loco, SignColors colour) noexcept;
    void m_updateSoundsTime() noexcept;
    void m_SoundPip(const Locomotive *loco) noexcept ;
    void m_Sound_DisableTyaga(const Locomotive *loco) noexcept;
    void m_Sound_ResetTime() noexcept;
    void m_makeDisplay(const Locomotive *loco) noexcept;

    /**
     * @brief m_scanObjects Проверяет объекты впереди, сзади или под составом
     * @param loco  указатель на лок.
     * @param where Где искать (1) - впереди, 2 - сзади, 4 под
     */
    void m_scanObjects(const Locomotive *loco, int where = 1);

    wchar_t m_stateString[MAX_STRING_NAME];
    int isEnabled = false;
    st_intermnalSAUT m_SELF;
};

#endif // SAUT_H
