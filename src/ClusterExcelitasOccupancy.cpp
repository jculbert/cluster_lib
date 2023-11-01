/*
 * ClusterExcelitasOccupancy.cpp
 *
 *  Created on: Jun. 2, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_EXCELITAS_OCCUPANCY

#include "em_gpio.h"
#include "gpiointerrupt.h"
#include "sl_emlib_gpio_init_serin_config.h"
#include "sl_emlib_gpio_init_motion_input_config.h"

#include "sl_udelay.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app/util/attribute-storage.h>

#include "ClusterExcelitasOccupancy.h"

#undef SILABS_LOG
#define SILABS_LOG(...)

namespace cluster_lib
{
static ClusterExcelitasOccupancy *cluster;
static bool interrupt = false;
static const unsigned INT_NUM = 4; // Note, valid values depends on pin number (see GPIO_ExtIntConfig)

static void int_callback(unsigned char intNo)
{
    if (cluster->state != ClusterExcelitasOccupancy::STATE_BLANKING)
    {
        SILABS_LOG("PIR int");
        GPIO_IntDisable(1<<INT_NUM);
        interrupt = true;
        cluster->RequestProcess(0);
    }
    else
        SILABS_LOG("PIR blanked");
}

static void inline set_serin()
{
    GPIO_PinOutSet(SL_EMLIB_GPIO_INIT_SERIN_PORT, SL_EMLIB_GPIO_INIT_SERIN_PIN);
}

static void inline clear_serin()
{
    GPIO_PinOutClear(SL_EMLIB_GPIO_INIT_SERIN_PORT, SL_EMLIB_GPIO_INIT_SERIN_PIN);
}

static void clear_direct_link()
{
    // Pull the detector interrupt output low to clear interrupt state
    GPIO_PinModeSet(SL_EMLIB_GPIO_INIT_MOTION_INPUT_PORT,
        SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN, gpioModePushPull, 0);

    // Restore for interrupt input
    GPIO_PinModeSet(SL_EMLIB_GPIO_INIT_MOTION_INPUT_PORT,
        SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN, SL_EMLIB_GPIO_INIT_MOTION_INPUT_MODE,
        SL_EMLIB_GPIO_INIT_MOTION_INPUT_DOUT);
}

static void init_detector()
{
    uint32_t config25bits = 0;

    // BPF threshold 8 bits, 24:17
    config25bits |= 100 << 17;

    // Blind Time, 4 bits, 16:13, 0.5 s and 8 s in steps of 0.5 s.
    config25bits |= 0x01 << 13; // Small value, not needed, we implement blanking logic

    // Pulse counter, 2 bits, 12:11, 1 to 4 pulses to trigger detection
    config25bits |= 1 << 11;

    // Window time, 2 bits, 10:9, 2 s up to 8 s in intervals of 2 s
    //config25bits |= 0; // min window for now

    // Operation Modes, 2 bits, 8:7
    config25bits |= 2 << 7; // 2 for Wake Up, interrupt, mode

    // Signal Source, 2 bits, 6:5,
    //config25bits |= 0; // PIR BPF

    // Reserved, 2 bits, 4:3, must be set to 2
    config25bits |= 2 << 3;

    // HPF Cut-Off, 1 bit, bit 2
    config25bits |= 0; // 0 for 0.4 Hz

    // Reserver, 1 bit, bit 1, must be set to 0
    //config25bits |= 0;

    // Count Mode, 1 bit, bit 0
    config25bits |= 1; // no zero crossing required

    // Send the 25 bit bit train in a loop
    // Start of each bit is marked by low to high
    // for a 0 value, output returns to zero after start pulse
    // for a 1 value, output is held for 80 usec or more
    clear_serin();
    sl_udelay_wait(1000); // Start output low for 1 ms

    const int32_t num_bits = 25;
    uint32_t mask = 1 << 24;
    taskENTER_CRITICAL();
    for (int32_t i = 0; i < num_bits; i++)
    {
        clear_serin();
        sl_udelay_wait(5);
        set_serin();
        sl_udelay_wait(5);

        if ((config25bits & mask) == 0)
        {
            // bit value is zero
            clear_serin();
        }
        sl_udelay_wait(100); // hold bit value
        mask >>= 1;
    }
    taskEXIT_CRITICAL();

    // Latch the data
    clear_serin();
    sl_udelay_wait(1000);
    SILABS_LOG("PIR init done");
}

static void UpdateClusterState(intptr_t notused)
{
    // State is a bitmap, bit 0 is for occupancy
    SILABS_LOG("PIR state %d", cluster->occupancy);
    chip::app::Clusters::OccupancySensing::Attributes::Occupancy::Set(cluster->endpoint, cluster->occupancy ? 1: 0);
}

ClusterExcelitasOccupancy::ClusterExcelitasOccupancy (uint32_t _endpoint, uint32_t _timeout, PostEventCallback _postEventCallback)
    : ClusterWorker(_endpoint, _postEventCallback), occupancy(false), state(STATE_IDLE), timeout(1000 * _timeout), blankingTime(3000 * _timeout / 4)
{
    cluster = this;
    init_detector();

    // Note, the input GPIO has been setup by app config to input no pull
    GPIOINT_CallbackRegister(INT_NUM, int_callback);
    GPIO_ExtIntConfig(SL_EMLIB_GPIO_INIT_MOTION_INPUT_PORT, SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN, INT_NUM, 1, 0, true); // rising edge only, interrupt enabled
    clear_direct_link();
}

ClusterExcelitasOccupancy::~ClusterExcelitasOccupancy ()
{
}

void ClusterExcelitasOccupancy::Process(const AppEvent * event)
{
    SILABS_LOG("PIR Process");

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

      SILABS_LOG("PIR Blanking");
      RequestProcess(blankingTime); // Start/restart blanking timeout, leave interrupts disabled
      return;
    }

    // Called due to timer
    switch (state)
    {
    case STATE_BLANKING:
        // End of blanking period
        clear_direct_link();
        GPIO_IntClear(1<<INT_NUM); // Clear any spurious interrupt
        GPIO_IntEnable(1<<INT_NUM);
        state = STATE_DELAY;
        SILABS_LOG("PIR Delay");
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

#endif //CLUSTER_PANASONIC_OCCUPANCY
