/*
 * ClusterTemperature.cpp
 *
 *  Created on: Jun. 11, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_TEMPERATURE

#include "sl_i2cspm_instances.h"
#include "sl_si70xx.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app/util/attribute-storage.h>

#include "ClusterTemperature.h"

#undef SILABS_LOG
#define SILABS_LOG(...)

namespace cluster_lib
{
static ClusterTemperature *cluster;

static void UpdateClusterState(intptr_t notused)
{
    SILABS_LOG("Temperature %d", cluster->temperature);
    chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(cluster->endpoint, cluster->temperature);
}

ClusterTemperature::ClusterTemperature (uint32_t _endpoint, PostEventCallback _postEventCallback)
    : ClusterWorker(_endpoint, _postEventCallback), temperature(0)
{
    cluster = this;
    sl_status_t status = sl_si70xx_init(sl_i2cspm_inst0, SI7021_ADDR);
    SILABS_LOG("sl_si7021_init status = %d\n", status);

    // First run
    RequestProcess(10000);
}

ClusterTemperature::~ClusterTemperature ()
{
}

void ClusterTemperature::Process(const AppEvent * event)
{
    SILABS_LOG("Temperature Process");

    int32_t temp_data;
    uint32_t rh_data;
    sl_si70xx_measure_rh_and_temp(sl_i2cspm_inst0, SI7021_ADDR, &rh_data, &temp_data);
    cluster->temperature = (int16_t) (temp_data / 10);
    SILABS_LOG("si7021: temp %d, RH %d\n", temp_data, rh_data);
    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusterState, reinterpret_cast<intptr_t>(nullptr));

    RequestProcess(5*60*1000);
}

} /* namespace cluster_lib */

#endif
