/***************************************************************************//**
 * @file network_rejoin.c
 * @brief File with Events and Function to rejoin a network.
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
 *  This file grants you a set of custom CLI commands to rejoin a network
 *    - rejoin : Launch the rejoin process
 *
 *  HOW TO USE :
 *    Include the header file in your app.c
 *    Create the emberAfMainInitCallback() into the app.c file and initialize
 *    the command group : my_rejoin_command_group, and events if necessary
 *    (refer to app.c of this project as an example)
 ******************************************************************************/

/***************************************************************************//**
 * Variable declarations / Includes.
 ******************************************************************************/

#include "network_rejoin.h"

static const sl_cli_command_info_t my_rejoin_command =
  SL_CLI_COMMAND(my_rejoin_handler,
                 "Function to rejoin the network",
                 "None",
                 { SL_CLI_ARG_END });

static const sl_cli_command_entry_t my_cli_commands[] = {
  { "rejoin", &my_rejoin_command, false },
  { NULL, NULL, false },
};

sl_cli_command_group_t my_rejoin_command_group = {
  { NULL },
  false,
  my_cli_commands
};

/***************************************************************************//**
 * Functions & events.
 ******************************************************************************/

/**
 * @brief CLI Handler to rejoin a network with join command
 *
 * @param arguments CLI arguments after the rejoin
 */
void my_rejoin_handler(sl_cli_command_arg_t *arguments)
{
  // Launch the join process
  sl_status_t status;
  status = sl_zigbee_find_and_rejoin_network(false,
                                             SL_ZIGBEE_ALL_802_15_4_CHANNELS_MASK,
                                             SL_ZIGBEE_REJOIN_REASON_NONE,
                                             SL_ZIGBEE_SLEEPY_END_DEVICE);

  // Check the status of the network Steering
  if (status == SL_STATUS_OK) {
    sl_zigbee_core_debug_println("Start of Network Steering successful");
  } else {
    sl_zigbee_core_debug_println("ERROR to start Network Steering");
  }
}
