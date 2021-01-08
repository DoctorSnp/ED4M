/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#ifndef EPK_H
#define EPK_H

#include "ts.h"
#include "src/shared_structs.h"

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
    int step(const Locomotive *loco, int state, st_gameTime currTime);
private:
    en_EPKState m_state = EPK_Normal;
    st_gameTime m_prevTime = {};
    st_gameTime m_currTime = {};
};

#endif // EPK_H
