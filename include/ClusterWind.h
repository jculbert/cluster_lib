/*
 * ClusterWind.h
 *
 *  Created on: Jan. 17, 2024
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_CLUSTERWIND_H_
#define CLUSTER_LIB_CLUSTERWIND_H_

#include "AppConfig.h"

#include "ClusterWorker.h"

namespace cluster_lib
{
    class ClusterWind : public cluster_lib::ClusterWorker
    {
    private:
        uint16_t pulse_cnt_start;
        uint16_t pulse_cnt;
        uint16_t pulse_cnt_max_delta;
        uint16_t num_samples;
    public:
        uint16_t wind;
        uint16_t gust;

    public:
      ClusterWind (uint32_t _endpoint, PostEventCallback _postEventCallback);

      virtual
      ~ClusterWind ();

      void Process(const AppEvent * event);

    private:
      void ProcesWindData();
    };
}

#endif /* CLUSTER_LIB_CLUSTERWIND_H_ */
