/*
 * ClusterGarageDoor.cpp
 *
 *  Created on: Jul. 18, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_GARAGE_DOOR

#include "em_gpio.h"
#include "gpiointerrupt.h"
#include "sl_emlib_gpio_init_contact_input_config.h"
#include "sl_emlib_gpio_init_relay_output_config.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/window-covering-server/window-covering-server.h>

#include <app/util/attribute-storage.h>

#ifdef CLUSTER_GARAGE_DOOR_LIGHT
#include <app/clusters/on-off-server/on-off-server.h>
#endif

#include "ClusterGarageDoor.h"

//#undef SILABS_LOG
//#define SILABS_LOG(...)

#ifndef CLUSTER_GARAGE_DOOR_LIGHT
using namespace chip::app::Clusters::WindowCovering;
#endif

namespace cluster_lib
{

static ClusterGarageDoor *cluster;

static const unsigned INT_NUM = 4; // Note, valid values depends on pin number (see GPIO_ExtIntConfig)

static void int_callback(unsigned char intNo)
{
    SILABS_LOG("Contact interrupt");
    cluster->contact_int = true;

    // Request processing if a relay pulse is not in progress
    // Delay a small amount to allow for bounce
    if (!cluster->relay_closed)
      cluster->RequestProcess(200);
}

static void UpdateClusterState(intptr_t notused)
{
    SILABS_LOG("Update door_closed = %d", cluster->door_closed);

#ifdef CLUSTER_GARAGE_DOOR_LIGHT
    OnOffServer::Instance().setOnOffValue(cluster->endpoint, !cluster->door_closed, false);
#else
    chip::Percent100ths percent100ths = cluster->door_closed ? 10000: 0;

    NPercent100ths current;
    current.SetNonNull(percent100ths);
    Attributes::CurrentPositionLiftPercent100ths::Set(cluster->endpoint, current);
#endif
}

ClusterGarageDoor::ClusterGarageDoor (uint32_t _endpoint, PostEventCallback _postEventCallback)
    : ClusterWorker(_endpoint, _postEventCallback),
      command(COMMAND_NONE), relay_closed(false), contact_int(false), pulse_relay_duration(500),
      target_changed(false), contact_ms(0)
{
    cluster = this;
    // Note, the contact input GPIO has been setup by app config to input with pull hight
    GPIOINT_CallbackRegister(INT_NUM, int_callback);
    GPIO_ExtIntConfig(SL_EMLIB_GPIO_INIT_CONTACT_INPUT_PORT, SL_EMLIB_GPIO_INIT_CONTACT_INPUT_PIN, INT_NUM, 1, 1, true); // rising and falling edge, interrupt enabled

    door_closed = GPIO_PinInGet(SL_EMLIB_GPIO_INIT_CONTACT_INPUT_PORT, SL_EMLIB_GPIO_INIT_CONTACT_INPUT_PIN) == 1;
    RequestProcess(30000);
}

ClusterGarageDoor::~ClusterGarageDoor ()
{
}

void ClusterGarageDoor::Process(const AppEvent * event)
{
    SILABS_LOG("Garage Process");

#if 0
    static bool state = false;
    if (state)
        GPIO_PinOutSet(SL_EMLIB_GPIO_INIT_RELAY_OUTPUT_PORT, SL_EMLIB_GPIO_INIT_RELAY_OUTPUT_PIN);
    else
        GPIO_PinOutClear(SL_EMLtarget_changedIB_GPIO_INIT_RELAY_OUTPUT_PORT, SL_EMLIB_GPIO_INIT_RELAY_OUTPUT_PIN);

    state = !state;
    RequestProcess(5000);
#endif

    // Request for periodic refresh
    // This could get overriden to a shorter value below
    RequestProcess(30000);

    if (relay_closed)
    {
        // We were called due to relay pulse duration
        relay_closed = false;
        GPIO_PinOutClear(SL_EMLIB_GPIO_INIT_RELAY_OUTPUT_PORT, SL_EMLIB_GPIO_INIT_RELAY_OUTPUT_PIN);
        SILABS_LOG("Pulse end");

        if (!cluster->contact_int)
        {
            // Contact change has not occurred yet.
            // Don't continue to below which will update state to old state
            return;
        }
    }

    if (cluster->contact_int)
    {
        cluster->contact_int = false;
        contact_ms = chip::System::SystemClock().GetMonotonicMilliseconds64().count();
    }

    door_closed = GPIO_PinInGet(SL_EMLIB_GPIO_INIT_CONTACT_INPUT_PORT, SL_EMLIB_GPIO_INIT_CONTACT_INPUT_PIN) == 1;
    SILABS_LOG("door_closed = %d", door_closed);

    if (target_changed)
    {
        target_changed = false;
#ifdef CLUSTER_GARAGE_DOOR_LIGHT
        // Ignore changed state within a blanking period after the most recent contact interrupt
        // This avoids false triggers due to state bouncing before end of movement.
        uint64_t delta_ms = chip::System::SystemClock().GetMonotonicMilliseconds64().count() - contact_ms;
        if (delta_ms > 5000)
        {
            bool on;
            chip::DeviceLayer::PlatformMgr().LockChipStack();
            OnOffServer::Instance().getOnOffValue(endpoint, &on);
            chip::DeviceLayer::PlatformMgr().UnlockChipStack();
            SILABS_LOG("Garage target_onoff %d", on);
            if ((on && door_closed) || (!on && !door_closed))
                command = on ? COMMAND_OPEN: COMMAND_CLOSE;
        }
        else
        {
            SILABS_LOG("Target changed ignored");
            RequestProcess((uint32_t)(delta_ms + 100)); // run again at end of blanking time
        }
#else
        NPercent100ths target;
        chip::DeviceLayer::PlatformMgr().LockChipStack();
        EmberAfStatus status = Attributes::TargetPositionLiftPercent100ths::Get(endpoint, target);
        chip::DeviceLayer::PlatformMgr().UnlockChipStack();
        if ((status == EMBER_ZCL_STATUS_SUCCESS) && !target.IsNull())
        {
           SILABS_LOG("Garage target_lift %d", target.Value());
           command = target.Value() == 0 ? COMMAND_OPEN: COMMAND_CLOSE;
        }
#endif
    }

    if (command != COMMAND_NONE)
    {
        SILABS_LOG("Process: %s", command == COMMAND_OPEN ? "OPEN": "CLOSE");
        command = COMMAND_NONE;
        relay_closed = true;
        GPIO_PinOutSet(SL_EMLIB_GPIO_INIT_RELAY_OUTPUT_PORT, SL_EMLIB_GPIO_INIT_RELAY_OUTPUT_PIN);
        RequestProcess(pulse_relay_duration);
        return; // Don't continue to state update below.
    }

    // Always schedule to update state. Will only cause an action if state is different from stored value
    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusterState, reinterpret_cast<intptr_t>(nullptr));
}

void ClusterGarageDoor::NotifyAttributeChange(const chip::app::ConcreteAttributePath & attributePath)
{
#ifdef CLUSTER_GARAGE_DOOR_LIGHT
    chip::app::ConcreteAttributePath targetOnOff(cluster->endpoint,
        chip::app::Clusters::OnOff::Id, chip::app::Clusters::OnOff::Attributes::OnOff::Id);

    if (attributePath == targetOnOff)
    {
        cluster->target_changed = true;
        cluster->RequestProcess(100); // small delay to allow notification to complete
    }
#else
    chip::app::ConcreteAttributePath targetLiftPath(cluster->endpoint,
        chip::app::Clusters::WindowCovering::Id, chip::app::Clusters::WindowCovering::Attributes::TargetPositionLiftPercent100ths::Id);

    if (attributePath == targetLiftPath)
    {
        cluster->target_changed = true;
        cluster->RequestProcess(100); // small delay to allow notification to complete
    }
#endif
}

} // namespace cluster_lib
#endif // CLUSTER_GARAGE_DOOR
