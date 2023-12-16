/*
 * LedOutput.h
 *
 *  Created on: Nov. 17, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_GPIOOUTPUT_H_
#define CLUSTER_LIB_GPIOOUTPUT_H_

#include "em_gpio.h"

#include <OutputHelper.h>

namespace cluster_lib
{

    class GpioOutput : public OutputHelper
    {
    public:
        GpioOutput (GPIO_Port_TypeDef port, unsigned int pin);
        virtual
        ~GpioOutput ();

        void SetOutput(bool state);

    private:
        GPIO_Port_TypeDef port;
        unsigned int pin;
    };

} /* namespace cluster_lib */

#endif /* CLUSTER_LIB_GPIOOUTPUT_H_ */
