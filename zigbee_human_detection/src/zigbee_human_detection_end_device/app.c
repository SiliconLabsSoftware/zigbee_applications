/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
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
#include "app/framework/util/af-main.h"
#include "app/util/common/common.h"
#include "app/framework/plugin/network-steering/network-steering.h"
#include \
  "app/framework/plugin/find-and-bind-initiator/find-and-bind-initiator.h"

#include "sl_simple_button_instances.h"

#include "human_detection_ui.h"
#include "human_detection_ai.h"

#if defined(SL_CATALOG_LED0_PRESENT)
#include "sl_led.h"
#include "sl_simple_led_instances.h"
#define led_turn_on(led)             sl_led_turn_on(led)
#define led_turn_off(led)            sl_led_turn_off(led)
#define led_toggle(led)              sl_led_toggle(led)
#define STATUS_LED                   (&sl_led_led0)
#else // !SL_CATALOG_LED0_PRESENT
#define led_turn_on(led)
#define led_turn_off(led)
#define led_toggle(led)
#endif // SL_CATALOG_LED0_PRESENT
#define led_stop(led) do {                       \
    led_turn_off(led);                           \
    sl_zigbee_af_event_set_inactive(&led_event); \
} while (0)

#define BUTTON0                      0

// Delay for find and bind handler execution
#define FIND_AND_BIND_DELAY_MS       3000
// Delay for run inference
#define RUN_INFERENCE_DELAY_MS       350

#define HUMAN_DETECTION_ENDPOINT     1
#define LED_BLINK_PERIOD_MS          2000

// -----------------------------------------------------------------------------
//                          Global Variables
// -----------------------------------------------------------------------------
static uint8_t human_detection_index = 1; // Not Human
static bool commissioning = false; // Holds the commissioning status
static bool binding = false; // Holds the binding status
static bool initialization_is_ok = false;

// Custom event controls
static sl_zigbee_af_event_t run_inference_event_control;
static sl_zigbee_af_event_t network_control_event_control;
static sl_zigbee_af_event_t finding_and_binding_event_control;
static sl_zigbee_af_event_t attribute_report_event_control;
static sl_zigbee_af_event_t led_event;

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------
static void run_inference_event_handler(sl_zigbee_af_event_t *event);
static void network_control_event_handler(sl_zigbee_af_event_t *event);
static void finding_and_binding_event_handler(sl_zigbee_af_event_t *event);
static void attribute_report_event_handler(sl_zigbee_af_event_t *event);
static void led_event_handler(sl_zigbee_af_event_t *event);
static uint8_t binding_table_unicast_binding_count(void);

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
 * Note: No callback in the Application Framework is
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
  sl_zigbee_app_debug_print("Zigbee Human Detection example\n");

  human_detection_ui_oled_init();
  if (SL_STATUS_OK != human_detection_ai_init()) {
    human_detection_ui_error();
  } else {
    human_detection_ui_init();
    initialization_is_ok = true;
  }

  sl_zigbee_af_event_init(&run_inference_event_control,
                          run_inference_event_handler);
  sl_zigbee_af_event_set_delay_ms(&run_inference_event_control,
                                  RUN_INFERENCE_DELAY_MS);
  sl_zigbee_af_isr_event_init(&network_control_event_control,
                              network_control_event_handler);
  sl_zigbee_af_event_init(&finding_and_binding_event_control,
                          finding_and_binding_event_handler);
  sl_zigbee_af_event_init(&attribute_report_event_control,
                          attribute_report_event_handler);
  sl_zigbee_af_event_init(&led_event, led_event_handler);
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
    sl_zigbee_af_event_set_inactive(&attribute_report_event_control);
    led_stop(STATUS_LED);
  } else if (status == SL_STATUS_NETWORK_UP) {
    // Get nodeID and update to OLED display
    sl_802154_short_addr_t node_id = sl_zigbee_af_get_node_id();
    sl_zigbee_app_debug_print("NodeID = %x\n", node_id);
    human_detection_ui_network_status_update((uint8_t)SL_ZIGBEE_JOINED_NETWORK,
                                             node_id);
    if (binding_table_unicast_binding_count() > 0) {
      binding = true;
      // If already in a network and bindings are valid, report attributes
      sl_zigbee_af_event_set_active(&attribute_report_event_control);
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
    led_stop(STATUS_LED);
    human_detection_ui_network_status_update((uint8_t)SL_ZIGBEE_NO_NETWORK, 0);
  } else {
    // On successful join, do find and bind after a short delay
    sl_zigbee_af_event_set_delay_ms(&finding_and_binding_event_control,
                                    FIND_AND_BIND_DELAY_MS);
  }
  commissioning = false;
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

  if (status != SL_STATUS_OK) {
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

/**
 * This function is called whenever a detection ("human" or "not human")
 * is detected by the neural network model.
 *
 * @param found_command_index The index of the detected keyword. 0 for "human"
 * 1 for "not human"
 */
void human_detection_ai_get_index(uint8_t found_command_index)
{
  human_detection_index = found_command_index;
  sl_zigbee_af_event_set_active(&attribute_report_event_control);
}

/***************************************************************************//**
 * A callback called in interrupt context whenever a button changes its state.
 *
 * @remark Can be implemented by the application if required. This function
 * can contain the functionality to be executed in response to changes of state
 * in each of the buttons, or callbacks to appropriate functionality.
 *
 * @note The button state should not be updated in this function, it is updated
 * by specific button driver prior to arriving here
 *
 * @param[out] handle             Pointer to button instance
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if (SL_SIMPLE_BUTTON_INSTANCE(BUTTON0) == handle) {
    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
      sl_zigbee_af_event_set_active(&network_control_event_control);
    }
  }
}

static void led_event_handler(sl_zigbee_af_event_t *event)
{
  sl_zigbee_af_event_set_inactive(&led_event);

  if (commissioning || binding) {
    led_toggle(STATUS_LED);
    sl_zigbee_af_event_set_delay_ms(&led_event, LED_BLINK_PERIOD_MS << 1);
  }
}

/**
 * This function is called whenever a inference is enabled
 */
static void run_inference_event_handler(sl_zigbee_af_event_t *event)
{
  sl_zigbee_af_event_set_inactive(&run_inference_event_control);

  if (initialization_is_ok == true) {
    human_detection_ai_loop();
  }
  sl_zigbee_af_event_set_delay_ms(&run_inference_event_control,
                                  RUN_INFERENCE_DELAY_MS);
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
  sl_status_t status;
  status = sl_zigbee_af_find_and_bind_initiator_start(HUMAN_DETECTION_ENDPOINT);

  sl_zigbee_app_debug_print("Find and bind initiator %s: 0x%X\n",
                            "start",
                            status);

  binding = true;
}

/** @brief Attributes report Event Handler
 *
 * This event handler write the attribute which triggers an automatic
 * report of the occupancy to the Client via the Occupancy Sensing
 * cluster.
 *
 */
static void attribute_report_event_handler(sl_zigbee_af_event_t *event)
{
  sl_zigbee_af_status_t status = SL_ZIGBEE_ZCL_STATUS_SUCCESS;

  sl_zigbee_af_event_set_inactive(&attribute_report_event_control);

  if (sl_zigbee_af_network_state() != SL_ZIGBEE_JOINED_NETWORK) {
    return;
  }

  status = sl_zigbee_af_write_server_attribute(HUMAN_DETECTION_ENDPOINT,
                                               ZCL_OCCUPANCY_SENSING_CLUSTER_ID,
                                               ZCL_OCCUPANCY_ATTRIBUTE_ID,
                                               (uint8_t *)&human_detection_index,
                                               ZCL_BITMAP8_ATTRIBUTE_TYPE);

  sl_zigbee_app_debug_print("%s reported: 0x%X\n", "Human detection", status);
}

/** @brief Network Control Event Handler
 *
 * This event handler is called in response to it's respective control
 * activation.
 *
 */
static void network_control_event_handler(sl_zigbee_af_event_t *event)
{
  sl_status_t status;

  sl_zigbee_af_event_set_inactive(&network_control_event_control);

  if (sl_zigbee_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    // Clear binding table
    status = sl_zigbee_clear_binding_table();
    sl_zigbee_app_debug_print("%s 0x%x", "Clear binding table\n", status);

    // Leave network
    status = sl_zigbee_leave_network(SL_ZIGBEE_LEAVE_NWK_WITH_NO_OPTION);
    sl_zigbee_app_debug_print("Leave network: 0x%X\n", status);
    commissioning = false;
    binding = false;

    // Update status to OLED display
    human_detection_ui_network_status_update((uint8_t)SL_ZIGBEE_NO_NETWORK, 0);
  } else if (!commissioning) {
    // If not in a network, attempt to join one
    status = sl_zigbee_af_network_steering_start();
    sl_zigbee_app_debug_print("Join network start: 0x%X\n", status);
    sl_zigbee_af_event_set_active(&led_event);
    commissioning = true;

    // Update status to OLED display
    human_detection_ui_network_status_update((uint8_t)SL_ZIGBEE_JOINING_NETWORK,
                                             0);
  }
}

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
