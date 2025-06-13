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

sl_zigbee_af_event_t sl_zigbee_time_sync_event_control;
static void sl_zigbee_time_sync_event_handler();

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

static void sl_zigbee_time_sync_event_handler()
{
  sl_zigbee_af_event_set_inactive(&sl_zigbee_time_sync_event_control);

  if (sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    // send read attribute
    uint8_t timeAttributeIds[] = {
      LOW_BYTE(ZCL_TIME_ATTRIBUTE_ID),
      HIGH_BYTE(ZCL_TIME_ATTRIBUTE_ID)
    };
    sl_zigbee_af_fill_command_global_client_to_server_read_attributes(
      ZCL_TIME_CLUSTER_ID,
      timeAttributeIds,
      sizeof(
        timeAttributeIds));
    sl_zigbee_af_get_command_aps_frame()->sourceEndpoint = 1;
    sl_zigbee_af_get_command_aps_frame()->destinationEndpoint = 1;
    sl_status_t status =
      sl_zigbee_af_send_command_unicast(SL_ZIGBEE_OUTGOING_DIRECT, 0x0000);
    sl_zigbee_af_core_println("Query time from the gateway status=0x%X",
                              status);
  }
  sl_zigbee_af_event_set_delay_ms(&sl_zigbee_time_sync_event_control, 5000);
}

bool sl_zigbee_af_stack_status_cb(sl_status_t status)
{
  if (status == SL_STATUS_NETWORK_DOWN) {
    sl_zigbee_af_event_set_inactive(&sl_zigbee_time_sync_event_control);
  } else if (status == SL_STATUS_NETWORK_UP) {
    sl_zigbee_af_event_set_delay_ms(&sl_zigbee_time_sync_event_control, 5000);
  }

  // This value is ignored by the framework.
  return false;
}

bool sl_zigbee_af_read_attributes_response_cb(
  sl_zigbee_af_cluster_id_t clusterId,
  int8u *buffer,
  int16u bufLen)
{
  if (ZCL_TIME_CLUSTER_ID != clusterId) {
    return false;
  }

  // attribute ID (2B) + status (1B) + date type (0B or 1B) + value (4B)
  if (bufLen < 7) {
    return false;
  }

  if ((sl_zigbee_af_get_int16u(buffer, 0, bufLen) == ZCL_TIME_ATTRIBUTE_ID)
      && (sl_zigbee_af_get_int8u(buffer, 2,
                                 bufLen) == SL_ZIGBEE_ZCL_STATUS_SUCCESS)) {
    sl_zigbee_af_set_time(sl_zigbee_af_get_int32u(buffer, 4, bufLen));
    sl_zigbee_af_core_println("time sync ok, time: %4x",
                              sl_zigbee_af_get_current_time());
    sl_zigbee_af_event_set_delay_ms(&sl_zigbee_time_sync_event_control,
                                    MILLISECOND_TICKS_PER_DAY);

    return true;
  }

  return false;
}

void sl_zigbee_af_main_init_cb(void)
{
  sl_zigbee_af_event_init(&sl_zigbee_time_sync_event_control,
                          sl_zigbee_time_sync_event_handler);
  sl_zigbee_af_event_set_active(&sl_zigbee_time_sync_event_control);
}
