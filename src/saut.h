#ifndef SAUT_H
#define SAUT_H

#include "ts.h"
#include "saut_datatype.h"

#define SAUT_NORMAL 1
#define SAUT_DISABLED 0
#define EPK_ALARM_FOR_EPK 2

typedef enum en_Saut
{
    DISABLE_TYAGA = 200,
    PLATFORM = 201,
    STATION = 202,
    Drive_Backward = 203,
    Start_Drive = 204,
    NEITRAL_STATE= 205,
    GREEN = 206,
    YELLOW = 207,
    KG = 208,
    RED = 209,
    WHITE = 210,
    SAUT_OFF = 211,
    TRY_BRAKE  = 212,
    STO = 213,
    PEREEZD = 214,
    PICK = 215,
    SIGNAL = 216,
    TRAIN_STOPPED = 217,
    ReversDown = 218,
    ReversUp = 219,
    Force_Is_Disabled = 220,
    SPEED_LIMIT = 221
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
    float SpeedLimit;
    int Distance;
    en_SignColors m_siglNext =  en_SignColors::COLOR_WHITE;
    en_SignColors m_siglPrev =  en_SignColors::COLOR_WHITE;

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
    en_SignColors m_getSignColor(int sig) noexcept;
    void m_playColor(const Locomotive *loco, en_SignColors colour) noexcept;
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
