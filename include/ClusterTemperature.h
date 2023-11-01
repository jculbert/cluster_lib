/*
 * ClusterTemperature.h
 *
 *  Created on: Jun. 11, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_CLUSTERTEMPERATURE_H_
#define CLUSTER_LIB_CLUSTERTEMPERATURE_H_

#include "AppConfig.h"

#include <ClusterWorker.h>

namespace cluster_lib
{

    class ClusterTemperature : public ClusterWorker
    {
    public:
        int16_t temperature;

        ClusterTemperature (uint32_t _endpoint, PostEventCallback _postEventCallback);
        virtual
        ~ClusterTemperature ();

        void Process(const AppEvent * event);
    };

} /* namespace cluster_lib */

#endif /* CLUSTER_LIB_CLUSTERTEMPERATURE_H_ */
