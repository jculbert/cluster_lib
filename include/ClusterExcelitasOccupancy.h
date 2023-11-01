/*
 * ClusterExcelitasOccupancy.h
 *
 *  Created on: Jun. 2, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_CLUSTEREXCELITASOCCUPANCY_H_
#define CLUSTER_LIB_CLUSTEREXCELITASOCCUPANCY_H_

#include <ClusterWorker.h>

namespace cluster_lib
{
    class ClusterExcelitasOccupancy : public ClusterWorker
    {
    public:
        bool occupancy;
        enum {STATE_IDLE, STATE_BLANKING, STATE_DELAY} state;
        uint32_t timeout; // in seconds
        uint32_t blankingTime;

        ClusterExcelitasOccupancy (uint32_t _endpoint, uint32_t _timeout, PostEventCallback _postEventCallback);
        virtual
        ~ClusterExcelitasOccupancy ();

        void Process(const AppEvent * event);
    };

} /* namespace cluster_lib */

#endif /* CLUSTER_LIB_CLUSTEREXCELITASOCCUPANCY_H_ */
