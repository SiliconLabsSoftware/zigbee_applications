/***************************************************************************//**
 * @file nwkKeyUpdate.c
 * @brief source file
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
#include "nwk_key_update.h"
#include "zigbee_app_framework_event.h"
#include "zigbee-security-manager.h"

sl_zigbee_af_event_t network_update_ctrl;

static const sl_cli_command_info_t my_nwk_update_command =
  SL_CLI_COMMAND(network_key_update_handler,
                 "Network Update Handler",
                 "No Argument",
                 { SL_CLI_ARG_END, });

/// Create the entries
static const sl_cli_command_entry_t my_update_nwk_cli_commands[] = {
  { "updateNwk", &my_nwk_update_command, false },
  { NULL, NULL, false },
};

/// Create the group of entries
sl_cli_command_group_t my_update_nwk_command_group = {
  { NULL },
  false,
  my_update_nwk_cli_commands
};

/***************************************************************************//**
 * Functions & events.
 ******************************************************************************/

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
    context.key_index = 0;    // 0 for current/active network key, 1 for alternate

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
 * @brief Custom CLI to launch a Network Key Update
 * The TC waits 9 sec before switching to this new key
 *
 * @param context
 */
void network_key_update_handler(sl_cli_command_arg_t *arguments)
{
  sl_status_t status;

  status = sl_zigbee_af_trust_center_start_network_key_update();
  // Check
  if (status == SL_STATUS_OK) {
    sl_zigbee_core_debug_println("Successfully updated NWK key");
  } else {
    sl_zigbee_core_debug_println("Failed to update NWK Key");
  }
}
