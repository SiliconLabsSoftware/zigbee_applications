/***************************************************************************//**
 * @file networkCreation.c
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
#include "network_creation.h"

static sl_802154_short_addr_t node_table[3] = { 0 };
static int current_index = 0;

/// Create the CLI_Command_info
static const sl_cli_command_info_t my_form_create_command =
  SL_CLI_COMMAND(event_network_form_handler,
                 "Form and Create custom Network",
                 "No argument",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t my_open_command =
  SL_CLI_COMMAND(network_open_handler,
                 "Open the network with well-known key",
                 "No Argument",
                 { SL_CLI_ARG_END, });

/// Create the entries
const sl_cli_command_entry_t my_cli_commands[] = {
  { "form", &my_form_create_command, false },
  { "open", &my_open_command, false },
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
  // Launch Open Network Event
  sl_zigbee_core_debug_println("PanId : %d", network->panId);
}

/** @brief Trust Center Join
 *
 * This callback is called from within the application framework's
 * implementation of sl_zigbee_internal_trust_center_join_handler or sl_zigbee_ezsp_trust_center_join_handler.
 * This callback provides the same arguments passed to the
 * TrustCenterJoinHandler. For more information about the TrustCenterJoinHandler
 * please see documentation included in stack/include/trust-center.h.
 *
 * @param newNodeId   Ver.: always
 * @param newNodeEui64   Ver.: always
 * @param parentOfNewNode   Ver.: always
 * @param status   Ver.: always
 * @param decision   Ver.: always
 */
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
 * @param arguments CLI arguments after form
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
    sl_zigbee_core_debug_println("%p network %p: 0x%X", "Form", "start",
                                 status);
  } else {
    sl_zigbee_core_debug_println("Network already created");
  }
}

/**
 * @brief CLI Function to open the network with to join with well-known key
 *
 * @param arguments CLI arguments after open
 */
void network_open_handler(sl_cli_command_arg_t *arguments)
{
  sl_status_t status;
  sl_zigbee_network_status_t state;

  sl_zigbee_core_debug_println("Open Network with well-Known key");
  state = sl_zigbee_af_network_state();

  // Check if Network Created
  if (state != SL_ZIGBEE_JOINED_NETWORK) {
    sl_zigbee_core_debug_println("Network not Joined, cannot open the network");
  } else {
    sl_zigbee_core_debug_println("Network UP, opening process launched");
    status = sl_zigbee_af_network_creator_security_open_network();
    sl_zigbee_core_debug_println("Network Open with Key : 0x%X", status);
  }
}
