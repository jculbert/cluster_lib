/*
 * LEDBlinker.h
 *
 *  Created on: Nov. 14, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_OUTPUTHELPER_H_
#define CLUSTER_LIB_OUTPUTHELPER_H_

#include <stdint.h>

#include "AppEvent.h"
#include "FreeRTOS.h"
#include "timers.h"

namespace cluster_lib
{
    class OutputHelper
    {
    public:
        OutputHelper ();

        virtual
        ~OutputHelper ();

        // Set num_pulses to zero for indefinite
        // Note, can only be called from task level
        void Pulse(uint32_t num_pulses, uint32_t period_ms);

        void Set(bool state);

        // Function to set actual output
        virtual void SetOutput(bool state) = 0;

    private:
        bool state;
        uint32_t num_pulses;
        TimerHandle_t timerHandle;

        static void _TimerEventHandler(TimerHandle_t timer);
        void TimerEventHandler();

        void StartTimer();
    };

} /* namespace cluster_lib */

#endif /* CLUSTER_LIB_OUTPUTHELPER_H_ */
