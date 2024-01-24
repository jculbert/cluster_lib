/*
 * ClusterWind.cpp
 *
 *  Created on: Jan. 17, 2024
 *      Author: jeff
 */

#include "AppConfig.h"

#ifdef CLUSTER_WIND

#include "em_chip.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_pcnt.h"
#include "em_prs.h"

#include "sl_emlib_gpio_init_PULSE_INPUT_config.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>

#include <app/util/attribute-storage.h>

#include "ClusterWind.h"

// The PCNT (Pulse Counter) related code was copied from
// https://github.com/SiliconLabs/peripheral_examples/tree/master/series2/pcnt/pcnt_extclk_single_overflow

// Wind speed is calculated from a pulse count for an overall period
// Gust is determined by a max pulse count change over each sample period
#define SAMPLE_PERIOD_SECS (4)
#define NUM_SAMPLES (60)
#define TOTAL_PERIOD_SECS (SAMPLE_PERIOD_SECS * NUM_SAMPLES)


namespace cluster_lib
{

// PRS channel to use for GPIO/PCNT
#define PCNT_PRS_Ch0    0

static ClusterWind *cluster;

/***************************************************************************//**
 * @brief PCNT0 interrupt handler
 *        This function acknowledges the interrupt request.
 *        It replaces a default one with weak linkage.
 *        This is part of a builtin PRS mechanism where interrupts are used
 *        with GPIO inputs for signaling.
 ******************************************************************************/
void PCNT0_IRQHandler(void)
{
  // Acknowledge interrupt
  PCNT_IntClear(PCNT0, PCNT_IF_OF);
}

/***************************************************************************//**
 * @brief Initialize PCNT0
 *        This function initializes pulse counter 0 and sets up the
 *        external single mode; PCNT0 overflow interrupt is also configured
 *        in this function
 ******************************************************************************/
static void initPcnt(void)
{
  PCNT_Init_TypeDef pcntInit = PCNT_INIT_DEFAULT;

  CMU_ClockEnable(cmuClock_PCNT0, true);

  pcntInit.mode     = pcntModeDisable;    // Initialize with PCNT disabled
  pcntInit.s1CntDir = false;        // S1 does not affect counter direction,
                                    // using default init setting; count up
  pcntInit.s0PRS    = PCNT_PRS_Ch0;

  pcntInit.top = 0xffff;

  // Enable PRS Channel 0
  PCNT_PRSInputEnable(PCNT0, pcntPRSInputS0, true);

  // Init PCNT0
  PCNT_Init(PCNT0, &pcntInit);

  // Set mode to externally clocked single input
  PCNT_Enable(PCNT0, pcntModeExtSingle);

  // Change to the external clock
  CMU->PCNT0CLKCTRL = CMU_PCNT0CLKCTRL_CLKSEL_PCNTS0;

  /*
   * When the PCNT operates in externally clocked mode and switching external
   * clock source, a few clock pulses are required on the external clock to
   * synchronize accesses to the externally clocked domain. This example uses
   * push button PB0 via GPIO/PRS/PCNT0_S0IN for the external clock, which would
   * require multiple button presses to sync the PCNT registers and clock
   * domain, during which button presses would not be counted.
   *
   * To get around this, such that each button push is recognized, firmware
   * can use the PRS software pulse triggering mechanism to generate
   * those first few pulses. This allows the first actual button press and
   * subsequent button presses to be properly counted.
   */
  while (PCNT0->SYNCBUSY) {
    PRS_PulseTrigger(1 << PCNT_PRS_Ch0);
  }

  // Enable Interrupt
  //PCNT_IntEnable(PCNT0, PCNT_IEN_OF);

  // Clear PCNT0 pending interrupt
  //NVIC_ClearPendingIRQ(PCNT0_IRQn);

  // Enable PCNT0 interrupt in the interrupt controller
  //NVIC_EnableIRQ(PCNT0_IRQn);
}

/***************************************************************************//**
 * @brief Initialize PRS
 *        This function sets up GPIO PRS which links BTN0 to PCNT0 PRS0
 ******************************************************************************/
static void initPrs(void)
{
  CMU_ClockEnable(cmuClock_PRS, true);

  // Set up GPIO PRS
  PRS_SourceAsyncSignalSet(PCNT_PRS_Ch0, PRS_ASYNC_CH_CTRL_SOURCESEL_GPIO,
                           SL_EMLIB_GPIO_INIT_PULSE_INPUT_PIN);
}

/***************************************************************************//**
 * @brief Initialize GPIO
 *        This function initializes push button PB0 and enables external
 *        interrupt for PRS functionality
 ******************************************************************************/
static void initGpio(void)
{
#if 0 // disable code handled by framework
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Initialize LED driver
  GPIO_PinModeSet(BSP_GPIO_LED0_PORT, BSP_GPIO_LED0_PIN, gpioModePushPull, 0);

  // Configure pin I/O - BTN0
  GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInput, 1);
#endif

  // Configure BTN0 for external interrupt
  // Needed for GPIO to work with PRS?
  GPIO_ExtIntConfig(SL_EMLIB_GPIO_INIT_PULSE_INPUT_PORT, SL_EMLIB_GPIO_INIT_PULSE_INPUT_PIN,
      SL_EMLIB_GPIO_INIT_PULSE_INPUT_PIN, false, false, false);
}

static void UpdateClusterState(intptr_t notused)
{
    SILABS_LOG("Wind wind=%d, gust=%d", cluster->wind, cluster->gust);
    //chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(cluster->endpoint, cluster->temperature);
    chip::app::Clusters::FlowMeasurement::Attributes::MeasuredValue::Set(cluster->endpoint, cluster->wind * 10);
    chip::app::Clusters::FlowMeasurement::Attributes::MeasuredValue::Set(cluster->endpoint+1, cluster->gust * 10);
}

ClusterWind::ClusterWind (uint32_t _endpoint, PostEventCallback _postEventCallback)
    : ClusterWorker(_endpoint, _postEventCallback), pulse_cnt_start(0), pulse_cnt(0), pulse_cnt_max_delta(0),
      num_samples(0), wind(0), gust(0)
{
    cluster = this;
    initGpio();
    initPrs();
    initPcnt();

    RequestProcess(SAMPLE_PERIOD_SECS * 1000);

    pulse_cnt_start = PCNT_CounterGet(PCNT0);
}

ClusterWind::~ClusterWind ()
{
}

void ClusterWind::ProcesWindData()
{
    // Based on pulse counts every 4 seconds for 240 seconds (60 samples)
    // Wind is calculated from finalSample and gust from maxSample
    const uint16_t hzToKph = 4; // (2.5 * (8.0 / 5.0)), // From wind sensor spec 1 Hz = 2.5 mph
    static uint16_t gustHistory[] = {0, 0, 0};
    const uint16_t gustHistoryLen = sizeof(gustHistory) / sizeof(gustHistory[0]);
    static uint16_t gustHistoryHead = 0;

    wind = (hzToKph * pulse_cnt + (TOTAL_PERIOD_SECS/2) ) / TOTAL_PERIOD_SECS;
    gustHistory[gustHistoryHead] = (hzToKph * pulse_cnt_max_delta + (SAMPLE_PERIOD_SECS/2) ) / SAMPLE_PERIOD_SECS;
    gustHistoryHead = (gustHistoryHead + 1) % gustHistoryLen;
    gust = 0;
    for (uint16_t i = 0; i < gustHistoryLen; i++)
    {
        if (gust < gustHistory[i])
            gust = gustHistory[i];
    }
    chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusterState, reinterpret_cast<intptr_t>(nullptr));
}

void ClusterWind::Process(const AppEvent * event)
{
    uint16_t last_cnt = pulse_cnt;

    // Note, the pulse counter runs without restart and will wrap at 0xffff
    // Here we get the count since start of cycle by removing an offset
    // Note, the counter wrapping does not affect this calculation
    pulse_cnt = PCNT_CounterGet(PCNT0) - pulse_cnt_start;

    uint16_t delta = pulse_cnt - last_cnt;
    SILABS_LOG("Wind Process cnt=%d, delta=%d", pulse_cnt, delta);
    if (delta > pulse_cnt_max_delta)
        pulse_cnt_max_delta = delta;

    if (++num_samples >= NUM_SAMPLES)
    {
        ProcesWindData();
        num_samples = 0;
        pulse_cnt_start = PCNT_CounterGet(PCNT0);
        pulse_cnt = 0;
        pulse_cnt_max_delta = 0;

        // Note, resetting the counter was found to cause a long spin wait
        // and in some cases it did not return. So we let the counter
        // run and wrap and take this into account as explained above.
        //PCNT_CounterReset(PCNT0); // This reloads the default top value
        //PCNT_CounterTopSet(PCNT0, 0, 0xffff);
        //SILABS_LOG("Wind reset done");
    }

    RequestProcess(SAMPLE_PERIOD_SECS * 1000);
}

} // cluster_lib

#endif //CLUSTER_WIND
