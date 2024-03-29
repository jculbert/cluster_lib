/*
 * ClusterAirQuality.cpp
 *
 *  Created on: Feb. 27, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_AIRQUALITY

#include "em_gpio.h"
#include "gpiointerrupt.h"
#include "sl_emlib_gpio_init_AIRQUALITY_INT_config.h"

#include <app-common/zap-generated/af-structs.h>
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app-common/zap-generated/cluster-objects.h>

#include <app/util/attribute-storage.h>

#include "ClusterAirQuality.h"

#include "sl_i2cspm_instances.h"
#include "sl_ccs811.h"
#include "sl_status.h"

namespace cluster_lib
{
static ClusterAirQuality *clusterWorker;

static const unsigned INT_NUM = 13; // Note, valid values depends on pin number (see GPIO_ExtIntConfig)

static void int_callback(unsigned char intNo)
{
    clusterWorker->RequestProcess(0);
}

static void UpdateClusterState(intptr_t notused)
{
    chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Set(clusterWorker->endpoint, clusterWorker->airquality);
}

sl_status_t status; // Putting this here so that debugger can show value
ClusterAirQuality::ClusterAirQuality (uint32_t endpoint, PostEventCallback _postEventCallback)
    : ClusterWorker(endpoint, _postEventCallback), airquality(0)
{
    clusterWorker = this;
    GPIO_IntDisable(1<<INT_NUM);
    GPIO_ExtIntConfig(SL_EMLIB_GPIO_INIT_AIRQUALITY_INT_PORT, SL_EMLIB_GPIO_INIT_AIRQUALITY_INT_PIN, INT_NUM, 0, 1, false); // falling edge only, interrupt disabled
    GPIOINT_CallbackRegister(INT_NUM, int_callback);

    status = sl_ccs811_init(sl_i2cspm_sensor);
    EFR32_LOG("sl_ccs811_init %d", status);
    if (status != SL_STATUS_OK)
        return; // For checking in debugger
    status = sl_ccs811_set_measure_mode(sl_i2cspm_sensor, CCS811_MEASURE_MODE_DRIVE_MODE_60SEC | CCS811_MEASURE_MODE_INTERRUPT);
    EFR32_LOG("set_measure_mode %d", status);
    if (status != SL_STATUS_OK)
        return; // For checking in debugger

    GPIO_IntEnable(1<<INT_NUM);
}

ClusterAirQuality::~ClusterAirQuality ()
{
}

static uint8_t state;
void ClusterAirQuality::Process()
{
    status = sl_ccs811_get_status(sl_i2cspm_sensor, &state);

    if ( !sl_ccs811_is_data_available(sl_i2cspm_sensor) )
    {
        // Check again after a delay
        // This fixes the problem where an interrupt occurs immediately after initializing the chip
        // but data is not ready at this point.
        // Things work correctly after the first reading completes, so we don't get to this code.
        RequestProcess(1000);
        return;
    }

    uint16_t eco2, tvoc;
    sl_ccs811_get_measurement(sl_i2cspm_sensor, &eco2, &tvoc);
    // The tvoc value range is 0 ppb to 1187 ppb
    // We map this to humidity value, format percent * 100;
    // So just multiply tvoc value by 10
    airquality = 10*tvoc;
    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusterState, reinterpret_cast<intptr_t>(nullptr));

    // For safety we request process with delay greater than measure period.
    // This should be always superceded be the measurement complete interrupt
    RequestProcess(90*1000);
}

} /* namespace cluster_lib */

#endif // CLUSTER_AIRQUALITY
