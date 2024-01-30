/*
 * Watchdog.cpp
 *
 *  Created on: Jan. 24, 2024
 *      Author: jeff
 */

#include <cstddef>

#include "em_cmu.h"
#include "em_emu.h"
#include "em_wdog.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "ConfigurationManager.h"

#include "silabs_utils.h"

#include <WatchdogTask.h>

#define TASK_STACK_SIZE (1024*2)
#define TASK_PRIORITY (tskIDLE_PRIORITY + 1)

namespace cluster_lib
{

static uint32_t reset_reason = 0;
static uint32_t watchdog_resets = 0;

static void InitWdog(void)
{
  // Enable clock for the WDOG module; has no effect on xG21
  CMU_ClockEnable(cmuClock_WDOG0, true);

  // Watchdog Initialize settings
  WDOG_Init_TypeDef wdogInit = WDOG_INIT_DEFAULT;
  CMU_ClockSelectSet(cmuClock_WDOG0, cmuSelect_ULFRCO); /* ULFRCO as clock source */
  wdogInit.debugRun = true;
  wdogInit.em2Run = true;
  wdogInit.perSel = wdogPeriod_64k; // 64k clock cycles of a 1kHz clock  ~64 seconds period

  // Initializing watchdog with chosen settings
  WDOGn_Init(DEFAULT_WDOG, &wdogInit);
}

static void task_func(void * unused)
{
    // Keep track of the number of watchdog reset in NVM
    static const char* kvs_key = "WatchdogResets";
    bool update_needed = false;
    CHIP_ERROR err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(kvs_key, &watchdog_resets, sizeof(watchdog_resets));
    if (err == CHIP_NO_ERROR)
    {
        SILABS_LOG("Restoring watchdog_resets value from kvs %d", watchdog_resets);

        if (reset_reason & EMU_RSTCAUSE_WDOG0)
        {
            // Update the count
            watchdog_resets++;
            update_needed = true;
        }
    }
    else
    {
        // key does not exist, must be first run
        update_needed = true;
        watchdog_resets = 0;
    }
    err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(kvs_key, &watchdog_resets, sizeof(watchdog_resets));
    if (err != CHIP_NO_ERROR)
        SILABS_LOG("Error writing watchdog_resets %d", err);

    while(1)
    {
        SILABS_LOG("Wdog reset_reason %d, watchdog_resets %d", reset_reason, watchdog_resets);
        WDOGn_Feed(DEFAULT_WDOG);
        vTaskDelay(pdMS_TO_TICKS(45000));
    }
}

void WatchdogInit(uint32_t _reset_reason)
{
    reset_reason = _reset_reason;

    InitWdog();
    static StaticTask_t watchdog_task;
    static StackType_t stack[TASK_STACK_SIZE];

    xTaskCreateStatic(task_func, "watchdog_task", TASK_STACK_SIZE, NULL, TASK_PRIORITY, stack, &watchdog_task);
}

}
