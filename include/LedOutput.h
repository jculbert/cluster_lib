/*
 * LedOutput.h
 *
 *  Created on: Nov. 17, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_LEDOUTPUT_H_
#define CLUSTER_LIB_LEDOUTPUT_H_

#include "LEDWidget.h"

#include <OutputHelper.h>

namespace cluster_lib
{

    class LedOutput : public OutputHelper
    {
    public:
        LedOutput (uint8_t led);
        virtual
        ~LedOutput ();

        void SetOutput(bool state);

    private:
        LEDWidget led_widget;
    };

} /* namespace cluster_lib */

#endif /* CLUSTER_LIB_LEDOUTPUT_H_ */
