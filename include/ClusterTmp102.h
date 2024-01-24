/*
 * ClusterTmp102.h
 *
 *  Created on: Jan. 23, 2024
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_CLUSTERTMP102_H_
#define CLUSTER_LIB_CLUSTERTMP102_H_

#include "ClusterWorker.h"

namespace cluster_lib
{

class ClusterTmp102 : public cluster_lib::ClusterWorker
{
public:
    uint32_t read_period_ms;
    enum {STATE_TRIGGER_READING, STATE_READ} state;
    int16_t temperature;

    ClusterTmp102(uint32_t _endpoint, PostEventCallback _postEventCallback, uint32_t read_period_sec);

    void Process(const AppEvent * event);
};

} // namespace cluster_lib

#endif /* CLUSTER_LIB_CLUSTERTMP102_H_ */
