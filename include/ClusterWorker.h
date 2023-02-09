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

class ClusterWorker
{
private:
    PostEventCallback postEvent;
    TimerHandle_t timerHandle; // Used for delayed RequestProcess

public:
    ClusterWorker(PostEventCallback _postEvent);

    virtual void Process()
    {}

    void RequestProcess(uint32_t delayms);
};

#endif /* CLUSTER_LIB_INCLUDE_CLUSTER_H_ */

}
