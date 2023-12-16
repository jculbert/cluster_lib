/*
 * LedOutput.cpp
 *
 *  Created on: Nov. 17, 2023
 *      Author: jeff
 */

#include "LedOutput.h"

namespace cluster_lib
{
    LedOutput::LedOutput (uint8_t led)
    {
        led_widget.Init(led);
    }

    LedOutput::~LedOutput ()
    {
    }

    void LedOutput::SetOutput(bool state)
    {
        led_widget.Set(state);
    }
} /* namespace cluster_lib */
