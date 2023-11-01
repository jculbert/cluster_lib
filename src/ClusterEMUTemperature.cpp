/*
 * ClusterEMUTemperature.cpp
 *
 *  Created on: Sep. 2, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_EMUTEMPERATURE

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app/util/attribute-storage.h>

#include <em_emu.h>

#include "ClusterEMUTemperature.h"

namespace cluster_lib
{
static ClusterEMUTemperature *cluster;

static void UpdateClusterState(intptr_t notused)
{
    chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(cluster->endpoint, cluster->temperature);
}

void ClusterEMUTemperature::SetTemperatureFromRaw(float rt)
{
    // Apply the correction from the MGM240P reference manual
    // T corr = a*x^^3 + b*x^^2 + c*x + d
    const float a = -9.939E-7;
    const float b = -1.526E-4;
    const float c = 1.040;
    const float d = -3.577;
    float rt2 = rt * rt;
    float rt3 = rt2 * rt;

    float t = a*rt3 + b*rt2 + c*rt + d - temperature_offset;
    temperature = (int16_t) (t * 100);
    SILABS_LOG("Temperature %d", temperature);
    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusterState, reinterpret_cast<intptr_t>(nullptr));
}

ClusterEMUTemperature::ClusterEMUTemperature(uint32_t _endpoint, PostEventCallback _postEventCallback,
                                             int32_t _num_samples, int32_t _sample_period_ms, float _temperature_offset)
: ClusterWorker(_endpoint, _postEventCallback), num_samples(_num_samples), sample_period_ms(_sample_period_ms),
  temperature_offset(_temperature_offset), temperature(0), raw_sum(0.0), raw_count(-1)
{
    cluster = this;
    RequestProcess(5000);
}

ClusterEMUTemperature::~ClusterEMUTemperature()
{
}

void ClusterEMUTemperature::Process(const AppEvent * event)
{
    float raw = EMU_TemperatureGet();

    if (raw_count == -1)
    {
        // First run special case, set temperature to first reading
        raw_count = 0;
        SetTemperatureFromRaw(raw);
    }

    raw_sum += raw;
    if (++raw_count == num_samples)
    {
        // New average value
        SetTemperatureFromRaw(raw_sum / num_samples);

        raw_count = 0;
        raw_sum = 0.0;
    }

    RequestProcess(sample_period_ms);
}

}

#endif // CLUSTER_EMUTEMPERATURE
