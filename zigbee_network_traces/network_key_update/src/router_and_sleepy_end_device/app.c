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
#include "network-steering.h"
#include "zigbee-security-manager.h"
#include "network_join.h"

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
}

/** @brief
 *
 * Application framework equivalent of ::emberRadioNeedsCalibratingHandler
 */
void sl_zigbee_af_radio_needs_calibrating_cb(void)
{
  sl_mac_calibrate_current_channel();
}

/** @brief Network Key Update Complete
 *
 * This is called by the framework when a network key update operation started
 * by the trust center is complete.
 *
 * @param status   Ver.: always
 */
void sl_zigbee_af_network_key_update_complete_cb(sl_status_t status)
{
  sl_zigbee_sec_man_key_t nwkKey;
  sl_zigbee_core_debug_println("Network Key Update Complete : ",
                               (status == SL_STATUS_OK) ? "Success" : "Failed");
  if (status == SL_STATUS_OK) {
    sl_zigbee_sec_man_context_t context;

    // Initialize the context
    sl_zigbee_sec_man_init_context(&context);

    // Set the context to export the current network key
    context.core_key_type = SL_ZB_SEC_MAN_KEY_TYPE_NETWORK;
    context.key_index = 0;     // 0 for current/active network key, 1 for alternate

    sl_status_t stt = sl_zigbee_sec_man_export_key(&context, &nwkKey);

    if (SL_STATUS_OK == stt) {
      // Display new NWK Key
      sl_zigbee_core_debug_println("Current NWK Key: ");
      sl_zigbee_af_print_zigbee_key(nwkKey.key);
    } else {
      sl_zigbee_core_debug_println("Export key error 0x%lx", stt);
    }
  }
}

/**
 * Main Callback called after initialization of the stack
 * WARNING : All the stack is not completely initialized
 *           You must wait set event after a little delay
 */
void sl_zigbee_af_main_init_cb(void)
{
  // Initialize Events
  sl_zigbee_af_event_init(&event_router_info_ctrl,
                          my_event_router_info_handler);

  // Initialize custom group event
  sl_cli_command_add_command_group(sl_cli_handles[0], &my_cli_command_group);

  // Launch Events
  sl_zigbee_af_event_set_delay_ms(&event_router_info_ctrl, INFO_DELAY_MS);
}
