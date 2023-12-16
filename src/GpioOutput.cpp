/*
 * LedOutput.cpp
 *
 *  Created on: Nov. 17, 2023
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef GPIOOUTPUT

#include "GpioOutput.h"

namespace cluster_lib
{
    GpioOutput::GpioOutput (GPIO_Port_TypeDef _port, unsigned int _pin)
            : port(_port), pin(_pin)
    {
    }

    GpioOutput::~GpioOutput ()
    {
    }

    void GpioOutput::SetOutput(bool state)
    {
        if (state)
            GPIO_PinOutSet(port, pin);
        else
            GPIO_PinOutClear(port, pin);
    }
} /* namespace cluster_lib */

#endif // GPIOOUTPUT
