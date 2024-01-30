/*
 * ClusterBattery.h
 *
 *  Created on: Jan. 30, 2024
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_CLUSTERBATTERY_H_
#define CLUSTER_LIB_CLUSTERBATTERY_H_

#include "AppConfig.h"

#include "ClusterWorker.h"

namespace cluster_lib
{

class ClusterBattery : public ClusterWorker
{
private:
    uint32_t nominal_mv;
    uint32_t refresh_ms;
    bool waiting_adc;
public:
    uint8_t battery_percent;

public:
    ClusterBattery (uint32_t _endpoint, PostEventCallback _postEventCallback, uint32_t nomimal_mv, uint32_t refresh_minutes);

    void Process(const AppEvent * event);
};

} /* namespace cluster_lib */

#endif /* CLUSTER_LIB_CLUSTERBATTERY_H_ */
