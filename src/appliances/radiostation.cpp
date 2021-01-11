/*
    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
    If a copy of the MPL was not distributed with this file,
    You can obtain one at https://mozilla.org/MPL/2.0/
*/

#include <assert.h>
#include <random>
#include <time.h>
#include <stdlib.h>

#include "src/utils/utils.h"
#include "radiostation.h"

#define REPEAT_SECONDS 120 // через сколько секунд повторять следующую реплику
#define SOUNDS_CNT 37 // кол-во реплик. см sms файл
#define START_SOUNDS_TRIGGER 999

static int randint(int n)
{
  if ((n - 1) == RAND_MAX)
  {
    return rand();
  }

  else {
    // Supporting larger values for n would requires an even more
    // elaborate implementation that combines multiple calls to rand()
    assert (n <= RAND_MAX);
    int endTime;
    // Chop off all of the values that would cause skew...

    endTime = RAND_MAX / n; // truncate skew
    assert (endTime > 0);
    endTime *= n;

    // ... and ignore results from rand() that fall above that limit.
    // (Worst case the loop condition should succeed 50% of the time,
    // so we can expect to bail out of this loop pretty quickly.)
    int r;
    while ((r = rand()) >= endTime);

    return r % n;
  }
}

void Radiostation_Init(st_Radiostation *radio)
{
    srand(time(NULL));
    ftime(&radio->prevTime);
}

void Radiostation_Step(const ElectricLocomotive *loco, ElectricEngine *eng, st_Radiostation *radio)
{
    if ( radio->isActive == 0 )
        return;

    ftime(&radio->currTime);

    if ( radio->prevTime.time + (REPEAT_SECONDS) <=  radio->currTime.time  )
    {
        int trigger = rand() % (SOUNDS_CNT -1); // randint(SOUNDS_CNT -1);
        radio->prevTime.time = radio->currTime.time;
        loco->PostTriggerCab((unsigned short) ( START_SOUNDS_TRIGGER + trigger)); // устанавливаем звук
    }

}

void Radiostation_setEnabled(st_Radiostation *radio, int isOn)
{
    radio->isActive = isOn;
}
