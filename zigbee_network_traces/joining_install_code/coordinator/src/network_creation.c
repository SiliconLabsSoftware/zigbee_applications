/***************************************************************************//**
 * @file network_creation.c
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
 *  This file grants you a set of custom CLI commands to form a network and
 *  open it for install-code
 *    - form : Create the Network
 *    - open : Allows to add a Transient Key to the TC to allow a specific
 *   device
 *    to join with its install-code key
 *
 *  HOW TO USE :
 *    Include this file and corresponding header files
 *    Create the sl_zigbee_af_main_init_cb() into the app.c file and initialize
 *    the command group : my_cli_command_group, and events
 *    (refer to app.c of this project as an example)
 ******************************************************************************/

/***************************************************************************//**
 * Variable declarations / Includes.
 ******************************************************************************/
#include "stack/include/zigbee-security-manager.h"
#include "network_creation.h"

sl_zigbee_af_event_t network_form_ctrl;
sl_zigbee_af_event_t network_opne_ctrl;
static sl_802154_short_addr_t node_table[3] = { 0 };
static int current_index = 0;

/// Create the CLI_Command_info
static const sl_cli_command_info_t my_form_create_command =
  SL_CLI_COMMAND(event_network_form_handler,
                 "Form and Create custom Network",
                 "No argument",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t my_open_install_code_command =
  SL_CLI_COMMAND(network_open_with_code_handler,
                 "Open the network with Install-code",
                 "index<7:0>" SL_CLI_UNIT_SEPARATOR "eui64" SL_CLI_UNIT_SEPARATOR "install-code" SL_CLI_UNIT_SEPARATOR,
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_HEX, SL_CLI_ARG_HEX, SL_CLI_ARG_END,
                 });

/// Create the entries
const sl_cli_command_entry_t my_cli_commands[] = {
  { "form", &my_form_create_command, false },
  { "open", &my_open_install_code_command, false },
  { NULL, NULL, false },
};

/// Create the group of entries
sl_cli_command_group_t my_cli_command_group = {
  { NULL },
  false,
  my_cli_commands
};

/***************************************************************************//**
 * Functions & events.
 ******************************************************************************/

/**
 * @brief Callback when Network Creator is completed
 *
 * @param network Network parameters
 * @param usedSecondaryChannels Flag indicating use of Secondary channels
 */
void sl_zigbee_af_network_creator_complete_cb(
  const sl_zigbee_network_parameters_t *network,
  bool usedSecondaryChannels)
{
  // Launch Open Network Event
  sl_zigbee_core_debug_println("PanId : %d", network->panId);
}

void sl_zigbee_af_trust_center_join_cb(sl_802154_short_addr_t newNodeId,
                                       sl_802154_long_addr_t newNodeEui64,
                                       sl_802154_short_addr_t parentOfNewNode,
                                       sl_zigbee_device_update_t status,
                                       sl_zigbee_join_decision_t decision)
{
  sl_zigbee_core_debug_println(
    "\n_______________TC Node joined Callback__________________");
  sl_zigbee_core_debug_println("Child 0x%x%x has %s the channel",
                               ((newNodeId >> 8) & 0xff),
                               (newNodeId & 0xff),
                               ((status)
                                != SL_ZIGBEE_DEVICE_LEFT) ? "joined" : "left");
  // JOINING ?
  if (status != SL_ZIGBEE_DEVICE_LEFT) {
    node_table[current_index] = newNodeId;
    sl_zigbee_core_debug_println("Current NodeID : 0x%x%x",
                                 (newNodeId >> 8) & 0xff,
                                 (newNodeId & 0xff));
    current_index += 1;
  }
  sl_zigbee_core_debug_println(
    "________________________________________________________\n");
}

/**
 * @brief CLI Function to form the network when : form
 *
 * @param arguments CLI arguments when forming Network
 */
void event_network_form_handler(sl_cli_command_arg_t *arguments)
{
  sl_status_t status;
  sl_zigbee_core_debug_println("Network formation");

  sl_zigbee_network_status_t state;

  // Network Creation
  state = sl_zigbee_af_network_state();

  if (state != SL_ZIGBEE_JOINED_NETWORK) {
    status = sl_zigbee_af_network_creator_start(true);
    sl_zigbee_core_debug_println("%p network %p: 0x%X",
                                 "Form",
                                 "start",
                                 status);
  } else {
    sl_zigbee_core_debug_println("Network already created");
  }
}

/**
 * @brief CLI Function to open the network with Install code
 *
 * @param arguments CLI arguments after open
 */
void network_open_with_code_handler(sl_cli_command_arg_t *arguments)
{
  sl_status_t status;
  sl_802154_long_addr_t eui64;
  sl_zigbee_key_data_t keyData;
  sl_zigbee_network_status_t state;
  size_t len = 18; // 16 bytes
  uint8_t code[18];

  sl_zigbee_core_debug_println("Open Network with Install Code");

  sl_zigbee_copy_eui64_arg(arguments, 1, eui64, true);
  sl_zigbee_copy_hex_arg(arguments, 2, code, sizeof(code), false);
  // Get the derived key from the code
  status = sli_zigbee_af_install_code_to_key(code, len, &keyData);

  state = sl_zigbee_af_network_state();
  // Check if Network Created
  if (state != SL_ZIGBEE_JOINED_NETWORK) {
    sl_zigbee_core_debug_println("Network not Joined, cannot open the network");
  } else {
    sl_zigbee_core_debug_println("Network UP, opening process launched");
    sl_zigbee_core_debug_print("EUI64 of the joining device: ");
    sl_zigbee_af_print_big_endian_eui64(eui64);
    sl_zigbee_core_debug_println("");
    sl_zigbee_core_debug_print("Install Code: ");
    sl_zigbee_core_debug_print_buffer(code, sizeof(code) - 2, true);
    sl_zigbee_core_debug_println("");
    sl_zigbee_core_debug_print("Install Code CRC: ");
    sl_zigbee_core_debug_print_buffer(&code[sizeof(code) - 2], 2, true);
    sl_zigbee_core_debug_println("");
    sl_zigbee_core_debug_print("Derived key from Install-Code: ");
    sl_zigbee_core_debug_print_buffer(keyData.contents,
                                      sizeof(keyData.contents),
                                      true);
    sl_zigbee_core_debug_println("");
    status = sl_zigbee_af_network_creator_security_open_network_with_key_pair(
      eui64,
      keyData);
    sl_zigbee_core_debug_println("Network Open with Key : 0x%X", status);
  }
}
