#include <cmath>
#include <stdio.h>

#include "src/utils/utils.h"
#include "src/elements.h"
#include "saut.h"

constexpr int TIME_FOR_DISABLE_TYAGA = 20;
constexpr int TIME_FOR_EPK_START = 14;
constexpr int TIME_FOR_EPK_BRAKE = TIME_FOR_EPK_START + 14;
constexpr int WHITE_LIMIT = 40; // ограничение на белый
constexpr int RED_LIMIT = 20;   // ограничение на красный


SAUT::SAUT()
{}

int SAUT::init() noexcept
{
    swprintf(m_stateString, sizeof(m_stateString),  L"%d/%d\n", 0,0);
    return 1;
}

int SAUT::start(const Locomotive *loco, Engine *eng, const st_ALSN *alsn) noexcept
{
    ftime(&m_SELF.prevTime);
    ftime(&m_SELF.currTime);

    m_SELF.m_siglNext = m_getSignColor(m_getSignCode(alsn));
    m_SELF.m_siglPrev = m_SELF.m_siglNext;
    m_SELF.timeForDisableForce = 5;
    m_SELF.timeForPick = 2;
    m_SELF.timeForEPKstart = 0;
    m_SELF.timeForEPKbrake = 0;

    isEnabled = 1;
    eng->ALSNOn = 0x1;
    m_playColor(loco, m_SELF.m_siglNext);
    return 1;
}

int SAUT::stop(const Locomotive *loco, Engine *eng) noexcept
{
    isEnabled = 0;
    eng->ALSNOn = 0;
    loco->PostTriggerCab(SAUT_sounds::SAUT_OFF);
    return 1;
}

int SAUT::step(const Locomotive *loco, Engine *eng, const st_ALSN *alsn) noexcept
{
    int sautState = SAUT_NORMAL;
    if (isEnabled == 0)
        return SAUT_DISABLED;

    m_SELF.Speed = alsn->CurrSpeed;
    //m_SELF.SpeedLimit = alsn->SpeedLimit.Limit;

    if (m_SELF.m_siglNext == SignColors::COLOR_WHITE)
        m_SELF.SpeedLimit = WHITE_LIMIT;

    if (m_SELF.m_siglNext == SignColors::COLOR_RED)
        m_SELF.SpeedLimit = RED_LIMIT;

    m_makeDisplay(loco);
    if ( loco->Velocity == 0.0)
        return SAUT_NORMAL;


    ftime(&m_SELF.currTime);
    m_updateSoundsTime();

    if ((m_SELF.SpeedLimit - 2.0) <= m_SELF.Speed) // за 2 км/ч начинаем пищать
         m_SoundPip(loco);

    if ( m_SELF.SpeedLimit < m_SELF.Speed )
    {
        m_Sound_DisableTyaga(loco);
        sautState = EPK_ALARM_FOR_EPK;
    }

    if ( (alsn->PrevSpeed == 0.0) && (std::fabs(alsn->CurrSpeed) > 0.0) )
    {
        if (alsn->CurrSpeed)
            loco->PostTriggerCab(SAUT_sounds::Start_Drive);
        else
            loco->PostTriggerCab(SAUT_sounds::Drive_Backward);
        sautState = EPK_ALARM_FOR_EPK;
    }

    m_SELF.m_siglNext = m_getSignColor(m_getSignCode(alsn));
    if (m_SELF.m_siglPrev != m_SELF.m_siglNext)
    {
        m_SELF.m_siglPrev = m_SELF.m_siglNext;
        m_playColor(loco, m_SELF.m_siglNext);
        if (m_SELF.m_siglNext != SignColors::COLOR_GREEN)
            sautState = EPK_ALARM_FOR_EPK;
    }

    m_scanObjects(loco, 4);
    return sautState;
}

int SAUT::m_getSignCode(const st_ALSN *alsn) noexcept
{
    for (int i = 0; i < alsn->NumSigForw &&  i< SIGNALS_CNT; i++ )
    {
       if ( CHECK_BIT(alsn->ForwardSignalsList[i].Flags, 3) )
           continue;
       if ( CHECK_BIT(alsn->ForwardSignalsList[i].Flags, 1) )
           continue;

       m_SELF.Distance = (int)alsn->ForwardSignalsList[i].Distance;
       if (IS_KG(alsn->ForwardSignalsList[i].Aspect[0]))
            return SIGASP_STOP_AND_PROCEED;
       else if (IS_YELLOW(alsn->ForwardSignalsList[i].Aspect[0]))
           return SIGASP_APPROACH_2;
       else if (IS_GREEN(alsn->ForwardSignalsList[i].Aspect[0]))
           return SIGASP_CLEAR_2;
       else
           continue;
    }

    if (IS_RED(alsn->ForwardSignalsList[alsn->NumSigForw-1].Aspect[0]))
        return SIGASP_STOP ;
    return  SIGASP_RESTRICTING;
}

/**
 * @brief _getForwardSignColor Получить цвет сигнала впереди
 * @param eng
 * @return
 */
SignColors SAUT::m_getSignColor(int sig) noexcept
{
    if ( (sig == SIGASP_CLEAR_1 ) || (sig == SIGASP_CLEAR_2) )
        return SignColors::COLOR_GREEN;
    else if ( (sig == SIGASP_APPROACH_1) || (sig == SIGASP_APPROACH_2) || (sig == SIGASP_APPROACH_3) )
        return SignColors::COLOR_YELLOW;
    else if (sig == SIGASP_STOP_AND_PROCEED)
        return SignColors::COLOR_RD_YEL;
    else if (sig == SIGASP_RESTRICTING)
        return SignColors::COLOR_WHITE;
    else if ((sig == SIGASP_STOP) ||  (SIGASP_BLOCK_OBSTRUCTED))
        return SignColors::COLOR_RED;
    else
        return SignColors::COLOR_WHITE;
        //return en_SignColors::UNSET;
}

void SAUT::m_playColor(const Locomotive *loco, SignColors colour) noexcept
{
    if (colour== SignColors::COLOR_GREEN)
        loco->PostTriggerCab(SAUT_sounds::GREEN);
    else if (colour == SignColors::COLOR_YELLOW)
        loco->PostTriggerCab(SAUT_sounds::YELLOW);
    else if (colour == SignColors::COLOR_RD_YEL)
        loco->PostTriggerCab(SAUT_sounds::KG);
    else if (colour == SignColors::COLOR_RED)
        loco->PostTriggerCab(SAUT_sounds::RED);
    else if (colour == SignColors::COLOR_WHITE)
        loco->PostTriggerCab(SAUT_sounds::WHITE);
    else
    {
        //m_SoundPip(loco);
        loco->PostTriggerCab(SAUT_sounds::STO);
    }
}

/* обновляет время для начала звучаний */
void SAUT::m_updateSoundsTime() noexcept
{
    ftime(&m_SELF.currTime);
    int prevMillis = m_SELF.prevTime.time * 1000 + m_SELF.prevTime.millitm;
    int currMillis = m_SELF.currTime.time * 1000 + m_SELF.currTime.millitm;

    if (prevMillis + 500 <= currMillis)
    {
        if (m_SELF.timeForDisableForce < TIME_FOR_DISABLE_TYAGA)
            m_SELF.timeForDisableForce++;
       m_SELF.timeForPick = 1;

       if (m_SELF.timeForDisableForce < TIME_FOR_EPK_START)
            m_SELF.timeForEPKstart++;
       if (m_SELF.timeForDisableForce < TIME_FOR_EPK_BRAKE)
            m_SELF.timeForEPKbrake++;

       m_SELF.prevTime = m_SELF.currTime;
    }
}

void SAUT::m_SoundPip(const Locomotive *loco) noexcept
{
    if (m_SELF.timeForPick )
    {
        loco->PostTriggerCab(SAUT_sounds::PICK);
        m_SELF.timeForPick = 0;
    }
}

void SAUT::m_Sound_DisableTyaga(const Locomotive *loco) noexcept
{
    if (m_SELF.timeForDisableForce >= TIME_FOR_DISABLE_TYAGA )
    {
        loco->PostTriggerCab(SAUT_sounds::DISABLE_TYAGA);
        m_SELF.timeForDisableForce = 0;
    }
}

void SAUT::m_Sound_ResetTime() noexcept
{
    m_SELF.timeForPick = 0;
    m_SELF.timeForEPKstart = 0;
    m_SELF.timeForDisableForce = 0;
    m_SELF.timeForEPKbrake = 0;
}

void SAUT::m_makeDisplay(const Locomotive *loco) noexcept
{
    wchar_t  sCH[100];
    if (m_SELF.m_siglNext == SignColors::COLOR_GREEN)
        swprintf( sCH, sizeof (sCH), L"Зелён");
    else if (m_SELF.m_siglNext == SignColors::COLOR_YELLOW)
        swprintf( sCH, sizeof (sCH), L"Желт");
    else if (m_SELF.m_siglNext == SignColors::COLOR_RD_YEL)
        swprintf( sCH, sizeof (sCH), L"КЖ");
    else if (m_SELF.m_siglNext == SignColors::COLOR_RED)
        swprintf( sCH, sizeof (sCH), L"Красн");
    else
        swprintf( sCH, sizeof (sCH), L"Белый");

    swprintf(m_stateString, sizeof(m_stateString),  L"SIG: (%s) %d/%d Dist: %d",
             sCH, (int)m_SELF.Speed, (int) (m_SELF.SpeedLimit - m_SELF.Speed), m_SELF.Distance);
}

void SAUT::m_scanObjects(const Locomotive *loco, int where)
{
    TrackItemsItem *itemsList;
    UINT objsCount = 0;
    static UINT prevID = 0;
    loco->GetTrackItems(where, 4000.0, itemsList, objsCount);
    for (UINT i =0; i < objsCount; i++)
    {
        if (itemsList[0].obj->ID != prevID)
        {
            prevID = itemsList[0].obj->ID;
            int soundPlayed = 1;
            if ( itemsList[0].obj->Type == TIT_CROSS )
                loco->PostTriggerCab(SAUT_sounds::PICK);
            if ( itemsList[0].obj->Type == TIT_PLATF )
                loco->PostTriggerCab(SAUT_sounds::PLATFORM);
            else if ( itemsList[0].obj->Type == TIT_CARSPWN)
                loco->PostTriggerCab(SAUT_sounds::PEREEZD);
            else if ( itemsList[0].obj->Type == TIT_UNK)
                loco->PostTriggerCab(SAUT_sounds::NEITRAL_STATE);
            else if ( itemsList[0].obj->Type == TIT_SPEED)
                loco->PostTriggerCab(SAUT_sounds::SIGNAL);
            else if ( itemsList[0].obj->Type == TIT_SIDING)
                loco->PostTriggerCab(SAUT_sounds::SPEED_LIMIT);
            else if ( itemsList[0].obj->Type == TIT_LEVELCR)
                loco->PostTriggerCab(SAUT_sounds::STATION);
            else if ( itemsList[0].obj->Type == TIT_LEVELCR)
                loco->PostTriggerCab(SAUT_sounds::ReversUp);
            else
                soundPlayed = 0;
                //loco->PostTriggerCab(SAUT_sounds::STO);
            if (soundPlayed)
                break;
        }
    }
}
