/***************************************************************************//**
 * @file nwk_key_update.c
 * @brief
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

/***************************************************************************//**
 * DESCRIPTION OF THE FILE
 *  This file grants you a set of custom CLI commands to update the NWK key of
 *  the current network.
 *
 *  HOW TO USE :
 *    Include this file and corresponding header files
 *    Create the emberAfMainInitCallback() into the app.c file and initialize
 *    the command group : my_update_nwk_command_group,
 *    (refer to app.c of this project as an example)
 ******************************************************************************/

/***************************************************************************//**
 * Variable declarations / Includes.
 ******************************************************************************/

#include "nwk_key_update.h"

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

/**
 * @brief Callback when Network Key update complete
 *
 * @param status Status of the NWK key update
 */
void sl_zigbee_af_network_key_update_complete_cb(sl_status_t status)
{
  sl_zigbee_core_debug_println(
    "Network Key Update Complete : ",
    (status == SL_STATUS_OK) ? "Success" : "Failed");
  if (status == SL_STATUS_OK) {
    // Display new NWK Key
    sl_zigbee_sec_man_context_t context;
    sl_zigbee_sec_man_key_t plaintext_key;

    sl_zigbee_sec_man_init_context(&context);
    context.core_key_type = SL_ZB_SEC_MAN_KEY_TYPE_NETWORK;
    sl_zigbee_sec_man_export_key(&context, &plaintext_key);
    sl_zigbee_core_debug_print("Current NWK Key : ");
    sl_zigbee_af_print_zigbee_key(plaintext_key.key);
  }
}

/**
 * @brief Custom CLI to launch a Network Key Update
 * The TC waits 9 sec before switching to this new key
 *
 * @param context Context of the handler
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
