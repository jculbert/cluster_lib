/*
 * ClusterEMUTemperature.h
 *
 *  Created on: Sep. 2, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_CLUSTEREMUTEMPERATURE_H_
#define CLUSTER_LIB_CLUSTEREMUTEMPERATURE_H_

#include "AppConfig.h"

#include <ClusterWorker.h>

namespace cluster_lib
{

class ClusterEMUTemperature : public cluster_lib::ClusterWorker
{
public:
    int32_t num_samples;
    int32_t sample_period_ms;
    float temperature_offset;
    int16_t temperature;
    float raw_sum;
    int32_t raw_count;

    ClusterEMUTemperature (uint32_t _endpoint, PostEventCallback _postEventCallback, int32_t num_samples, int32_t sample_period_ms,
                           float temperature_offset);
    virtual
    ~ClusterEMUTemperature ();

    void SetTemperatureFromRaw(float rt);

    void Process(const AppEvent * event);
};

}
#endif /* CLUSTER_LIB_CLUSTEREMUTEMPERATURE_H_ */
