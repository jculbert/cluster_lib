/*
 * Cluster.h
 *
 *  Created on: Feb. 9, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_INCLUDE_CLUSTER_H_
#define CLUSTER_LIB_INCLUDE_CLUSTER_H_

#include <stdint.h>

#include "AppEvent.h"
#include "FreeRTOS.h"
#include "timers.h"

namespace cluster_lib {

typedef void (*PostEventCallback)(const AppEvent * event);

void PostEventHandler(AppEvent *event);

class ClusterWorker
{
public:
    PostEventCallback postEventCallback;
    TimerHandle_t timerHandle; // Used for delayed RequestProcess

    // Base value for ClusterWorkerEvent SubType
    static const int8_t EventSubTypeBase = 0;

    uint32_t endpoint;
    ClusterWorker(uint32_t endpoint, PostEventCallback _postEventCallback);

    virtual void Process(const AppEvent * event) = 0;

    void RequestProcess(uint32_t delayms);
};

}

#endif /* CLUSTER_LIB_INCLUDE_CLUSTER_H_ */

