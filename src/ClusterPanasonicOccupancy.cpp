/*
 * ClusterPanasonicOccupancy.cpp
 *
 *  Created on: Feb. 9, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_PANASONIC_OCCUPANCY

#include "em_gpio.h"
#include "gpiointerrupt.h"
#include "sl_emlib_gpio_init_motion_input_config.h"

#include <app-common/zap-generated/af-structs.h>
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app-common/zap-generated/cluster-objects.h>

#include <app/clusters/occupancy-sensor-server/occupancy-sensor-server.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

#include <ClusterPanasonicOccupancy.h>

namespace cluster_lib {

static ClusterPanasonicOccupancy *cluster;
static bool interrupt = false;

static const unsigned INT_NUM = 3; // Note, valid values depends on pin number (see GPIO_ExtIntConfig)

static void int_callback(unsigned char intNo)
{
    if (cluster->state != ClusterPanasonicOccupancy::STATE_BLANKING)
    {
        GPIO_IntDisable(1<<INT_NUM);
        interrupt = true;
        cluster->RequestProcess(0);
    }
}

static void UpdateClusterState(intptr_t notused)
{
    // State is a bitmap, bit 0 is for occupancy
    chip::app::Clusters::OccupancySensing::Attributes::Occupancy::Set(cluster->endpoint, cluster->occupancy ? 1: 0);
}

ClusterPanasonicOccupancy::ClusterPanasonicOccupancy(uint32_t endpoint, uint32_t _timeout, PostEventCallback _postEventCallback)
  : ClusterWorker(endpoint, _postEventCallback), occupancy(false), state(STATE_IDLE), timeout(1000 * _timeout), blankingTime(3000 * _timeout / 4)
{
    // Note, the input GPIO has been setup by app config to input with pull low
    GPIO_ExtIntConfig(SL_EMLIB_GPIO_INIT_MOTION_INPUT_PORT, SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN, INT_NUM, 1, 0, true); // rising edge only, interrupt enabled
    GPIOINT_CallbackRegister(INT_NUM, int_callback);
    cluster = this;
}

void ClusterPanasonicOccupancy::Process()
{
    if (interrupt)
    {
      interrupt = false;
      if (state == STATE_IDLE)
      {
        // new motion, call parent to send motion_state
        state = STATE_BLANKING;
        occupancy = true;
        chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusterState, reinterpret_cast<intptr_t>(nullptr));
      }
      else
        state = STATE_BLANKING;

      RequestProcess(blankingTime); // Start/restart blanking timeout, leave interrupts disabled
      return;
    }

    // Called due to timer
    switch (state)
    {
    case STATE_BLANKING:
        // End of blanking period
        GPIO_IntClear(1<<INT_NUM); // Clear any spurious interrupt
        GPIO_IntEnable(1<<INT_NUM);
        state = STATE_DELAY;
        RequestProcess(timeout - blankingTime); // Delay state until the end of delay period
        break;
    case STATE_DELAY:
        state = STATE_IDLE;
        occupancy = false;
        chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusterState, reinterpret_cast<intptr_t>(nullptr));
        break;
    case STATE_IDLE:
    default:
        break;
    }
}

} /* namespace cluster_lib */

#endif // CLUSTER_PANASONIC_OCCUPANCY
