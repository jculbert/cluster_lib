/*
 * ClusterAirQuality.h
 *
 *  Created on: Feb. 27, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_CLUSTERAIRQUALITY_H_
#define CLUSTER_LIB_CLUSTERAIRQUALITY_H_

#include "AppConfig.h"

#ifdef CLUSTER_AIRQUALITY

#include <stdint.h>

#include <ClusterWorker.h>

namespace cluster_lib
{

class ClusterAirQuality : public ClusterWorker
{
public:
    uint32_t airquality; // For now we use a humidity cluster for this, value between 0 and 100

    ClusterAirQuality (uint32_t _endpoint, PostEventCallback _postEventCallback);

    virtual
    ~ClusterAirQuality ();

    void Process();
};

} /* namespace cluster_lib */

#endif // CLUSTER_AIRQUALITY

#endif /* CLUSTER_LIB_CLUSTERAIRQUALITY_H_ */
