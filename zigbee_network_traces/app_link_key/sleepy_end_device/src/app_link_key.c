/***************************************************************************//**
 * @file appLinkKey.c
 * @brief File with Events and Function to create & open the network.
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

/***************************************************************************//**
 * DESCRIPTION OF THE FILE
 *  This file allows the user to request an application link key to communicate
 *  with another device in the network specified by its eui64.
 *    - appLKey {eui64 of the remote node}
 *
 *  HOW TO USE :
 *   Include the header to the app.c
 *   In the emberAfMainInitCallback(), initialize the custom command group such
 *   as :
 *   sl_cli_command_add_command_group(sl_cli_handles[0],
 *   &my_request_cli_command_group);
 *   (refer to app.c of this project as an example).
 ******************************************************************************/

/***************************************************************************//**
 * Variable declarations / Includes.
 ******************************************************************************/

#include "app_link_key.h"

static const sl_cli_command_info_t my_request_link_command =
  SL_CLI_COMMAND(my_request_app_link_key_handler,
                 "Function to request App Link Key",
                 "EUI64 of the other node",
                 { SL_CLI_ARG_HEX, SL_CLI_ARG_END });

static const sl_cli_command_entry_t my_request_cli_commands[] = {
  { "appLKey", &my_request_link_command, false },
  { NULL, NULL, false },
};

sl_cli_command_group_t my_request_cli_command_group = {
  { NULL },
  false,
  my_request_cli_commands
};

/***************************************************************************//**
 * Functions & events.
 ******************************************************************************/

/**
 * @brief CLI Handler to request the TC an Application Link Key
 *
 * @param args
 */
void my_request_app_link_key_handler(sl_cli_command_arg_t *args)
{
  sl_status_t status;
  sl_802154_long_addr_t eui64;
  sl_zigbee_af_application_task_t currentTask;
  sl_zigbee_copy_eui64_arg(args, 0, eui64, true);

  /* Fast Poll Mode enabled */
  // Waiting for the APS Link Key
  currentTask = SL_ZIGBEE_AF_FORCE_SHORT_POLL;
  sl_zigbee_af_add_to_current_app_tasks_cb(currentTask);
  sl_zigbee_af_set_wake_timeout_bitmask_cb(currentTask);

  // Request the Link key
  status = sl_zigbee_request_link_key(eui64);
  if (status == SL_STATUS_OK) {
    sl_zigbee_core_debug_println("Successfully requested the Link key");
  }
}
