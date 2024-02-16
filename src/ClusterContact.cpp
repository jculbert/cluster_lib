/*
 * ClusterContact.cpp
 *
 *  Created on: May 25, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_CONTACT

#include "em_gpio.h"
#include "gpiointerrupt.h"
#include "sl_i2cspm_instances.h"
#include "sl_si7210.h"

#include "ClusterContact.h"

#include "sl_emlib_gpio_init_hall_input_config.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app/util/attribute-storage.h>

static cluster_lib::ClusterContact *cluster;

static void int_callback(unsigned char intNo)
{
    SILABS_LOG("hall int");
    cluster->state = GPIO_PinInGet(SL_EMLIB_GPIO_INIT_HALL_INPUT_PORT, SL_EMLIB_GPIO_INIT_HALL_INPUT_PIN) == 0 ? false: true;
    cluster->RequestProcess(0);
}

static sl_status_t hall_init()
{
    sl_status_t status = sl_si7210_init(sl_i2cspm_inst0);
    SILABS_LOG("sl_si7210_init: status %d", status);
    if (status == SL_STATUS_OK)
    {
        sl_si7210_configure_t config = {};
        config.threshold = 0.5;
        config.hysteresis = config.threshold / 5.0;

        // Configure sets threshold and histeresis and enables sleep with periodic measurements
        status = sl_si7210_configure(sl_i2cspm_inst0, &config);
        SILABS_LOG("sl_si7210_configure: status %d", status);
    }
    return status;
}

static void update_cluster_state(intptr_t notused)
{
    // State is a bitmap, bit 0 is for occupancy
    chip::app::Clusters::BooleanState::Attributes::StateValue::Set(cluster->endpoint, cluster->state);
}

static void read_hall()
{
  float value = 0.123;
  sl_status_t status = sl_si7210_measure(sl_i2cspm_inst0, 1000, &value);
  SILABS_LOG("read_hall: status %d, value %d", status, (int32_t)(value * 1000));

  // Had trouble getting periodic reading to work and calling
  // measure to we completely init hall after a reading
  hall_init();
}

namespace cluster_lib
{

ClusterContact::ClusterContact (uint32_t _endpoint, PostEventCallback _postEventCallback)
  : ClusterWorker(_endpoint, _postEventCallback), state(false)
{
    cluster = this;

    // We configure the SI7210 to measure periodically and control output according to a threshold
    // an interrupt will occur for each change in this output

    const int INT_NUM = 4; // Note, valid values depends on pin number (see GPIO_ExtIntConfig)
    GPIOINT_CallbackRegister(INT_NUM, int_callback);
    GPIO_ExtIntConfig(SL_EMLIB_GPIO_INIT_HALL_INPUT_PORT, SL_EMLIB_GPIO_INIT_HALL_INPUT_PIN, INT_NUM, 1, 1, true); // rising and falling edge, interrupt enabled

    hall_init();
}

ClusterContact::~ClusterContact ()
{
}

void ClusterContact::Process(const AppEvent * event)
{
    switch (event->ClusterWorkerEvent.SubType)
    {
        case EventSubTypeBase:
            chip::DeviceLayer::PlatformMgr().ScheduleWork(update_cluster_state, reinterpret_cast<intptr_t>(nullptr));
            break;

        case EventSubTypeReadHall:
            read_hall();
            break;

        default:
            // Should not happen
            break;
    }
}

void ClusterContact::RequestHallRead()
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_ClusterWorker;
    event.ClusterWorkerEvent.Context = (void *) this;
    event.ClusterWorkerEvent.SubType = EventSubTypeReadHall;
    event.Handler            = PostEventHandler;
    postEventCallback(&event);
}

} /* namespace cluster_lib */

#endif // CLUSTER_CONTACT
