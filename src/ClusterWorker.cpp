/*
 * Cluster.cpp
 *
 *  Created on: Feb. 9, 2023
 *      Author: jeff
 */


#include "ClusterWorker.h"

namespace cluster_lib {

void PostEventHandler(AppEvent *event)
{
    ( (ClusterWorker*)(event->ClusterWorkerEvent.Context) )->Process(event);
}

void TimerEventHandler(TimerHandle_t timer)
{
    // Just call RequestProcess with zero delay
    ( (ClusterWorker*) (pvTimerGetTimerID(timer)) )->RequestProcess(0);
}

ClusterWorker::ClusterWorker(uint32_t _endpoint, PostEventCallback _postEventCallback)
  : endpoint(_endpoint), postEventCallback(_postEventCallback)
{
    timerHandle = xTimerCreate("ClusterWorker", 10, false, this, TimerEventHandler);
}

void ClusterWorker::RequestProcess(uint32_t delayms)
{
    if (delayms == 0)
    {
        AppEvent event;
        event.Type               = AppEvent::kEventType_ClusterWorker;
        event.ClusterWorkerEvent.Context = (void *) this;
        event.ClusterWorkerEvent.SubType = EventSubTypeBase;
        event.Handler            = PostEventHandler;
        postEventCallback(&event);
        return;
    }

    // Start timer to call RequestProcess process again after delay
    if (xPortIsInsideInterrupt())
    {
        BaseType_t higherPrioTaskWoken = pdFALSE;;
        xTimerChangePeriodFromISR(timerHandle, pdMS_TO_TICKS(delayms), &higherPrioTaskWoken);
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
        xTimerChangePeriod(timerHandle, pdMS_TO_TICKS(delayms), 100);
        xTimerStart(timerHandle, 100);
    }
}
}
