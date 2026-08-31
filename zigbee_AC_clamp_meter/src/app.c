/***************************************************************************//**
 * @file app.c
 * @brief Callbacks implementation and application specific code.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include "app/framework/include/af.h"
#include "app.h"
#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"
#include "sl_led.h"
#include "mikroe_accurrent.h"
#include "sl_spidrv_instances.h"
#include "network-steering.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
#define LED_BLINK_PERIOD_MS                             1000 // This period of the LED blinking event
#define AC_PARAMETERS_MEASUREMENT_EXECUTION_PERIOD_S    1   // AC current measurement periodic timer value in second

#define CURRENT_AND_POWER_REPORT_COUNTER                10        // Counter limit for current and power values reporting
#define TOTAL_POWER_REPORT_COUNTER                      30        // Counter limit for total power value reporting
#define AC_VOLTAGE_CONST                                230 // AC voltage (nominal)
#define CONVERTER_FROM_mW_TO_W                          1000 // Constant for converting from mW to W
#define TIME_CONVERTER_FROM_S_TO_MSEC                   1000

#define CURRENT_CLICK_MEASUREMENT_ENDPOINT              1 // Endpoint with the Smart Metering and the Electrical ZCL
#define STATUS_LED                                      (&sl_led_led0)

/*
 * This DEBUG_PRINT flag is responsible for printing every measurement step.
 * If this is true, then the measurement step will be printed on the console.
 * Default value is the true.
 * */
#define DEBUG_PRINT                                     true

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
static bool commissioning = false; // Holds the commissioning status

typedef struct {
  uint16_t rms;       // Instantaneous current value
  uint32_t sum;       // Summary of current values
  uint16_t avg;       // Current average
}ac_current_t;

typedef struct {
  uint32_t active;    // Instantaneous active power value
  uint32_t sum;       // Summary of power values
  uint16_t avg;       // Power average
  uint64_t total;     // Total power
}ac_power_t;

typedef struct {
  ac_current_t current;         // AC current
  ac_power_t power;             // AC power
  uint16_t measurement_cnt;     // Counter of measuring
} ac_measurement_parameters_t;

// -----------------------------------------------------------------------------
//                                Const
// -----------------------------------------------------------------------------

// Period time value of AC parameters measuring
static const uint32_t AC_PARAMETER_MEASUREMENT_PERIOD =
  AC_PARAMETERS_MEASUREMENT_EXECUTION_PERIOD_S
  * TIME_CONVERTER_FROM_S_TO_MSEC;
// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------
static void network_steering_event_handler(sl_zigbee_af_event_t *event);
static void leave_network_event_handler(sl_zigbee_af_event_t *event);
static void led_event_handler(sl_zigbee_af_event_t *event);
static void ac_parameter_measurements_execution_event_handler(
  sl_zigbee_af_event_t *event);
static void report_sending(ac_measurement_parameters_t *ac);

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------
static sl_zigbee_af_event_t network_steering_event_control;
static sl_zigbee_af_event_t leave_network_event_control;
static sl_zigbee_af_event_t led_event_control;
static sl_zigbee_af_event_t ac_parameter_measurements_execution_event_control;

// User provided code. Application process.
void app_process_action(void)
{
}

// -----------------------------------------------------------------------------
//                          Callback Handler
// -----------------------------------------------------------------------------

/** @brief Main Init
 *
 * This function is called from the application's main function. It gives the
 * application a chance to do any initialization required at system startup. Any
 * code that you would normally put into the top of the application's main()
 * routine should be put into this function. This is called before the clusters,
 * plugins, and the network are initialized so some functionality is not yet
 * available.
 *         Note: No callback in the Application Framework is
 * associated with resource cleanup. If you are implementing your application on
 * a Unix host where resource cleanup is a consideration, we expect that you
 * will use the standard Posix system calls, including the use of atexit() and
 * handlers for signals such as SIGTERM, SIGINT, SIGCHLD, SIGPIPE and so on. If
 * you use the signal() function to register your signal handler, please mind
 * the returned value which may be an Application Framework function. If the
 * return value is non-null, please make sure that you call the returned
 * function from your handler to avoid negating the resource cleanup of the
 * Application Framework itself.
 *
 */
void sl_zigbee_af_main_init_cb(void)
{
  sl_status_t sc;
  sl_zigbee_app_debug_print("AC Current Click Measurement Zigbee example\n");
  // Enable the AC Measurement sensor
  sc = mikroe_accurrent_init(sl_spidrv_mikroe_handle);
  if (sc != SL_STATUS_OK) {
    sl_zigbee_app_debug_print("Initialization error. Please check again ...\r\n");
    return;
  }
  sl_zigbee_app_debug_print(
    "AC click sensor initialization was successfully.\r\n");

  sl_zigbee_af_isr_event_init(&network_steering_event_control,
                              network_steering_event_handler);
  sl_zigbee_af_isr_event_init(&leave_network_event_control,
                              leave_network_event_handler);
  sl_zigbee_af_event_init(&led_event_control,
                          led_event_handler);
  sl_zigbee_af_event_init(&ac_parameter_measurements_execution_event_control,
                          ac_parameter_measurements_execution_event_handler);
}

// -----------------------------------------------------------------------------
//                          Event Handlers
// -----------------------------------------------------------------------------

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action. The framework
 * will always process the stack status after the callback returns.
 */
void sl_zigbee_af_stack_status_cb(sl_status_t status)
{
  if (status == SL_STATUS_NETWORK_DOWN) {
    sl_led_turn_off(STATUS_LED);
  } else if (status == SL_STATUS_NETWORK_UP) {
    sl_led_turn_on(STATUS_LED);
  }
}

/** @brief Leave Network Event Handler
 *
 * This event handler is called in response to it's respective control
 * activation. It handles the network leaving process.
 *
 */
static void leave_network_event_handler(sl_zigbee_af_event_t *event)
{
  sl_status_t status;

// Leave network
  status = sl_zigbee_leave_network(SL_ZIGBEE_LEAVE_NWK_WITH_NO_OPTION);
  sl_zigbee_app_debug_print("%s 0x%02X\r\n", "Network leave", status);
// Clear Binding Table
  sl_zigbee_clear_binding_table();
  sl_zigbee_app_debug_print("Binding Table is cleared! \r\n");
  commissioning = false;
  sl_zigbee_af_event_set_inactive(
    &ac_parameter_measurements_execution_event_control);
}

/** @brief Network Steering Event Handler
 *
 * This event handler is called in response to it's respective control
 * activation. It handles the network steering process.
 *
 */
static void network_steering_event_handler(sl_zigbee_af_event_t *event)
{
  sl_status_t status;

  // If not in a network, attempt to join one
  if (sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    sl_zigbee_app_debug_print(
      "Leave the network first (the device is connected to a network)!");
    return;
  }
  status = sl_zigbee_af_network_steering_start();
  if (status != SL_STATUS_OK) {
    sl_zigbee_app_debug_print("Initiate network failed: 0x%02X\r\n",
                              status);
    commissioning = false;
  } else {
    sl_zigbee_app_debug_print("Initiate network is successful: 0x%02X\r\n",
                              status);
    sl_zigbee_app_debug_print("%s network %s: 0x%02X\r\n",
                              "Join",
                              "start",
                              status);
    commissioning = true;
    sl_zigbee_af_event_set_active(&led_event_control);
  }
}

/** @brief Complete network steering.
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to SL_STATUS_OK to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt. Ver.: always
 *
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID. Ver.: always
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network. Ver.: always
 *
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete. Ver.: always
 */
void sl_zigbee_af_network_steering_complete_cb(sl_status_t status,
                                               uint8_t totalBeacons,
                                               uint8_t joinAttempts,
                                               uint8_t finalState)
{
  (void)totalBeacons;
  (void)joinAttempts;
  (void)finalState;

  commissioning = false;
  if (status != SL_STATUS_OK) {
    sl_zigbee_app_debug_print("Network is not found: 0x%02X\r\n",
                              status);
  } else {
    sl_zigbee_app_debug_println("%s network %s: 0x%02X\r\n",
                                "Join",
                                "completed",
                                status);
    sl_zigbee_af_event_set_active(
      &ac_parameter_measurements_execution_event_control);
  }
}

/** @brief AC Parameter Measurements Execution Event Handler
 *
 * This event handler is responsible for AC parameter measurements execution.
 * It will measured the AC current and this will calculated the power and
 * collect the all power values based on the AC current. The all power is calculated in W
 * and the consumption calculation will handle by zigbee coordinator.
 *
 */
static void ac_parameter_measurements_execution_event_handler(
  sl_zigbee_af_event_t *event)
{
  static ac_measurement_parameters_t ac;
  ac.measurement_cnt++;
  ac.current.rms = (uint16_t)mikroe_accurrent_get_ma();
  ac.current.sum += ac.current.rms;
#if DEBUG_PRINT
  sl_zigbee_af_app_println("%d. Step: Sample current value: %d mA\r\n",
                           ac.measurement_cnt,
                           ac.current.rms);
#endif

  ac.power.active = (uint32_t)((ac.current.rms * AC_VOLTAGE_CONST)
                               / CONVERTER_FROM_mW_TO_W);
  ac.power.sum += ac.power.active;
#if DEBUG_PRINT
  sl_zigbee_af_app_println("%d. Step: Sample power value: %d W\r\n",
                           ac.measurement_cnt,
                           ac.power.active);
#endif

  if (ac.measurement_cnt % CURRENT_AND_POWER_REPORT_COUNTER == 0) {
    ac.current.avg = (uint16_t)ac.current.sum
                     / CURRENT_AND_POWER_REPORT_COUNTER;
    sl_zigbee_af_app_println("Current value: %d mA\r\n", ac.current.avg);
    ac.power.avg = (uint16_t)(ac.power.sum  / CURRENT_AND_POWER_REPORT_COUNTER);
    sl_zigbee_af_app_println("Power value: %d W\r\n", ac.power.avg);
    ac.power.total += ac.power.sum
                      * AC_PARAMETERS_MEASUREMENT_EXECUTION_PERIOD_S;
    if (ac.measurement_cnt == TOTAL_POWER_REPORT_COUNTER) {
      sl_zigbee_af_app_println("Total power value: %llu W\r\n", ac.power.total);
    }
    ac.current.sum = 0;
    ac.power.sum = 0;
    report_sending(&ac);
  } else {
    sl_zigbee_af_event_set_delay_ms(
      &ac_parameter_measurements_execution_event_control,
      AC_PARAMETER_MEASUREMENT_PERIOD);
  }
}

/** @brief Report Sending
 *
 * This function is called in response to its respective control
 * activation. It will report the measured values to the Electrical
 * Measurement and Simple Metering server clusters.
 *
 */
static void report_sending(ac_measurement_parameters_t *ac)
{
  sl_status_t status;

  sl_zigbee_af_event_set_delay_ms(
    &ac_parameter_measurements_execution_event_control,
    AC_PARAMETER_MEASUREMENT_PERIOD);

  status = sl_zigbee_af_write_server_attribute(
    CURRENT_CLICK_MEASUREMENT_ENDPOINT,
    ZCL_ELECTRICAL_MEASUREMENT_CLUSTER_ID,
    ZCL_RMS_CURRENT_ATTRIBUTE_ID,
    (uint8_t *)&ac->current.avg,
    ZCL_INT16U_ATTRIBUTE_TYPE);
  sl_zigbee_app_debug_println("%s reported: 0x%X\n",
                              "AC Current - RMSValue",
                              status);

  status = sl_zigbee_af_write_server_attribute(
    CURRENT_CLICK_MEASUREMENT_ENDPOINT,
    ZCL_ELECTRICAL_MEASUREMENT_CLUSTER_ID,
    ZCL_ACTIVE_POWER_ATTRIBUTE_ID,
    (uint8_t *)&ac->power.avg,
    ZCL_INT16S_ATTRIBUTE_TYPE);
  sl_zigbee_app_debug_println("%s reported: 0x%X\n",
                              "Active power - Value",
                              status);

  if (ac->measurement_cnt == TOTAL_POWER_REPORT_COUNTER) {
    ac->measurement_cnt = 0;
    status = sl_zigbee_af_write_server_attribute(
      CURRENT_CLICK_MEASUREMENT_ENDPOINT,
      ZCL_SIMPLE_METERING_CLUSTER_ID,
      ZCL_CURRENT_SUMMATION_DELIVERED_ATTRIBUTE_ID,
      (uint8_t *)&ac->power.total,
      ZCL_INT48U_ATTRIBUTE_TYPE);
    sl_zigbee_app_debug_println("%s reported: 0x%X\n",
                                "Total power - Value",
                                status);
  }
}

// -----------------------------------------------------------------------------
// LED event handler
// -----------------------------------------------------------------------------

static void led_event_handler(sl_zigbee_af_event_t *event)
{
  sl_zigbee_network_status_t status = sl_zigbee_af_network_state();
  if (commissioning) {
    if (status != SL_ZIGBEE_JOINED_NETWORK) {
      sl_led_toggle(STATUS_LED);
      sl_zigbee_af_event_set_delay_ms(&led_event_control,
                                      LED_BLINK_PERIOD_MS << 1);
    } else {
      sl_led_turn_on(STATUS_LED);
    }
  } else if (status == SL_ZIGBEE_JOINED_NETWORK) {
    sl_led_turn_on(STATUS_LED);
  } else if (status == SL_ZIGBEE_NO_NETWORK) {
    sl_led_turn_off(STATUS_LED);
  }
}

// -----------------------------------------------------------------------------
// Push button event handler
// -----------------------------------------------------------------------------

void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
    if (&sl_button_btn0 == handle) {
      sl_zigbee_af_event_set_active(&network_steering_event_control);
    } else if (&sl_button_btn1 == handle) {
      sl_zigbee_af_event_set_active(&leave_network_event_control);
    }
  }
}

/** @brief
 *
 * Application framework equivalent of ::sl_zigbee_radio_needs_calibrating_handler
 */
void sl_zigbee_af_radio_needs_calibrating_cb(void)
{
  sl_mac_calibrate_current_channel();
}
