/*
 * ClusterBattery.cpp
 *
 *  Created on: Jan. 30, 2024
 *      Author: jeff
 */


// IADC code copied from https://github.com/SiliconLabs/peripheral_examples/blob/master/series2/iadc/iadc_single_interrupt/src/main_single_interrupt.c

#include "em_cmu.h"
#include "em_emu.h"
#include "em_iadc.h"

#include <AppTask.h>
//#include <SensorManager.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/clusters/occupancy-sensor-server/occupancy-hal.h>
#include <app/clusters/occupancy-sensor-server/occupancy-sensor-server.h>
#include <cmsis_os2.h>
#include <platform/silabs/platformAbstraction/SilabsPlatform.h>

#include "ClusterBattery.h"

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::DeviceLayer::Silabs;
using namespace chip::Protocols::InteractionModel;

static cluster_lib::ClusterBattery *cluster;

static uint32_t vbat_mv;

void IADC_IRQHandler(void)
{
  // Read a result from the FIFO
  IADC_Result_t sample = IADC_pullSingleFifoResult(IADC0);

  // 1210 for the bandgap reference, and 4 because we convert Vdd/4
  vbat_mv = (sample.data * 4 * 1210) / 0xFFF;
  ChipLogDetail(AppServer, "vbat %d", vbat_mv);

  /*
   * Clear the single conversion complete interrupt.  Reading FIFO
   * results does not do this automatically.
   */
  IADC_clearInt(IADC0, IADC_IF_SINGLEDONE);

  cluster->RequestProcess(0);
}

namespace cluster_lib
{

static void initIADC(void)
{
    IADC_Init_t init = IADC_INIT_DEFAULT;
    IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
    IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
    IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;

    CMU_ClockEnable(cmuClock_IADC0, true);

    // Use the FSRC0 as the IADC clock so it can run in EM2
    CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);

#if 0
    // Set the prescaler needed for the intended IADC clock frequency
    // Set CLK_ADC to 10 MHz
    const uint32_t CLK_SRC_ADC_FREQ = 20000000;
    init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);
#endif

    // Set the prescaler needed for the intended IADC clock frequency
    //const uint32_t CLK_ADC_FREQ = 10000000;  // CLK_ADC - 10 MHz max in normal mode
    const uint32_t CLK_ADC_FREQ = 10000000;  // CLK_ADC - 10 MHz max in normal mode
    init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_ADC_FREQ, 0);

    // Shutdown between conversions to reduce current
    init.warmup = iadcWarmupNormal;

    /*
     * Configuration 0 is used by both scan and single conversions by
     * default.  Use internal bandgap as the reference and specify the
     * reference voltage in mV.
     *
     * Resolution is not configurable directly but is based on the
     * selected oversampling ratio (osrHighSpeed), which defaults to
     * 2x and generates 12-bit results.
     */
    initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;
    initAllConfigs.configs[0].vRef = 1210;
    initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed2x;
    initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain1x;

    /*
     * CLK_SRC_ADC must be prescaled by some value greater than 1 to
     * derive the intended CLK_ADC frequency.
     *
     * Based on the default 2x oversampling rate (OSRHS)...
     *
     * conversion time = ((4 * OSRHS) + 2) / fCLK_ADC
     *
     * ...which results in a maximum sampling rate of 833 ksps with the
     * 2-clock input multiplexer switching time is included.
     */
    initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
        CLK_ADC_FREQ, 0, iadcCfgModeNormal, init.srcClkPrescale);

    /*
     * Specify the input channel.  When negInput = iadcNegInputGnd, the
     * conversion is single-ended.
     */
    singleInput.posInput   = iadcPosInputAvdd;
    singleInput.negInput   = iadcNegInputGnd;

    // Allocate the analog bus for ADC0 inputs
    //GPIO->IADC_INPUT_0_BUS |= IADC_INPUT_0_BUSALLOC; // Needed?

    // Initialize IADC
    IADC_init(IADC0, &init, &initAllConfigs);

    // Initialize a single-channel conversion
    IADC_initSingle(IADC0, &initSingle, &singleInput);

    // Clear any previous interrupt flags
    IADC_clearInt(IADC0, _IADC_IF_MASK);

    // Enable single-channel done interrupts
    IADC_enableInt(IADC0, IADC_IEN_SINGLEDONE);

    // Enable IADC interrupts
    NVIC_ClearPendingIRQ(IADC_IRQn);
    NVIC_EnableIRQ(IADC_IRQn);
}

// Returns battery percent in zigbee units (int percent * 2)
// battery percent is in 1/2 percent units, so 200 is 100 percent
static uint8_t get_zigbee_battery_percent(uint32_t _vbat_mv)
{
    typedef struct { float v; int p; } vp_t;

#ifdef BATTERY_TYPE_AAA_NIMH
    // divided by 2000 because vbat_mv is for two batteries
    float vbat = ((float)_vbat_mv) / 2000.0f;

    // Curve for AAA_NiMH
    static const vp_t curve[] = {
        {1.45f, 100},
        {1.35f,  90},
        {1.30f,  80},
        {1.25f,  65},
        {1.20f,  50},
        {1.15f,  30},
        {1.10f,  12},
        {1.05f,   3},
        {1.00f,   0}
    };

#elif defined(BATTERY_TYPE_AAA_ALKALINE)
    // divided by 2000 because vbat_mv is for two batteries
    float vbat = ((float)_vbat_mv) / 2000.0f;

    static const vp_t curve[] = {
        {1.60,100},
        {1.50,95},
        {1.45,90},
        {1.40,80},
        {1.35,70},
        {1.30,60},
        {1.25,50},
        {1.20,40},
        {1.15,30},
        {1.10,20},
        {1.05,10},
        {1.00,5},
        {0.90,0}
    };

#elif defined(BATTERY_TYPE_LITHIUM)
    float vbat = ((float)_vbat_mv) / 1000.0f;

    // Curve for Lithium
    static const vp_t curve[] = {
        {3.00f, 100},
        {2.95f,  90},
        {2.90f,  80},
        {2.85f,  60},
        {2.80f,  40},
        {2.75f,  20},
        {2.70f,  10},
        {2.50f,   0},
    };
#endif

    uint32_t curve_len = sizeof(curve) / sizeof(curve[0]);

    if (vbat >= curve[0].v) return 200;
    if (vbat <= curve[curve_len-1].v) return 0;
    for (uint32_t i = 0; i < (curve_len-1); ++i) {
        float v1 = curve[i].v, v2 = curve[i+1].v;
        if (vbat <= v1 && vbat >= v2) {
            // interpolate between the two points
            float p1 = (float)curve[i].p, p2 = (float)curve[i+1].p;
            float t = (v1 - vbat) / (v1 - v2);
            return (uint8_t) (2.0f * (p1 - t * (p1 - p2) + 0.5f));
        }
    }
    return 0; // should be impossible
}

ClusterBattery::ClusterBattery (uint32_t _endpoint, PostEventCallback _postEventCallback, uint32_t _nominal_mv, uint32_t _refresh_minutes)
  : ClusterWorker(_endpoint, _postEventCallback), nominal_mv(_nominal_mv), refresh_ms(_refresh_minutes * 60000),
    waiting_adc(false), battery_percent(0)

{
    cluster = this;
    RequestProcess(10000);
}

void ClusterBattery::UpdateClusterState()
{
    DeviceLayer::PlatformMgr().ScheduleWork([](intptr_t arg)-> void {
        PowerSource::Attributes::BatPercentRemaining::Set(cluster->endpoint, cluster->battery_percent);
    });
}

void ClusterBattery::Process(const AppEvent * event)
{
    ChipLogDetail(AppServer, "Batt Process");

    if (waiting_adc)
    {
        waiting_adc = false;

        // reset ADC and disable clock between measurements to reduce power
        IADC_reset(IADC0);
        CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_Disabled);
        CMU_ClockEnable(cmuClock_IADC0, false);

        battery_percent = get_zigbee_battery_percent(vbat_mv);
        ChipLogProgress(AppServer, "Batt percent %d", battery_percent);

        UpdateClusterState();
        RequestProcess(refresh_ms);
    }
    else
    {
        waiting_adc = true;
        initIADC();
        IADC_command(IADC0, iadcCmdStartSingle);
    }
}

} /* namespace cluster_lib */

