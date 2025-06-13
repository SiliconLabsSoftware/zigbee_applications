/***************************************************************************//**
 * @file app.c
 * @brief RHT sensor Zigbee example
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

#include "sl_power_manager.h"
#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/util/common/common.h"
#include "app/framework/plugin/network-steering/network-steering.h"
#include \
  "app/framework/plugin/find-and-bind-initiator/find-and-bind-initiator.h"

#include "sl_simple_button_instances.h"
#include "sl_sensor_rht.h"

#if defined(SL_CATALOG_LED0_PRESENT)
#include "sl_led.h"
#include "sl_simple_led_instances.h"
#define led_turn_on(led)  sl_led_turn_on(led)
#define led_turn_off(led) sl_led_turn_off(led);
#define led_toggle(led)   sl_led_toggle(led)
#define STATUS_LED        (&sl_led_led0)
#else // !SL_CATALOG_LED0_PRESENT
#define led_turn_on(led)
#define led_turn_off(led)
#define led_toggle(led)
#endif // SL_CATALOG_LED0_PRESENT
#define led_stop(led) do {                       \
    led_turn_off(led);                           \
    sl_zigbee_af_event_set_inactive(&led_event); \
} while (0)

// Normally, we don't need to send report directly
#define USE_SEND_REPORT_TO_BINDINGS  (0)
#define REPORT_PERIOD_MS             (4000)

#define LED_BLINK_PERIOD_MS          1000

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
#define RHT_MEASUREMENT_ENDPOINT     1 // Endpoint with the RH and Temp ZCL
#define FIND_AND_BIND_DELAY_MS       3000 // Delay for find and bind handler
                                          // execution

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------
#if USE_SEND_REPORT_TO_BINDINGS
static sl_status_t report_attribute(sl_zigbee_af_cluster_id_t cluster,
                                    sl_zigbee_af_attribute_id_t attribute,
                                    uint8_t attribute_type,
                                    uint8_t buff[2]);

#endif

static void network_steering_event_handler(sl_zigbee_af_event_t *event);
static void finding_and_binding_event_handler(sl_zigbee_af_event_t *event);
static void leave_network_event_handler(sl_zigbee_af_event_t *event);
static void attribute_report_event_handler(sl_zigbee_af_event_t *event);
static void led_event_handler(sl_zigbee_af_event_t *event);
static void delay_event_handler(sl_zigbee_af_event_t *event);
static uint8_t binding_table_unicast_binding_count(void);
static void binding_table_print(void);

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------
static sl_zigbee_af_event_t network_steering_event_control;
static sl_zigbee_af_event_t finding_and_binding_event_control;
static sl_zigbee_af_event_t leave_network_event_control;
static sl_zigbee_af_event_t attribute_report_event_control;
static sl_zigbee_af_event_t led_event;
static sl_zigbee_af_event_t delay_event;

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
static bool commissioning = false; // Holds the commissioning status
static bool binding = false; // Holds the binding status

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

  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM0);
  sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM2);
  sl_zigbee_app_debug_print("RHT SED Zigbee example\n");

  // Enable the Si70xx sensor in the WSTK
  sc = sl_sensor_rht_init();
  if (sc != SL_STATUS_OK) {
    sl_zigbee_app_debug_print(
      "Relative Humidity and Temperature sensor initialization failed.\n");
  }

  sl_zigbee_af_isr_event_init(&network_steering_event_control,
                              network_steering_event_handler);
  sl_zigbee_af_isr_event_init(&leave_network_event_control,
                              leave_network_event_handler);
  sl_zigbee_af_isr_event_init(&led_event,
                              led_event_handler);

  sl_zigbee_af_event_init(&finding_and_binding_event_control,
                          finding_and_binding_event_handler);
  sl_zigbee_af_event_init(&attribute_report_event_control,
                          attribute_report_event_handler);
  sl_zigbee_af_event_init(&delay_event,
                          delay_event_handler);

// Print content of binding table
  binding_table_print();
}

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
    led_turn_off(STATUS_LED);
    sl_zigbee_af_event_set_inactive(&attribute_report_event_control);
  } else if (status == SL_STATUS_NETWORK_UP) {
    if (binding_table_unicast_binding_count() > 0) {
      binding = true;
      // If already in a network and bindings are valid, report attributes
      sl_zigbee_af_event_set_inactive(&attribute_report_event_control);
    }
  }
}

// -----------------------------------------------------------------------------
// Push button event handler

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
  sl_zigbee_app_debug_print("%s network %s: 0x%02X\n",
                            "Join",
                            "complete",
                            status);

  if (status != SL_STATUS_OK) {
    commissioning = false;
    led_stop(STATUS_LED);
  } else {
    // On successful join, do find and bind after a short delay
    sl_zigbee_af_event_set_delay_ms(&finding_and_binding_event_control,
                                    FIND_AND_BIND_DELAY_MS);
  }
}

/** @brief Complete
 *
 * This callback is fired by the initiator when the Find and Bind process is
 * complete.
 *
 * @param status Status code describing the completion of the find and bind
 * process Ver.: always
 */
void sl_zigbee_af_find_and_bind_initiator_complete_cb(sl_status_t status)
{
  sl_zigbee_app_debug_print("Find and bind initiator %s: 0x%X\n",
                            "complete",
                            status);

  if (status == SL_STATUS_OK) {
    sl_zigbee_af_event_set_delay_ms(&attribute_report_event_control,
                                    REPORT_PERIOD_MS);
  } else {
    sl_zigbee_app_debug_print("Ensure a valid binding target!\n");
    sl_zigbee_af_event_set_inactive(&attribute_report_event_control);
    binding = false;
  }
  led_stop(STATUS_LED);
}

/** @brief
 *
 * Application framework equivalent of ::emberRadioNeedsCalibratingHandler
 */
void sl_zigbee_af_radio_needs_calibrating_cb(void)
{
  sl_mac_calibrate_current_channel();
}

// -----------------------------------------------------------------------------
//                          Event Handler
// -----------------------------------------------------------------------------

/** @brief Leave Network Event Handler
 *
 * This event handler is called in response to it's respective control
 * activation. It handles the network leaving process.
 *
 */
static void leave_network_event_handler(sl_zigbee_af_event_t *event)
{
  sl_status_t status;

// Clear binding table
  status = sl_zigbee_clear_binding_table();
  sl_zigbee_app_debug_print("%s 0x%x", "Clear binding table\n", status);

// Leave network
  status = sl_zigbee_leave_network(SL_ZIGBEE_LEAVE_NWK_WITH_NO_OPTION);
  sl_zigbee_app_debug_print("%s 0x%x", "leave\n", status);

  commissioning = false;
  binding = false;
}

/** @brief Network Steering Event Handler
 *
 * This event handler is called in response to it's respective control
 * activation. It handles the network steering process. If already in a network
 * it forces the device to report it's attributes: Temperature and relative
 * humidity
 *
 */
static void network_steering_event_handler(sl_zigbee_af_event_t *event)
{
  sl_status_t status;

  if (sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    // Check if the device has successfully established bindings, if not do so
    if (!binding) {
      sl_zigbee_af_event_set_active(&finding_and_binding_event_control);
    } else {
      sl_zigbee_af_event_set_active(&attribute_report_event_control);
    }
    led_stop(STATUS_LED);
  } else {
    // If not in a network, attempt to join one
    status = sl_zigbee_af_network_steering_start();
    sl_zigbee_app_debug_print("%s network %s: 0x%X\n",
                              "Join",
                              "start",
                              status);
    commissioning = true;
    sl_zigbee_af_event_set_active(&led_event);
  }
}

/** @brief Find and Bind Event Handler
 *
 * This event handler is called in response to it's respective control
 * activation. It handles the find and bind process as an initiator. It requires
 * a valid target. Upon a successful procedure, a series of binding will be
 * added to the binding table of the device for matching clusters found in the
 * target.
 *
 */
static void finding_and_binding_event_handler(sl_zigbee_af_event_t *event)
{
  sl_status_t status = sl_zigbee_af_find_and_bind_initiator_start(
    RHT_MEASUREMENT_ENDPOINT);

  sl_zigbee_app_debug_print("Find and bind initiator %s: 0x%X\n",
                            "start",
                            status);

  binding = true;
}

/** @brief Attributes report Event Handler
 *
 * This event handler is called in response to its respective control
 * activation. It will report the MeasuredValue of the RH and Temperature
 * measurement server clusters. Data will be sent though the matching binding
 *
 */
static void attribute_report_event_handler(sl_zigbee_af_event_t *event)
{
  sl_status_t status = SL_STATUS_OK;
  uint32_t rh;
  int32_t t;
  union {
    int16_t t;
    uint16_t rh;
  } attribute;

  if (sl_zigbee_af_network_state() != SL_ZIGBEE_JOINED_NETWORK) {
    return;
  }

  sl_zigbee_af_event_set_delay_ms(&attribute_report_event_control,
                                  REPORT_PERIOD_MS);

  if (SL_STATUS_OK == sl_sensor_rht_get(&rh, &t)) {
    sl_zigbee_app_debug_print("RH: %d, Temperature: %d\n", rh, t);

    // Attribute MeasuredValue = 100 x temperature in degrees Celsius.
    attribute.t = t / 10;
#if USE_SEND_REPORT_TO_BINDINGS
    // Send report directly
    status = report_attribute(ZCL_TEMP_MEASUREMENT_CLUSTER_ID,
                              ZCL_TEMP_MEASURED_VALUE_ATTRIBUTE_ID,
                              ZCL_INT16S_ATTRIBUTE_TYPE,
                              (uint8_t *)&attribute);
#else
    // Just update attribute data to send report to bindings
    status = sl_zigbee_af_write_server_attribute(RHT_MEASUREMENT_ENDPOINT,
                                                 ZCL_TEMP_MEASUREMENT_CLUSTER_ID,
                                                 ZCL_TEMP_MEASURED_VALUE_ATTRIBUTE_ID,
                                                 (uint8_t *)&attribute,
                                                 ZCL_INT16S_ATTRIBUTE_TYPE);
#endif
    sl_zigbee_app_debug_print("%s reported: 0x%X\n",
                              "Temp - MeasuredValue",
                              status);

    // Attribute MeasuredValue = 100 x Relative humidity
    attribute.rh = rh / 10;
#if USE_SEND_REPORT_TO_BINDINGS
    // Send report directly
    status = report_attribute(ZCL_TEMP_MEASUREMENT_CLUSTER_ID,
                              ZCL_TEMP_MEASURED_VALUE_ATTRIBUTE_ID,
                              ZCL_INT16U_ATTRIBUTE_TYPE,
                              (uint8_t *)&attribute);
#else
    // Just update attribute data to send report to bindings
    status = sl_zigbee_af_write_server_attribute(RHT_MEASUREMENT_ENDPOINT,
                                                 ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                                                 ZCL_RELATIVE_HUMIDITY_MEASURED_VALUE_ATTRIBUTE_ID,
                                                 (uint8_t *)&attribute,
                                                 ZCL_INT16U_ATTRIBUTE_TYPE);
#endif
    sl_zigbee_app_debug_print("%s reported: 0x%X\n",
                              "RH - MeasuredValue",
                              status);
  } else {
    sl_zigbee_app_debug_print("Failed to read RHT sensor\n");
  }
}

static void led_event_handler(sl_zigbee_af_event_t *event)
{
  if (commissioning || binding) {
    led_toggle(STATUS_LED);
    sl_zigbee_af_event_set_delay_ms(&delay_event, LED_BLINK_PERIOD_MS << 1);
  }
}

static void delay_event_handler(sl_zigbee_af_event_t *event)
{
  sl_zigbee_af_event_set_active(&led_event);
}

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------
#if USE_SEND_REPORT_TO_BINDINGS

/** @brief Report Attribute
 *
 * This function reports the specified attribute from the specified cluster
 * through a valid binding in the binding table. Therefore, a valid binding
 * entry with a  matching cluster must exist for the report to be successful.
 *
 */
static sl_status_t report_attribute(sl_zigbee_af_cluster_id_t cluster,
                                    sl_zigbee_af_attribute_id_t attribute,
                                    uint8_t attribute_type,
                                    uint8_t buff[2])
{
  sl_status_t status = SL_STATUS_OK;

// Fill attribute record - See af-structs.h for details of
//   ReportAttributeRecord contents.
  uint8_t attribute_record[] = {
    LOW_BYTE(attribute), // uint16_t attributeId
    HIGH_BYTE(attribute),
    attribute_type,        // uint8_t attributeType;
    buff[0], buff[1],      // uint8_t* attributeLocation;
  };

// Fill a ZCL global report attributes command buffer
  sl_zigbee_af_fill_command_global_server_to_client_report_attributes(
    cluster,
    attribute_record,
    sizeof(attribute_record) / sizeof(uint8_t));

// Specify endpoints for command sending
  sl_zigbee_af_set_command_endpoints(RHT_MEASUREMENT_ENDPOINT, 1);

// Use binding table to send unicast command
  status = sl_zigbee_af_send_command_unicast_to_bindings();

  return status;
}

#endif

static uint8_t binding_table_unicast_binding_count(void)
{
  uint8_t i;
  sl_zigbee_binding_table_entry_t result;
  uint8_t bindings = 0;

  for (i = 0; i < sl_zigbee_get_binding_table_size(); i++) {
    sl_status_t status = sl_zigbee_get_binding(i, &result);
    if ((status == SL_STATUS_OK)
        && (result.type == SL_ZIGBEE_UNICAST_BINDING)) {
      bindings++;
    }
  }
  return bindings;
}

static void binding_table_print(void)
{
  uint8_t i;
  sl_zigbee_binding_table_entry_t result;

  const char *typeStrings[] = {
    "EMPTY",
    "UNICA",
    "M2ONE",
    "MULTI",
    "?    ",
  };
  uint8_t bindings = 0;

  sl_zigbee_af_guaranteed_println("#  type   nwk  loc   rem   clus   node   eui");
  for (i = 0; i < sl_zigbee_get_binding_table_size(); i++) {
    sl_status_t status = sl_zigbee_get_binding(i, &result);
    if (status == SL_STATUS_OK) {
      if (result.type > SL_ZIGBEE_UNICAST_BINDING) {
        result.type = 4;  // last entry in the string list above
      }
      if (result.type != SL_ZIGBEE_UNUSED_BINDING) {
        bindings++;
        sl_zigbee_af_guaranteed_print("%d: ", i);
        sl_zigbee_af_guaranteed_print("%p", typeStrings[result.type]);
        sl_zigbee_af_guaranteed_print("  %d    0x%x  0x%x  0x%2x 0x%2x ",
                                      result.networkIndex,
                                      result.local,
                                      result.remote,
                                      result.clusterId,
                                      sl_zigbee_get_binding_remote_node_id(i));
        sl_zigbee_af_print_big_endian_eui64(result.identifier);
        sl_zigbee_af_guaranteed_println("");
      }
    } else {
      sl_zigbee_af_guaranteed_println("0x%x: sl_zigbee_get_binding Error: %x",
                                      status);
      sl_zigbee_af_app_flush();
    }
    sl_zigbee_af_app_flush();
  }
  sl_zigbee_af_guaranteed_println("%d of %d bindings used",
                                  bindings,
                                  sl_zigbee_get_binding_table_size());
}
