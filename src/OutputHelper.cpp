/*
 * LEDBlinker.cpp
 *
 *  Created on: Nov. 14, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef LEDBLINKER

#include <OutputHelper.h>

namespace cluster_lib
{
    void OutputHelper::_TimerEventHandler(TimerHandle_t timer)
    {
        ((OutputHelper*) pvTimerGetTimerID(timer))->TimerEventHandler();
    }

    void OutputHelper::TimerEventHandler()
    {
        if (state)
        {
            Set(false);
            // If num_pulses is not 1, set a timer for this off period
            // which will trigger the next on period
            // Else we are done for this cycle
            if (num_pulses != 1)
                StartTimer();
        }
        else
        {
            // Start the next on period
            Set(true);
            StartTimer();

            // Note, if num_pulses is zero here we are pulsing indefinitely
            // so don't decrement the pulse counter
            if (num_pulses > 1)
                --num_pulses;
        }
    }

    OutputHelper::OutputHelper()
            : state(false), num_pulses(0)
    {
        // Note, the period set here is not actually used
        // The period will be set before starting the timer
        timerHandle = xTimerCreate("OutputHelper", pdMS_TO_TICKS(1000), false, this, _TimerEventHandler);
    }

    OutputHelper::~OutputHelper()
    {
    }

    void OutputHelper::Set(bool _state)
    {
        state = _state;
        SetOutput(_state);
    }

    void OutputHelper::Pulse(uint32_t _num_pulses, uint32_t period_ms)
    {
        num_pulses = _num_pulses;
        Set(true);
        xTimerChangePeriod(timerHandle, pdMS_TO_TICKS(period_ms), 100);
        StartTimer();
    }

    void OutputHelper::StartTimer()
    {
        if (xPortIsInsideInterrupt())
        {
            BaseType_t higherPrioTaskWoken = pdFALSE;;
            xTimerStartFromISR(timerHandle, &higherPrioTaskWoken);

            // Copied the following from BaseApplication::PostEvent
#ifdef portYIELD_FROM_ISR
            portYIELD_FROM_ISR(higherPrioTaskWoken);
#elif portEND_SWITCHING_ISR // portYIELD_FROM_ISR or portEND_SWITCHING_ISR
            portEND_SWITCHING_ISR(higherPrioTaskWoken);
#else                       // portYIELD_FROM_ISR or portEND_SWITCHING_ISR
#error "Must have portYIELD_FROM_ISR or portEND_SWITCHING_ISR"
#endif // portYIELD_FROM_ISR or portEND_SWITCHING_ISR
        }
        else
        {
            xTimerStart(timerHandle, 100);
        }
    }

} /* namespace cluster_lib */

#endif // LEDBLINKER
