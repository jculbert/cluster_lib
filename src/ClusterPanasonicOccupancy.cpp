/*
 * ClusterPanasonicOccupancy.cpp
 *
 *  Created on: Feb. 9, 2023
 *      Author: jeff
 */

#include "em_gpio.h"
#include "gpiointerrupt.h"
#include "sl_emlib_gpio_init_motion_input_config.h"

#include <ClusterPanasonicOccupancy.h>

namespace cluster_lib {

static ClusterPanasonicOccupancy *cluster;
static bool interrupt = false;

static const unsigned INT_NUM = 9;

static void int_callback(unsigned char intNo)
{
    if (cluster->state != ClusterPanasonicOccupancy::STATE_BLANKING)
    {
        GPIO_IntDisable(1<<INT_NUM);
        interrupt = true;
        cluster->RequestProcess(0);
    }
}

ClusterPanasonicOccupancy::ClusterPanasonicOccupancy(uint32_t _timeout, PostEventCallback _postEvent)
  : ClusterWorker(_postEvent), state(STATE_IDLE), timeout(_timeout), blankingTime(3 * _timeout / 4)
{
    // Note, the input GPIO has been setup by app config to input with pull low
    GPIO_ExtIntConfig(SL_EMLIB_GPIO_INIT_MOTION_INPUT_PORT, SL_EMLIB_GPIO_INIT_MOTION_INPUT_PIN, INT_NUM, 1, 0, true); // rising edge only, interrupt enabled
    GPIOINT_CallbackRegister(INT_NUM, int_callback);
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
        //Entity::process_state();
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
        //Entity::process_state();
        break;
    case STATE_IDLE:
    default:
        break;
    }
}

} /* namespace cluster_lib */
