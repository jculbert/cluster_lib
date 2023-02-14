/*
 * ClusterPanasonicOccupancy.h
 *
 *  Created on: Feb. 9, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_INCLUDE_CLUSTERPANASONICOCCUPANCY_H_
#define CLUSTER_LIB_INCLUDE_CLUSTERPANASONICOCCUPANCY_H_

#include <stdint.h>

#include <ClusterWorker.h>

namespace cluster_lib
{

class ClusterPanasonicOccupancy : public ClusterWorker
{
public:
    bool occupancy;
    enum {STATE_IDLE, STATE_BLANKING, STATE_DELAY} state;
    uint32_t timeout; // in seconds
    uint32_t blankingTime;

    ClusterPanasonicOccupancy(uint32_t _endpoint, uint32_t _timeout, PostEventCallback _postEventCallback);

    virtual
    ~ClusterPanasonicOccupancy()
    {}

    void Process();
};

} /* namespace cluster_lib */

#endif /* CLUSTER_LIB_INCLUDE_CLUSTERPANASONICOCCUPANCY_H_ */
