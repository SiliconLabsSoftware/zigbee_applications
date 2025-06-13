/***************************************************************************//**
 * @file battery_monitor.c
 * @brief An example about sampling the battery voltage through IADC(Series 2).
 * @version v0.01
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided \'as-is\', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/
#include "app/framework/include/af.h"
#include "app_log.h"
#include "battery_monitor.h"
#include "gpiointerrupt.h"
#include "em_cmu.h"
#include "em_iadc.h"
#include "em_prs.h"

#define GPIO_PORT_A                       gpioPortA
#define GPIO_PORT_B                       gpioPortB
#define GPIO_PORT_C                       gpioPortC
#define GPIO_PORT_D                       gpioPortD

// Shorter macros for plugin options
#define FIFO_SIZE                         16
#define MS_BETWEEN_BATTERY_CHECK          ((30 * 60 * 10) / 3)
#define BSP_BATTERYMON_TX_ACTIVE_CHANNEL  6
#define BSP_BATTERYMON_TX_ACTIVE_PORT     gpioPortC
#define BSP_BATTERYMON_TX_ACTIVE_PIN      0
#define BSP_BATTERYMON_TX_ACTIVE_LOC      0

#define MAX_INT_MINUS_DELTA               0xe0000000

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
#define PRS_SOURCE                        PRS_ASYNC_CH_CTRL_SOURCESEL_RAC
#define PRS_SIGNAL                        PRS_ASYNC_CH_CTRL_SIGSEL_RACTX
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_2)
#define PRS_SOURCE                        PRS_ASYNC_CH_CTRL_SOURCESEL_RACL
#define PRS_SIGNAL                        PRS_ASYNC_CH_CTRL_SIGSEL_RACLTX
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_4)
#define PRS_SOURCE                        PRS_ASYNC_CH_CTRL_SOURCESEL_RACL
#define PRS_SIGNAL                        PRS_ASYNC_CH_CTRL_SIGSEL_RACLTX
#else
error("please define the correct macros here!")
#endif

// Set CLK_ADC to 100kHz (this corresponds to a sample rate of 10ksps)
#define CLK_SRC_ADC_FREQ                  1000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ                      100000 // CLK_ADC

/** Default config for IADC single input structure. */
#define IADC_SINGLEINPUT_BATTERY                                 \
  {                                                              \
    iadcNegInputGnd,       /* Negative input GND */              \
    iadcPosInputAvdd,      /* Positive input iadcPosInputAvdd */ \
    0,                     /* Config 0 */                        \
    false                  /* Do not compare results */          \
  }

#define IADC_REFERENCE_VOLTAGE_MILLIVOLTS 1210

// ------------------------------------------------------------------------------
// Forward Declaration
static uint16_t filter_voltage_sample(uint16_t sample);
static void battery_monitor_v2_read_adc_event_handler(uint8_t intNO);

// ------------------------------------------------------------------------------
// Globals

//// structure used to store irq configuration from GIC plugin
// static HalGenericInterruptControlIrqCfg *irqConfig;

// count used to track when the last measurement occurred
// Ticks start at 0.  We use this value to limit how frequently we make
// measurements in an effort to conserve battery power.  By setting this to an
// arbitrary value close to MAX_INT, we are going to make sure we make a
// battery measurement on the first transmission.
static uint32_t last_battery_measure_tick = MAX_INT_MINUS_DELTA;

// sample FIFO access variables
static uint8_t sample_ptr = 0;
static uint16_t voltage_fifo[FIFO_SIZE];
static bool fifo_initialized = false;

// Remember the last reported voltage value from callback, which will be the
// return value if anyone needs to manually poll for data
static uint16_t last_reported_voltage_milliV;

// ------------------------------------------------------------------------------
// Implementation of public functions

void sl_zigbee_af_plugin_battery_monitor_v2_init_cb(void)
{
  hal_battery_monitor_initialize();
}

void hal_battery_monitor_initialize(void)
{
  IADC_Init_t        init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t  init_all_configs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t  init_single = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t init_single_input = IADC_SINGLEINPUT_BATTERY;

  CMU_ClockEnable(cmuClock_IADC0, true);

  IADC_reset(IADC0);

  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO); // 20MHz

  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  init_all_configs.configs[0].reference = iadcCfgReferenceInt1V2;

  // Divides CLK_SRC_ADC to set the CLK_ADC frequency
  init_all_configs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                       CLK_ADC_FREQ,
                                                                       0,
                                                                       iadcCfgModeNormal,
                                                                       init.srcClkPrescale);
  IADC_init(IADC0, &init, &init_all_configs);

  init_single.dataValidLevel = IADC_SCANFIFOCFG_DVL_VALID4;
  IADC_initSingle(IADC0, &init_single, &init_single_input);

  CMU_ClockEnable(cmuClock_PRS, true);

  // Initialize the PRS system to drive a GPIO high when the preamble is in the
  // air, effectively becoming a TX_ACT pin
  PRS_SourceAsyncSignalSet(BSP_BATTERYMON_TX_ACTIVE_CHANNEL,
                           PRS_SOURCE,
                           PRS_SIGNAL);
  PRS_PinOutput(BSP_BATTERYMON_TX_ACTIVE_CHANNEL,
                prsTypeAsync,
                BSP_BATTERYMON_TX_ACTIVE_PORT,
                BSP_BATTERYMON_TX_ACTIVE_PIN);
  GPIO_PinModeSet(BSP_BATTERYMON_TX_ACTIVE_PORT,
                  BSP_BATTERYMON_TX_ACTIVE_PIN,
                  gpioModePushPull,
                  0);

  GPIO_ExtIntConfig(BSP_BATTERYMON_TX_ACTIVE_PORT,
                    BSP_BATTERYMON_TX_ACTIVE_PIN,
                    BSP_BATTERYMON_TX_ACTIVE_PIN,
                    1,
                    0,
                    1);
  GPIOINT_CallbackRegister(BSP_BATTERYMON_TX_ACTIVE_PIN,
                           battery_monitor_v2_read_adc_event_handler);
  GPIO_IntEnable(BSP_BATTERYMON_TX_ACTIVE_PIN);
}

uint16_t hal_get_battery_voltage_milliV(void)
{
  return last_reported_voltage_milliV;
}

static uint32_t hal_battery_monitor_read_voltage()
{
  uint32_t milliV = 0;
  uint32_t vMax = 0xFFF; // 12 bit ADC maximum
  uint32_t reference_milliV = IADC_REFERENCE_VOLTAGE_MILLIVOLTS;
  float milliV_per_bit = (float)reference_milliV / (float)vMax;

  IADC_InitSingle_t  init_single = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t init_single_input = IADC_SINGLEINPUT_BATTERY;

  init_single.dataValidLevel = IADC_SCANFIFOCFG_DVL_VALID4;
  IADC_initSingle(IADC0, &init_single, &init_single_input);

  // Start IADC conversion
  IADC_command(IADC0, iadcCmdStartSingle);

  // Wait for conversion to be complete
  while ((IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK
                           | _IADC_STATUS_SINGLEFIFODV_MASK))
         != IADC_STATUS_SINGLEFIFODV) {
    // while combined status bits 8 & 6 don't equal 1 and 0 respectively
  }
  // Get IADC result
  IADC_Result_t sample = IADC_readSingleResult(IADC0);

  // refer to RM 23.3.5.2 to understand the factor 4.
  milliV = (uint32_t)(milliV_per_bit * sample.data) * 4;
  sl_zigbee_af_app_println("IADC sample: %d, milliV=%lu", sample.data, milliV);

  return milliV;
}

SL_WEAK void battery_monitor_data_ready_cb(uint16_t battery_voltage_milliV)
{
}

// This event will sample the ADC during a radio transmission and notify any
// interested parties of a new valid battery voltage level via the
// battery_monitor_data_ready_cb
void battery_monitor_v2_read_adc_event_handler(uint8_t intNo)
{
  (void)intNo;

  uint16_t voltage_milliV = 0;
  uint32_t current_ms_tick = halCommonGetInt32uMillisecondTick();
  uint32_t time_since_last_measure_ms = current_ms_tick
                                        - last_battery_measure_tick;

  if (time_since_last_measure_ms >= MS_BETWEEN_BATTERY_CHECK) {
    voltage_milliV = hal_battery_monitor_read_voltage();
    // filter the voltage to prevent spikes from overly influencing data
    voltage_milliV = filter_voltage_sample(voltage_milliV);
    battery_monitor_data_ready_cb(voltage_milliV);
    last_reported_voltage_milliV = voltage_milliV;
    last_battery_measure_tick = current_ms_tick;
  }
}

// Provide smoothing of the voltage readings by reporting an average over the
// last few values
static uint16_t filter_voltage_sample(uint16_t sample)
{
  uint32_t voltage_sum;
  uint8_t i;

  if (fifo_initialized) {
    voltage_fifo[sample_ptr++] = sample;

    if (sample_ptr >= FIFO_SIZE) {
      sample_ptr = 0;
    }
    voltage_sum = 0;
    for (i = 0; i < FIFO_SIZE; i++) {
      voltage_sum += voltage_fifo[i];
    }
    sample = voltage_sum / FIFO_SIZE;
  } else {
    for (i = 0; i < FIFO_SIZE; i++) {
      voltage_fifo[i] = sample;
    }
    fifo_initialized = true;
  }

  return sample;
}

void hal_sleep_cb(boolean enter, SleepModes sleepMode)
{
  if (sleepMode < 2) {
    return;
  }

  if (enter) {
    IADC0->EN_CLR = IADC_EN_EN;
  } else {
    IADC0->EN_SET = IADC_EN_EN;
  }
}
