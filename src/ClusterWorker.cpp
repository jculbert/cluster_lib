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
    ( (ClusterWorker*)(event->ClusterWorkerEvent.Context) )->Process();
}

void TimerEventHandler(TimerHandle_t timer)
{
    // Just call RequestProcess with zero delay
    ( (ClusterWorker*) (pvTimerGetTimerID(timer)) )->RequestProcess(0);
}

ClusterWorker::ClusterWorker(PostEventCallback _postEvent)
  : postEvent(_postEvent)
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
        event.Handler            = PostEventHandler;
        postEvent(&event);
        return;
    }

    // Start timer to call RequestProcess process again after delay
    xTimerStart(timerHandle, delayms);
}

}
