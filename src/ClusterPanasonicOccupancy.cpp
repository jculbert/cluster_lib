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
#include "sl_simple_button.h"
#include "sl_emlib_gpio_init_motion_input_config.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app/util/attribute-storage.h>

#include <platform/KeyValueStoreManager.h>

#include <ClusterPanasonicOccupancy.h>

//#undef SILABS_LOG
//#define SILABS_LOG(...)

namespace cluster_lib {

static const char* kvs_key = "OccupancyTimeout";

static ClusterPanasonicOccupancy *cluster;
static bool interrupt = false;

static void int_callback(unsigned char intNo)
{
    SILABS_LOG("Occupancy interrupt");
    if (cluster->state != ClusterPanasonicOccupancy::STATE_BLANKING)
    {
        GPIO_IntDisable(1<<SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN);
        interrupt = true;
        cluster->RequestProcess(0);
    }
}

static void UpdateClusterState(intptr_t notused)
{
    // State is a bitmap, bit 0 is for occupancy
    chip::app::Clusters::OccupancySensing::Attributes::Occupancy::Set(cluster->endpoint, cluster->occupancy ? 1: 0);
}

void ClusterPanasonicOccupancy::SetBlankingTime()
{
    blanking_time_ms = (3 * timeout_ms) / 4;
}

ClusterPanasonicOccupancy::ClusterPanasonicOccupancy(uint32_t endpoint, uint32_t _timeout, uint32_t _max_timeout, uint8_t led, PostEventCallback _postEventCallback)
  : ClusterWorker(endpoint, _postEventCallback), occupancy(false), state(STATE_IDLE),
    timeout_ms(60000 * _timeout), max_timeout_ms(60000 * _max_timeout), led_output(NULL)
{
    // Note, the input GPIO has been setup by app config to input with pull low
    GPIO_ExtIntConfig(SL_EMLIB_GPIO_INIT_MOTION_INPUT_PORT, SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN, SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN, 1, 0, true); // rising edge only, interrupt enabled
    GPIOINT_CallbackRegister(SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN, int_callback);
    cluster = this;

    // Try to read the kvs timeout value
    // Use if there, otherwise init to the constructor value
    uint32_t timeout_kvs;
    CHIP_ERROR err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(kvs_key, &timeout_kvs, sizeof(timeout_kvs));
    if (err == CHIP_NO_ERROR)
    {
        SILABS_LOG("Restoring timeout value from kvs %d", timeout_kvs);
        timeout_ms = timeout_kvs;
    }
    else
    {
        SILABS_LOG("Init kvs timeout value %d", timeout_ms);
        err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(kvs_key, &timeout_ms, sizeof(timeout_ms));
        if (err != CHIP_NO_ERROR)
            SILABS_LOG("Error writing kvs timeout value %d", err);
    }

    SetBlankingTime();

    // The LED output, if configured, is used to display blanking time
    // by pulsing it with count set to number of minutes
    if (led != 0xff)
        led_output = new LedOutput(led);
}

void ClusterPanasonicOccupancy::ButtonHandler(AppEvent * aEvent)
{
    cluster->_ButtonHandler(aEvent);
}

void ClusterPanasonicOccupancy::_ButtonHandler(AppEvent * aEvent)
{
    // Implement debounce by ignoring any back to back events within a small period
    static uint64_t last_ms = 0;
    uint64_t now = chip::System::SystemClock().GetMonotonicMilliseconds64().count();
    if ((now - last_ms) < 20)
        return;

    last_ms = now;

    static uint64_t pressed_ms = 0;
    if (aEvent->ButtonEvent.Action == SL_SIMPLE_BUTTON_PRESSED)
    {
        pressed_ms = now;
    }
    else if (aEvent->ButtonEvent.Action == SL_SIMPLE_BUTTON_RELEASED)
    {
        if ((now - pressed_ms) >= 2000)
        {
            // long press, change to the next time_out multiple of minutes
            timeout_ms += 60000;
            if (timeout_ms >= max_timeout_ms)
                timeout_ms = 60000;

            SILABS_LOG("Changed timeout to %d", timeout_ms);

            SetBlankingTime();

            // Update kvs value
            CHIP_ERROR err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(kvs_key, &timeout_ms, sizeof(timeout_ms));
            if (err != CHIP_NO_ERROR)
                SILABS_LOG("Error writing kvs timeout value %d", err);
        }
        else
        {
            SILABS_LOG("Timeout is %d", timeout_ms);
        }

        if (led_output != NULL)
        {
            // Display the delay time by flashing the led once for each 1 minute
            led_output->Pulse(timeout_ms/60000, 500);
        }
    }
}

void ClusterPanasonicOccupancy::Process(const AppEvent * event)
{
    SILABS_LOG("Occupancy process");

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

      RequestProcess(blanking_time_ms); // Start/restart blanking timeout, leave interrupts disabled
      return;
    }

    // Called due to timer
    switch (state)
    {
    case STATE_BLANKING:
        // End of blanking period
        GPIO_IntClear(1<<SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN); // Clear any spurious interrupt
        GPIO_IntEnable(1<<SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN);
        state = STATE_DELAY;
        RequestProcess(timeout_ms - blanking_time_ms); // Delay state until the end of delay period
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
