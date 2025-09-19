/***************************************************************************//**
 * @file app.c
 * @brief Top level application functions
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
#include "zigbee_sleep_config.h"
#include "network-steering.h"
#include "zll-commissioning.h"
#include "find-and-bind-initiator.h"
#include "af.h"
#include "network-formation.h"
#include "sl_simple_button_instances.h"

#define TRANSITION_TIME_DS           20
#define FINDING_AND_BINDING_DELAY_MS 3000
#define BUTTON0                      0
#define SWITCH_ENDPOINT              1

static bool commissioning = false;

static sl_zigbee_af_event_t commissioning_event;
static sl_zigbee_af_event_t finding_and_binding_event;

// ---------------
// Event handlers

static void commissioning_event_handler(sl_zigbee_af_event_t *event)
{
  sl_status_t status;
  uint16_t bytes;

  if (sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    sl_zigbee_af_get_command_aps_frame()->sourceEndpoint = SWITCH_ENDPOINT;
    bytes = sl_zigbee_af_fill_command_on_off_cluster_toggle();
    if (bytes > 0) {
      sl_zigbee_af_send_command_unicast_to_bindings();
    } else {
      sl_zigbee_app_debug_print("Fill command failed!\r\n");
    }
  } else {
    status = sl_zigbee_af_network_steering_start();
    if (status == SL_STATUS_OK) {
      sl_zigbee_app_debug_print("sl_zigbee_af_network_steering_start OK\r\n");
    } else {
      sl_zigbee_app_debug_print("Initiate network failed: 0x%02X\r\n",
                                status);
    }
  }
}

static void sl_zigbee_af_finding_and_binding_event_handler(
  sl_zigbee_af_event_t *event)
{
  sl_zigbee_app_debug_print("sl_zigbee_af_finding_and_binding_event_handler\r\n");
  sl_zigbee_af_find_and_bind_initiator_start(SWITCH_ENDPOINT);
}

// ----------------------
// Implemented Callbacks

void sl_zigbee_af_main_init_cb(void)
{
  sl_zigbee_af_isr_event_init(&commissioning_event,
                              commissioning_event_handler);
  sl_zigbee_af_event_init(&finding_and_binding_event,
                          sl_zigbee_af_finding_and_binding_event_handler);
}

/** @brief Complete network steering.
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to EMBER_SUCCESS to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt.
 *
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID.
 *
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network.
 *
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete.
 */
void sl_zigbee_af_network_steering_complete_cb(sl_status_t status,
                                               uint8_t totalBeacons,
                                               uint8_t joinAttempts,
                                               uint8_t finalState)
{
  sl_zigbee_app_debug_print("%s network %s: 0x%02X\r\n",
                            "Join",
                            "complete",
                            status);

  if (status != SL_STATUS_OK) {
    commissioning = false;
  } else {
    sl_zigbee_af_event_set_delay_ms(&finding_and_binding_event,
                                    FINDING_AND_BINDING_DELAY_MS);
  }
}

/** @brief Touch Link Complete
 *
 * This function is called by the ZLL Commissioning Common plugin when touch
 *   linking
 * completes.
 *
 * @param networkInfo The ZigBee and ZLL-specific information about the network
 * and target. Ver.: always
 * @param deviceInformationRecordCount The number of sub-device information
 * records for the target. Ver.: always
 * @param deviceInformationRecordList The list of sub-device information
 * records for the target. Ver.: always
 */
void sl_zigbee_af_zll_commissioning_common_touch_link_complete_cb(
  const sl_zigbee_zll_network_t *networkInfo,
  uint8_t deviceInformationRecordCount,
  const sl_zigbee_zll_device_info_record_t *deviceInformationRecordList)
{
  sl_zigbee_app_debug_print("%s network %s: 0x%02X\r\n",
                            "Touchlink",
                            "complete",
                            SL_STATUS_OK);

  sl_zigbee_af_event_set_delay_ms(&finding_and_binding_event,
                                  FINDING_AND_BINDING_DELAY_MS);
}

/** @brief Touch Link Failed
 *
 * This function is called by the ZLL Commissioning Client plugin if touch
 *   linking
 * fails.
 *
 * @param status The reason the touch link failed. Ver.: always
 */
void sl_zigbee_af_zll_commissioning_client_touch_link_failed_cb(
  sl_zigbee_af_zll_commissioning_status_t status)
{
  sl_zigbee_app_debug_print("%s network %s: 0x%02X\r\n",
                            "Touchlink",
                            "complete",
                            SL_STATUS_FAIL);

  commissioning = false;
}

/** @brief Find and Bind Complete
 *
 * This callback is fired by the initiator when the Find and Bind process is
 * complete.
 *
 * @param status Status code describing the completion of the find and bind
 * process Ver.: always
 */
void sl_zigbee_af_find_and_bind_initiator_complete_cb(sl_status_t status)
{
  sl_zigbee_app_debug_print("Find and bind initiator %s: 0x%02X\r\n",
                            "complete",
                            status);
  commissioning = false;
}

/** @brief
 *
 * Application framework equivalent of ::emberRadioNeedsCalibratingHandler
 */
void sl_zigbee_af_radio_needs_calibrating_cb(void)
{
  sl_mac_calibrate_current_channel();
}

#if defined(SL_CATALOG_SIMPLE_BUTTON_PRESENT) \
  && (SL_ZIGBEE_APP_FRAMEWORK_USE_BUTTON_TO_STAY_AWAKE == 0)
#include "sl_simple_button.h"
#include "sl_simple_button_instances.h"

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
 *    @param[out] handle             Pointer to button instance
 ******************************************************************************/
void sl_button_on_change(const sl_button_t *handle)
{
  if (SL_SIMPLE_BUTTON_INSTANCE(BUTTON0) == handle) {
    if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
      sl_zigbee_af_event_set_active(&commissioning_event);
    }
  }
}

#endif

// Internal testing stuff
#if defined(SL_ZIGBEE_TEST)
void sl_zigbee_af_hal_button_isr_cb(uint8_t button,
                                    uint8_t state)
{
  if (state == BUTTON_RELEASED) {
    sl_zigbee_event_set_active(&commissioning_event);
  }
}

#endif // SL_ZIGBEE_TEST
