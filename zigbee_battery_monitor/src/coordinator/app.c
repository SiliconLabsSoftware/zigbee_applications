/***************************************************************************//**
 * @file app.c
 * @brief Callbacks implementation and application specific code.
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
#include "find-and-bind-target.h"

#define END_POINT_1  1
static sl_zigbee_af_event_t finding_and_binding_event;

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
  UNUSED_VAR(totalBeacons);
  UNUSED_VAR(joinAttempts);
  UNUSED_VAR(finalState);
  sl_zigbee_app_debug_println("%s network %s: 0x%02X",
                              "Join",
                              "complete",
                              status);
}

/** @brief
 * RadioNeedsCalibratingHandler
 */
void sl_zigbee_af_radio_needs_calibrating_cb(void)
{
  sl_mac_calibrate_current_channel();
}

/** @brief Post Attribute Change
 *
 * This function is called by the application framework after it changes an
 * attribute value. The value passed into this callback is the value to which
 * the attribute was set by the framework.
 *
 * @param endpoint   Ver.: always
 * @param clusterId   Ver.: always
 * @param attributeId   Ver.: always
 * @param mask   Ver.: always
 * @param manufacturerCode   Ver.: always
 * @param type   Ver.: always
 * @param size   Ver.: always
 * @param value   Ver.: always
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
  UNUSED_VAR(manufacturerCode);
  UNUSED_VAR(type);
  UNUSED_VAR(size);
  UNUSED_VAR(value);
  if ((clusterId == ZCL_POWER_CONFIG_CLUSTER_ID)
      && (attributeId == ZCL_BATTERY_VOLTAGE_ATTRIBUTE_ID)
      && (mask == CLUSTER_MASK_CLIENT)) {
    uint8_t voltageDeciV;

    if (sl_zigbee_af_read_server_attribute(endpoint,
                                           ZCL_POWER_CONFIG_CLUSTER_ID,
                                           ZCL_BATTERY_VOLTAGE_ATTRIBUTE_ID,
                                           &voltageDeciV,
                                           sizeof(voltageDeciV))
        == SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
      sl_zigbee_af_app_println("New voltage reading: %d mV", voltageDeciV);
    }
  }
}

static void finding_and_binding_event_handler(sl_zigbee_af_event_t *event)
{
  UNUSED_VAR(event);
  if (sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    sl_zigbee_af_event_set_inactive(&finding_and_binding_event);
    sl_zigbee_app_debug_println("Find and bind target start: 0x%02X",
                                sl_zigbee_af_find_and_bind_target_start(
                                  END_POINT_1));
  }
}

/** @brief
 * A callback invoked when the status of the stack changes. If the status
 * parameter equals SL_STATUS_NETWORK_UP, then the getNetworkParameters command can
 * be called to obtain the new network parameters. If any of the parameters are
 * being stored in nonvolatile memory by the Host, the stored values should be
 * updated.
 * @param status Stack status
 */
void sl_zigbee_af_stack_status_cb(sl_status_t status)
{
  // Note, the ZLL state is automatically updated by the stack and the plugin.
  if (status == SL_STATUS_NETWORK_UP) {
    sl_zigbee_af_event_set_active(&finding_and_binding_event);
  }
}

/** @brief Init
 * Application init function
 */
void sl_zigbee_af_main_init_cb(void)
{
  sl_zigbee_af_event_init(&finding_and_binding_event,
                          finding_and_binding_event_handler);
}

/** @brief Report Attributes
 *
 * This function is called by the application framework when a Report Attributes
 * command is received from an external device.  The application should return
 * true if the message was processed or false if it was not.
 *
 * @param clusterId The cluster identifier of this command.  Ver.: always
 * @param buffer Buffer containing the list of attribute report records.  Ver.:
 * always
 * @param bufLen The length in bytes of the list.  Ver.: always
 */
bool sl_zigbee_af_report_attributes_cb(sl_zigbee_af_cluster_id_t clusterId,
                                       int8u *buffer,
                                       int16u bufLen)
{
  int16_t attribute;
  uint16_t battery_voltage;

  sl_zigbee_af_app_println("clusterId: %d", clusterId);

  // Custom processing done only for cluster 0x405 and 0x402 (RH and Temperature
  // measurement clusters)
  if ((clusterId == ZCL_POWER_CONFIG_CLUSTER_ID)
      || (clusterId == ZCL_BATTERY_VOLTAGE_ATTRIBUTE_ID)) {
    attribute = buffer[bufLen - 2];
    battery_voltage = buffer[bufLen - 1] * 100;
    sl_zigbee_af_app_println("buffer: %d", battery_voltage);
    sl_zigbee_af_app_println("attribute: %x", attribute);
  }
  return true;
}
