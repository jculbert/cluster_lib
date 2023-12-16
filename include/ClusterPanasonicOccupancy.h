/*
 * ClusterPanasonicOccupancy.h
 *
 *  Created on: Feb. 9, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_INCLUDE_CLUSTERPANASONICOCCUPANCY_H_
#define CLUSTER_LIB_INCLUDE_CLUSTERPANASONICOCCUPANCY_H_

#include <AppConfig.h>

#ifdef CLUSTER_PANASONIC_OCCUPANCY

#include <stdint.h>

#include <ClusterWorker.h>
#include <LedOutput.h>

namespace cluster_lib
{

class ClusterPanasonicOccupancy : public ClusterWorker
{
public:
    bool occupancy;
    enum {STATE_IDLE, STATE_BLANKING, STATE_DELAY} state;
    uint32_t timeout_ms;
    uint32_t max_timeout_ms;
    uint32_t blanking_time_ms;
    LedOutput *led_output;

    // Note, timeout_ms and max_timeout are in minutes
    ClusterPanasonicOccupancy(uint32_t endpoint, uint32_t timeout, uint32_t max_timeout, uint8_t led, PostEventCallback _postEventCallback);

    virtual
    ~ClusterPanasonicOccupancy()
    {}

    void SetBlankingTime();

    static void ButtonHandler(AppEvent * aEvent);

    void _ButtonHandler(AppEvent * aEvent);

    void Process(const AppEvent * event);
};

} /* namespace cluster_lib */

#endif // CLUSTER_PANASONIC_OCCUPANCY

#endif /* CLUSTER_LIB_INCLUDE_CLUSTERPANASONICOCCUPANCY_H_ */
