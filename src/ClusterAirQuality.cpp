/*
 * ClusterAirQuality.cpp
 *
 *  Created on: Feb. 27, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_AIRQUALITY

#include <app-common/zap-generated/af-structs.h>
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app-common/zap-generated/cluster-objects.h>

#include "ClusterAirQuality.h"

namespace cluster_lib
{

ClusterAirQuality::ClusterAirQuality (uint32_t endpoint, PostEventCallback _postEventCallback)
    : ClusterWorker(endpoint, _postEventCallback), airquality(0)
{
}

ClusterAirQuality::~ClusterAirQuality ()
{
}

void ClusterAirQuality::Process()
{

}

} /* namespace cluster_lib */

#endif // CLUSTER_AIRQUALITY
