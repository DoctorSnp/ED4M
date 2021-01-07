/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#include "src/elements.h"

#include "epk.h"

EPK::EPK()
{

}

int EPK::init()
{
    ftime(&prevTime);
    currTime = prevTime;
    m_state = en_EPKState::EPK_Normal;
    return 1;
}

void EPK::setEnabled(const Locomotive *loco, int isEnabled)
{
    if (isEnabled == 0)
    {
        loco->PostTriggerCab(SoundsID::EPK_ALARM_S);
        m_state = en_EPKState::EPK_Normal;
    }
}

void EPK::okey(const Locomotive *loco)
{
    if (m_state == en_EPKState::EPK_Svist) // можно только если ЭПК не сорвало.
    {
        m_state = en_EPKState::EPK_Normal;
        loco->PostTriggerCab(SoundsID::EPK_ALARM_S + 1);
    }
}

int EPK::step(const Locomotive *loco, int state)
{
    ftime(&currTime);
    if ((state == EPK_ACTIVATING) || (m_state != en_EPKState::EPK_Normal ))
    {
        if (m_state == en_EPKState::EPK_ACTIVATING)
            return 1;
        if (m_state == en_EPKState::EPK_Normal)
        {
            m_state = en_EPKState::EPK_Svist;
            //ftime(&prevTime);
        }
        ftime(&currTime);
        if (prevTime.time + 5 <= currTime.time )
        {
            loco->PostTriggerCab(SoundsID::EPK_ALARM_S);
            if (prevTime.time + 12 <= currTime.time)
            {
                m_state =  en_EPKState::EPK_ACTIVATING;
                return 1;
            }
        }
    }
    else
        ftime(&prevTime);
    return 0;
}
