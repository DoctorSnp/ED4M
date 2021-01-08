/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef EPK_H
#define EPK_H

#include "ts.h"
#include "sys/timeb.h"

typedef enum en_EPKState
{
    EPK_Normal = 0,
    EPK_Svist = 1,
    EPK_ACTIVATING = 2,
}en_EPKState;

class EPK
{
public:
    EPK();
    int init();
    void setEnabled(const Locomotive *loco, int isEnabled);
    void okey(const Locomotive *loco);
    int step(const Locomotive *loco, int state);
private:
    en_EPKState m_state = EPK_Normal;
    struct timeb prevTime;
    struct timeb currTime;
};

#endif // EPK_H
