/***************************************************************************//**
 * @file
 * @brief trust_center_swap_out_callbacks.c
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
 *
 * EVALUATION QUALITY
 * This code has been minimally tested to ensure that it builds with the
 * specified dependency versions and is suitable as a demonstration for
 * evaluation purposes only.
 * This code will be maintained at the sole discretion of Silicon Labs.
 *
 ******************************************************************************/

/**********************************************************************************************************************************************************************
 * General Includes
 **********************************************************************************************************************************************************************/
#include "af.h"
#include <stdlib.h>
#include <string.h>
// Be sure to include the header file for this sample application.
#include "trust_center_swap_out_callbacks.h"
#include "trust_center_swap_out_cli_defs.h"
#include "sl_cli.h"
#include "stack/include/zigbee-security-manager.h"
#include "sl_token_manager_api.h"
#include "sl_token_manufacturing_generic.h"
#include "sl_token_api.h"

/**********************************************************************************************************************************************************************
 * Definitions
 **********************************************************************************************************************************************************************/

#define NETWORK_KEY_INDEX 0

extern sl_cli_handle_t sl_cli_example_handle;

// The global backup of the trust center restoration data.
static TcRestoreData_t restoreData;

// Set the necessary security bitmasks for restoration assuming that there
// is a network key and a global key.
static uint16_t TCSO_INITIAL_SECURITY_BITMASK =
  (SL_ZIGBEE_NO_FRAME_COUNTER_RESET
   |
   SL_ZIGBEE_REQUIRE_ENCRYPTED_KEY
   | SL_ZIGBEE_HAVE_NETWORK_KEY
   |
   SL_ZIGBEE_HAVE_PRECONFIGURED_KEY
   |
   SL_ZIGBEE_TRUST_CENTER_USES_HASHED_LINK_KEY);

/**
 * @brief Exports a key from the Zigbee Security Manager.
 * @param[in] key_type Type of the key to be exported
 * @param[out] p_exported_key Pointer to the key to be exported.
 * @note Refer to AN1412 for more information of ZigBee Security Manager:
 * https://www.silabs.com/documents/public/application-notes/an1412-zigbee-security-manager.pdf
 */
static sl_status_t exportTcKey(sl_zigbee_key_struct_t *p_key_struct,
                               sl_zigbee_key_type_t key_type)
{
  sl_status_t status = SL_STATUS_OK;
  sl_zigbee_sec_man_key_t key;
  // Declare the context struct.
  sl_zigbee_sec_man_context_t context;
  // Initialize the context.
  sl_zigbee_sec_man_init_context(&context);
  // Set the key type.
  p_key_struct->type = key_type;
  context.core_key_type = p_key_struct->type;
  // Set the EUI64 associated with this key.
  sl_802154_long_addr_t eui;
  sl_zigbee_af_get_eui64(eui);
  memmove(context.eui64, eui, sizeof(sl_802154_long_addr_t));
  // Mark this context as having its EUI64 field set (so the export operation is aware that it should
  // search for this key by its EUI64 value). This is necessary so an unset key index value doesn't get
  // treated as a reference to the key at index 0, for example.
  context.flags |= ZB_SEC_MAN_FLAG_EUI_IS_VALID;
  // Export the key.
  status = sl_zigbee_sec_man_export_key(&context, &key);
  if (status != SL_STATUS_OK) {
    sl_zigbee_app_debug_println("Failed to export key. Error: 0x%x", status);
    return status; // No key to export so don't even try to get the key information.
  }
  memcpy(p_key_struct->key.contents, key.key, SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
  switch (p_key_struct->type)
  {
    case SL_ZB_SEC_MAN_KEY_TYPE_NETWORK:
    {
      sl_zigbee_sec_man_network_key_info_t network_key_info;
      status = sl_zigbee_sec_man_get_network_key_info(&network_key_info);
      if (status != SL_STATUS_OK) {
        sl_zigbee_app_debug_println(
          "Failed to get network key information. Error: 0x%x",
          status);
        return status; // Can't fill in the key information if we get an error, can we?
      }
      p_key_struct->sequenceNumber =
        network_key_info.network_key_sequence_number;
      p_key_struct->outgoingFrameCounter =
        network_key_info.network_key_frame_counter;
      break;
    }
    default:
    {
      // Get the key information.
      sl_zigbee_sec_man_aps_key_metadata_t key_info;
      status = sl_zigbee_sec_man_get_aps_key_info(&context, &key_info);
      if (status != SL_STATUS_OK) {
        sl_zigbee_app_debug_println(
          "Failed to get APS key information. Error: 0x%x",
          status);
        return status; // Can't fill in the key information if we get an error, can we?
      }
      // Set the key information in the EmberKeyStruct.
      p_key_struct->bitmask = key_info.bitmask;
      p_key_struct->outgoingFrameCounter = key_info.outgoing_frame_counter;
      p_key_struct->incomingFrameCounter = key_info.incoming_frame_counter;
      break;
    }
  }
  return status;
}

void restoreTcFromBackup(sl_cli_command_arg_t *arguments)
{
  sl_zigbee_initial_security_state_t state;
  sl_zigbee_network_parameters_t params;
  sl_status_t status;
  (void)arguments;
  // If a network already exists, TC replacement does not make sense.
  if (sl_zigbee_af_network_state() != SL_ZIGBEE_NO_NETWORK) {
    sl_zigbee_app_debug_println("Cannot import TC data while network is up.");
    return;
  }

  // Copy security information into initialization format.
  sl_zigbee_app_debug_println("Setting security information.");
  memcpy(sl_zigbee_key_contents(&(state.networkKey)),
         sl_zigbee_key_contents(&(restoreData.networkKey)),
         SL_ZIGBEE_ENCRYPTION_KEY_SIZE);
  state.networkKeySequenceNumber = restoreData.networkKeySeqNum;
  memcpy(sl_zigbee_key_contents(&(state.preconfiguredKey)),
         sl_zigbee_key_contents(&(restoreData.globalKey)),
         SL_ZIGBEE_ENCRYPTION_KEY_SIZE);

  // The initial security bitmask is a combination of existing configuration
  // parameters, hopefully based on the loaded data.
  // Because the parameters are manually loaded, there's no need to set the
  // tokens and the bitmask can be configured directly from the header file.
  state.bitmask = TCSO_INITIAL_SECURITY_BITMASK;

  // Set node information and network parameters.
  sl_zigbee_app_debug_println(
    "Setting node information and network parameters.");
  memcpy(params.extendedPanId, restoreData.ePanId,
         sizeof(params.extendedPanId));
  params.panId = restoreData.panId;
  params.radioTxPower = restoreData.radioPower;
  params.radioChannel = restoreData.radioChannel;
  params.joinMethod = SL_ZIGBEE_USE_MAC_ASSOCIATION;

  // Initialize the network security state.
  sl_zigbee_app_debug_println("Starting network security state.");
  status = sl_zigbee_set_initial_security_state(&state);
  if (status == SL_STATUS_OK) {
    sl_zigbee_app_debug_println(
      "Successfully initialized network security state.");
  } else {
    sl_zigbee_app_debug_println(
      "Failed to initialize security state. Error: 0x%04X",
      status);
  }

  // Set network frame counter
  status = sl_token_manager_set_data(TOKEN_STACK_NONCE_COUNTER,
                                     &restoreData.networkFC,
                                     sizeof(int32u));
  if (status != SL_STATUS_OK) {
    sl_zigbee_app_debug_println(
      "Failed to write network frame counter. Error: 0x%04X",
      status);
  }
  // Set global frame counter
  status = sl_token_manager_set_data(TOKEN_STACK_APS_FRAME_COUNTER,
                                     &restoreData.globalFC,
                                     sizeof(int32u));
  if (status != SL_STATUS_OK) {
    sl_zigbee_app_debug_println(
      "Failed to write global frame counter. Error: 0x%04X",
      status);
  }

  // Now that the parameters have been configured, kick off the network.
  status = sl_zigbee_af_form_network(&params);
  if (status == SL_STATUS_OK) {
    sl_zigbee_app_debug_println(
      "Successfully formed network with given params.");
  } else {
    sl_zigbee_app_debug_println("Failed to form network. Error: 0x%x", status);
  }

  // Reboot so that the tokens can be reset appropriately.
  halReboot();
}

void getTcBackupInfo(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  getTcNodeInfo();
  getTcSecurityInfo();
}

void getTcNodeInfo(void)
{
  sl_zigbee_app_debug_println("Trust Center Node Information: ");
  // Node information.
  sl_zigbee_app_debug_print("Node ID: %u\n",
                            (unsigned int)SL_ZIGBEE_TRUST_CENTER_NODE_ID);
  sl_status_t getStatus;
  sl_zigbee_node_type_t nodeType;
  sl_zigbee_network_parameters_t params;
  getStatus = sl_zigbee_af_get_network_parameters(&nodeType, &params);
  if (getStatus == SL_STATUS_OK) {
    sl_zigbee_app_debug_println("Pan ID: 0x%04x", params.panId);
    sl_zigbee_app_debug_println("Radio Channel: %d", params.radioChannel);
    sl_zigbee_app_debug_println("Radio Power: %d", params.radioTxPower);
    sl_zigbee_app_debug_println(
      "Extended Pan ID: %02X%02X%02X%02X%02X%02X%02X%02X",
      params.extendedPanId[7],
      params.extendedPanId[6],
      params.extendedPanId[5],
      params.extendedPanId[4],
      params.extendedPanId[3],
      params.extendedPanId[2],
      params.extendedPanId[1],
      params.extendedPanId[0]);
  } else {
    sl_zigbee_app_debug_println(
      "Pan ID, ePan Id, radio channel/power not found. \
											          Error : 0x%x",
      getStatus);
  }

  // Device EUID information
  sl_zigbee_app_debug_println("***** Getting EUID information *****");
  // Get EUI64 used by ZigBee stack
  sl_802154_long_addr_t eui64_value;
  sl_zigbee_af_get_eui64(eui64_value);
  sl_zigbee_app_debug_print("Stack EUID : ");
  sl_zigbee_af_print_big_endian_eui64(eui64_value);
  sl_zigbee_app_debug_println(" ");
  // Get MFG_EMBER_EUI_64
  sl_get_mfg_token_ember_eui64(&eui64_value);
  sl_zigbee_app_debug_print("   MFG EUI64 Token :      ");
  sl_zigbee_af_print_big_endian_eui64(eui64_value);
  sl_zigbee_app_debug_println("");
  // Get MFG_CUSTOM_EUI_64
  sl_get_mfg_token_custom_eui64(&eui64_value);
  sl_zigbee_app_debug_print("   Custom EUI64 Token :   ");
  sl_zigbee_af_print_big_endian_eui64(eui64_value);
  sl_zigbee_app_debug_println("");
  // Get STACK_RESTORED_EUI64
  uint8_t restored_eui64[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  if (sl_token_manager_get_data(NVM3KEY_STACK_RESTORED_EUI64,
                                restored_eui64,
                                sizeof(sl_802154_long_addr_t))
      == SL_STATUS_OK) {
    sl_zigbee_app_debug_print("   Restored EUI64 Token : ");
    sl_zigbee_af_print_big_endian_eui64(restored_eui64);
    sl_zigbee_app_debug_println("");
  } else {
    sl_zigbee_app_debug_println("Failed to get Restored EUI64");
  }
  sl_zigbee_app_debug_println("************************************");
}

void getTcSecurityInfo(void)
{
  // Key information about security.
  sl_zigbee_key_struct_t keyStruct;
  if (exportTcKey(&keyStruct, SL_ZB_SEC_MAN_KEY_TYPE_TC_LINK) != SL_STATUS_OK) {
    sl_zigbee_app_debug_println("Failed to retrieve TC Link Key information");
    return;
  }
  sl_zigbee_app_debug_print("Global Key: ");
  sl_zigbee_af_print_zigbee_key(sl_zigbee_key_contents(&(keyStruct.key)));
  sl_zigbee_app_debug_println("Global Key FC : 0x%2X seqNum : 0x%x",
                              keyStruct.outgoingFrameCounter,
                              keyStruct.sequenceNumber);

  if (exportTcKey(&keyStruct, SL_ZB_SEC_MAN_KEY_TYPE_NETWORK) != SL_STATUS_OK) {
    sl_zigbee_app_debug_println("Failed to retrieve Network Key information");
    return;
  }
  sl_zigbee_app_debug_print("Network Key: ");
  sl_zigbee_af_print_zigbee_key(sl_zigbee_key_contents(&(keyStruct.key)));
  sl_zigbee_app_debug_println("Network Key FC : 0x%2X seqNum : 0x%x",
                              keyStruct.outgoingFrameCounter,
                              keyStruct.sequenceNumber);
}

void saveTcPanId(sl_cli_command_arg_t *arguments)
{
  restoreData.panId = sl_cli_get_argument_uint16(arguments, 0);
  sl_zigbee_app_debug_println("Successfully stored Pan ID as 0x%04X",
                              restoreData.panId);
}

void saveTcRadioPower(sl_cli_command_arg_t *arguments)
{
  restoreData.radioPower = sl_cli_get_argument_int8(arguments, 0);
  sl_zigbee_app_debug_println("Successfully stored radio power as %d",
                              restoreData.radioPower);
}

void saveTcRadioChannel(sl_cli_command_arg_t *arguments)
{
  restoreData.radioChannel = sl_cli_get_argument_uint8(arguments, 0);
  sl_zigbee_app_debug_println("Successfully stored radio channel as %d",
                              restoreData.radioChannel);
}

void saveTcEPanId(sl_cli_command_arg_t *arguments)
{
  sl_zigbee_copy_eui64_arg(arguments, 0, restoreData.ePanId, true);
  sl_zigbee_app_debug_print("Successfully stored extended Pan ID as: ");
  sl_zigbee_af_print_big_endian_eui64(restoreData.ePanId);
  sl_zigbee_app_debug_println(" ");
}

void saveTcEUI64(sl_cli_command_arg_t *arguments)
{
  sl_zigbee_copy_eui64_arg(arguments, 0, restoreData.eui64, true);
  sl_zigbee_app_debug_print("Successfully stored eui64 as: ");
  sl_zigbee_af_print_big_endian_eui64(restoreData.eui64);
  sl_zigbee_app_debug_println(" ");

  // Set the custom EUI64 to match that of the original trust center.
  // Note that the custom EUI64 can only be set once.
  // Make sure that the manufacturing tokens 0xFFFFFFFFFFFFFFFF.
  // If not, use commander.exe to erase it.
  sl_set_mfg_token_custom_eui64_data(&restoreData.eui64);
  halReboot();
}

void saveTcGlobalKey(sl_cli_command_arg_t *arguments)
{
  size_t read_size;
  uint8_t *key_contents;
  key_contents = sl_cli_get_argument_hex(arguments, 0, &read_size);
  memcpy(restoreData.globalKey.contents, key_contents, read_size);
  sl_zigbee_app_debug_println("Stored global key in backup object as: ");
  sl_zigbee_af_print_zigbee_key(sl_zigbee_key_contents(
                                  &(restoreData.globalKey)));
}

void saveTcNetworkKey(sl_cli_command_arg_t *arguments)
{
  size_t read_size;
  uint8_t *key_contents;
  key_contents = sl_cli_get_argument_hex(arguments, 0, &read_size);
  memcpy(restoreData.networkKey.contents, key_contents, read_size);
  sl_zigbee_app_debug_println("Stored network key in backup object as: ");
  sl_zigbee_af_print_zigbee_key(sl_zigbee_key_contents(
                                  &(restoreData.networkKey)));
}

void saveTcGlobalFC(sl_cli_command_arg_t *arguments)
{
  restoreData.globalFC = sl_cli_get_argument_uint32(arguments, 0);
  sl_zigbee_app_debug_println(
    "Stored global frame counter in backup object as \
										          0x%2X.",
    restoreData.globalFC);
}

void saveTcNetworkFC(sl_cli_command_arg_t *arguments)
{
  restoreData.networkFC = sl_cli_get_argument_uint32(arguments, 0);
  sl_zigbee_app_debug_println(
    "Stored network frame counter in backup object as \
										          0x%2X.",
    restoreData.networkFC);
}

void saveTcNetworkKeySeqNum(sl_cli_command_arg_t *arguments)
{
  restoreData.networkKeySeqNum = sl_cli_get_argument_uint8(arguments, 0);
  sl_zigbee_app_debug_println(
    "Stored network sequence number in backup object.");
}

void saveTcRestoredEUI64(sl_cli_command_arg_t *arguments)
{
  sl_802154_long_addr_t eui64;
  sl_zigbee_copy_eui64_arg(arguments, 0, eui64, true);

  // Set the Restored EUI64 to match that of the original trust center.
  // This token takes the higher priority than MFG_EMBER_EUI_64 and MFG_CUSTOM_EUI_64.
  // It is re-writable.
  if (sl_token_manager_set_data(NVM3KEY_STACK_RESTORED_EUI64,
                                eui64,
                                sizeof(sl_802154_long_addr_t))
      != SL_STATUS_OK) {
    sl_zigbee_app_debug_println("Failed to write Restored EUI64");
    return;
  }
  sl_zigbee_app_debug_print("Successfully stored eui64 as: ");
  sl_zigbee_af_print_big_endian_eui64(eui64);
  sl_zigbee_app_debug_println(" ");
  halReboot();
}

void getTcBackupStoredFields(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  sl_zigbee_app_debug_println("Pan ID : 0x%2x", restoreData.panId);
  sl_zigbee_app_debug_println("Radio Channel : %d", restoreData.radioChannel);
  sl_zigbee_app_debug_println("Radio Power : %d", restoreData.radioPower);
  sl_zigbee_app_debug_println(
    "Extended Pan Id: %02X%02X%02X%02X%02X%02X%02X%02X",
    restoreData.ePanId[7],
    restoreData.ePanId[6],
    restoreData.ePanId[5],
    restoreData.ePanId[4],
    restoreData.ePanId[3],
    restoreData.ePanId[2],
    restoreData.ePanId[1],
    restoreData.ePanId[0]);
  sl_zigbee_app_debug_print("Global Key: ");
  sl_zigbee_af_print_zigbee_key(sl_zigbee_key_contents(
                                  &(restoreData.globalKey)));
  sl_zigbee_app_debug_println("Global FC : 0x%2X", restoreData.globalFC);
  sl_zigbee_app_debug_print("Network Key: ");
  sl_zigbee_af_print_zigbee_key(sl_zigbee_key_contents(
                                  &(restoreData.networkKey)));
  sl_zigbee_app_debug_println("Network FC : 0x%2X", restoreData.networkFC);
  sl_zigbee_app_debug_println("Network Sequence Number : 0x%2X",
                              restoreData.networkKeySeqNum);
}

// This function is called by the Stack; not the application.
// It is originally implemented in /protocol/zigbee/app/framework/common/zigbee_token_interface.c
// Due to an issue, adding zigbee_token_interface.c into the project will lead to compilation errors
// The workaround is to implement this function in user-application
// Strong implementation to restore the EUI64 incase backup/restore
void sl_zigbee_get_restored_eui64(sl_802154_long_addr_t eui64)
{
  uint8_t blank[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  uint8_t restoredEui64[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  sl_status_t status = sl_token_manager_get_data(NVM3KEY_STACK_RESTORED_EUI64,
                                                 restoredEui64,
                                                 sizeof(sl_802154_long_addr_t));
  if (status == SL_STATUS_OK) {
    if (memcmp(blank,
               restoredEui64,
               sizeof(sl_802154_long_addr_t)) == 0) {
      // There is no restored EUI64, no action
    } else {
      // There is a restored EUI available, so use that.
      memcpy(eui64, restoredEui64, sizeof(sl_802154_long_addr_t));
    }
  }
}

bool trust_center_swap_out_cli_register()
{
  return sl_cli_command_add_command_group(sl_cli_example_handle,
                                          &customTrustCenterSwapOutCommandsGroup);
}
