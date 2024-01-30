/*
 * watchdog.h
 *
 *  Created on: Jan. 24, 2024
 *      Author: jeff
 */

#ifndef CLUSTER_LIB_INCLUDE_WATCHDOGTASK_H_
#define CLUSTER_LIB_INCLUDE_WATCHDOGTASK_H_

#include <cstddef>

namespace cluster_lib
{

void WatchdogInit(uint32_t reset_reason);

}

#endif /* CLUSTER_LIB_INCLUDE_WATCHDOGTASK_H_ */
