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

#include "sl_simple_led.h"
#include "sl_simple_led_instances.h"

#include "app/framework/include/af.h"
#include "app/framework/plugin/network-steering/network-steering.h"
#include "app/framework/plugin/network-creator/network-creator.h"
#include \
  "app/framework/plugin/network-creator-security/network-creator-security.h"
#include "app/framework/plugin/find-and-bind-target/find-and-bind-target.h"

#include "sl_simple_button.h"
#include "sl_simple_button_instances.h"

#define LIGHT_ENDPOINT               (1)
#define FINDING_AND_BINDING_DELAY_MS 3000
#define LED_BLINK_PERIOD_MS          2000

#define led0_on()     sl_led_turn_on(&sl_led_led0);
#define led0_off()    sl_led_turn_off(&sl_led_led0);
#define led0_toggle() sl_led_toggle(&sl_led_led0);

#define led1_on()     sl_led_turn_on(&sl_led_led1);
#define led1_off()    sl_led_turn_off(&sl_led_led1);
#define led1_toggle() sl_led_toggle(&sl_led_led1);

sli_zigbee_event_t led_event;
sli_zigbee_event_t finding_and_binding_event;

static void on_off_cluster_server_attribute_changed_callback(uint8_t endpoint);

/**
 * @brief       Handler function for event led_event
 * @param       None
 * @return      None
 */
static void led_event_handler(void)
{
  if (sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    uint16_t identify_time;
    sl_zigbee_af_read_server_attribute(LIGHT_ENDPOINT,
                                       ZCL_IDENTIFY_CLUSTER_ID,
                                       ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                                       (uint8_t *)&identify_time,
                                       sizeof(identify_time));
    if (identify_time > 0) {
      led0_toggle();
      sl_zigbee_af_event_set_delay_ms(&led_event,
                                      LED_BLINK_PERIOD_MS);
    } else {
      led0_on();
    }
  } else {
    sl_status_t status = sl_zigbee_af_network_steering_start();
    sl_zigbee_af_debug_print("%p network %p: 0x%X", "Join", "start", status);
    UNUSED_VAR(status);
  }
}

/**
 * @brief       Handler function for event finding_and_binding_event
 * @param       None
 * @return      None
 */
static void finding_and_binding_event_handler()
{
  if (sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    sl_zigbee_af_event_set_inactive(&finding_and_binding_event);
    sl_zigbee_af_find_and_bind_target_start(LIGHT_ENDPOINT);
    sl_zigbee_af_event_set_active(&led_event);
  }
}

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action.  The return code
 * from this callback is ignored by the framework.  The framework will always
 * process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
bool sli_zigbee_af_stack_status_callback(sl_status_t status)
{
  // Note, the ZLL state is automatically updated by the stack and the plugin.
  if (status == SL_STATUS_NETWORK_DOWN) {
    led0_off();
  } else if (status == SL_STATUS_NETWORK_UP) {
    led0_on();
    sl_zigbee_af_event_set_delay_ms(&finding_and_binding_event,
                                    (LED_BLINK_PERIOD_MS * 2));
  }

  // This value is ignored by the framework.
  return false;
}

/**
 * @brief       Override for sl_zigbee_af_main_init_cb() function
 * @param       None
 * @return      None
 */
void sl_zigbee_af_main_init_cb(void)
{
  sl_zigbee_af_event_init(&led_event, led_event_handler);
  sl_zigbee_af_event_init(&finding_and_binding_event,
                          finding_and_binding_event_handler);
}

/** @brief Complete
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to SL_STATUS_OK to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt. Ver.: always
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID. Ver.: always
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network. Ver.: always
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete. Ver.: always
 */
void sl_zigbee_af_network_steering_complete_cb(sl_status_t status,
                                               uint8_t totalBeacons,
                                               uint8_t joinAttempts,
                                               uint8_t finalState)
{
  sl_zigbee_app_debug_print("%p network %p: 0x%X", "Join", "complete", status);

  if (status != SL_STATUS_OK) {
    status = sl_zigbee_af_network_creator_start(false);   // distributed
    sl_zigbee_app_debug_print("%p network %p: 0x%X", "Form", "start", status);
  }
}

/** @brief Complete
 *
 * This callback notifies the user that the network creation process has
 * completed successfully.
 *
 * @param network The network that the network creator plugin successfully
 * formed. Ver.: always
 * @param usedSecondaryChannels Whether or not the network creator wants to
 * form a network on the secondary channels Ver.: always
 */
void sl_zigbee_af_network_creator_complete_cb(
  const sl_zigbee_network_parameters_t *network,
  bool usedSecondaryChannels)
{
  sl_zigbee_af_debug_print("%p network %p: 0x%X",
                           "Form distributed",
                           "complete",
                           SL_STATUS_OK);
}

/** @brief On/off Cluster Server Post Init
 *
 * Following resolution of the On/Off state at startup for this endpoint,
 *   perform any
 * additional initialization needed; e.g., synchronize hardware state.
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 */
void sl_zigbee_af_on_off_cluster_server_post_init_cb(uint8_t endpoint)
{
  // At startup, trigger a read of the attribute and possibly a toggle of the
  // LED to make sure they are always in sync.
  on_off_cluster_server_attribute_changed_callback(endpoint);
}

/** @brief Post Attribute Change
 *
 * This function is called by the application framework after it changes an
 * attribute value. The value passed into this callback is the value to which
 * the attribute was set by the framework.
 */
void sl_zigbee_af_post_attribute_change_cb(uint8_t endpoint,
                                           sl_zigbee_af_cluster_id_t clusterId,
                                           sl_zigbee_af_attribute_id_t attributeId,
                                           uint8_t mask,
                                           uint16_t manufacturerCode,
                                           uint8_t type,
                                           uint8_t size,
                                           uint8_t *value)
{
  if ((clusterId == ZCL_ON_OFF_CLUSTER_ID)
      && (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID)
      && (mask == CLUSTER_MASK_SERVER)) {
    on_off_cluster_server_attribute_changed_callback(endpoint);
  }
}

/** @brief Server Attribute Changed
 *
 * On/off cluster, Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
static void on_off_cluster_server_attribute_changed_callback(uint8_t endpoint)
{
  // When the on/off attribute changes, set the LED appropriately.  If an error
  // occurs, ignore it because there's really nothing we can do.
  uint8_t on_off;

  if (sl_zigbee_af_read_server_attribute(endpoint,
                                         ZCL_ON_OFF_CLUSTER_ID,
                                         ZCL_ON_OFF_ATTRIBUTE_ID,
                                         (uint8_t *)&on_off,
                                         sizeof(on_off))
      == SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
    if (on_off) {
      led1_on();
    } else {
      led1_off();
    }
  }
}
