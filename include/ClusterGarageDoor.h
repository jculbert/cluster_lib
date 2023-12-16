/*
 * ClusterGarageDoor.h
 *
 *  Created on: Jul. 18, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_CLUSTERGARAGEDOOR_H_
#define CLUSTER_LIB_CLUSTERGARAGEDOOR_H_

#include <app/ConcreteAttributePath.h>

#include <ClusterWorker.h>
#include <GpioOutput.h>

namespace cluster_lib
{

    class ClusterGarageDoor : public cluster_lib::ClusterWorker
    {
    public:
        enum {COMMAND_NONE, COMMAND_OPEN, COMMAND_CLOSE} command;
        bool door_closed;
        bool contact_int; // true means a contact rising or falling interrupt occurred
        bool target_changed;
        unsigned pulse_relay_duration;
        uint64_t contact_ms; // Time of last contact interrupt
        GpioOutput gpio_output;

       ClusterGarageDoor (uint32_t _endpoint, PostEventCallback _postEventCallback);
        virtual
        ~ClusterGarageDoor ();

        void Process(const AppEvent * event);

        static void NotifyAttributeChange(const chip::app::ConcreteAttributePath & attributePath);
    };
}

#endif /* CLUSTER_LIB_CLUSTERGARAGEDOOR_H_ */
