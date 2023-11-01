/*
 * ClusterContact.h
 *
 *  Created on: May 25, 2023
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_CLUSTERCONTACT_H_
#define CLUSTER_LIB_CLUSTERCONTACT_H_

#include "AppConfig.h"

#ifdef CLUSTER_CONTACT

#include <stdint.h>

#include <ClusterWorker.h>

namespace cluster_lib
{

class ClusterContact : public ClusterWorker
{
public:
    static const int8_t EventSubTypeReadHall = EventSubTypeBase + 1;
    bool state;

    ClusterContact (uint32_t _endpoint, PostEventCallback _postEventCallback);
    virtual
    ~ClusterContact ();

    void Process(const AppEvent * event);

    void RequestHallRead();
};

} /* namespace cluster_lib */

#endif // CLUSTER_CONTACT

#endif /* CLUSTER_LIB_CLUSTERCONTACT_H_ */
