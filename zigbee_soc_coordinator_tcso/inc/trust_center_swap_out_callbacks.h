/***************************************************************************//**
 * @file
 * @brief trust_center_swap_callbacks.h
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

#ifndef TRUST_CENTER_SWAP_OUT_CALLBACKS_H
#define TRUST_CENTER_SWAP_OUT_CALLBACKS_H

#include "af.h"

/// Maintain all necessary TC data in a struct for straightforward access.
typedef struct {
  int16u nodeId;                    /// The ID of the trust center node.
  int16u panId;                     /// 16-bit PAN ID for the network.
  int8s radioPower;                 /// The network's operating radio power.
  int8u radioChannel;               /// The network's operating radio channel.
  int8u ePanId[8];                  /// 64-bit extended PAN ID for the network.
  sl_802154_long_addr_t eui64;      /// EUI64 manufacturing ID for the TC device.
  sl_zigbee_key_data_t globalKey;   /// Global key: value stored in globalKey.contents.
  int32u globalFC;                  /// The frame counter for the global key.
  sl_zigbee_key_data_t networkKey;  /// Network key: value stored in networkKey.contents.
  int32u networkFC;                 /// The frame counter for the network key.
  int8u networkKeySeqNum;           /// The sequence number for the network key.
} TcRestoreData_t;

/**************************************************************************//**
 * Prints all of the backup parameters associated with the trust center of the
 * current, live network to the serial command line interface.
 *
 * The network must be actively running or it will print an error message.
 * Information printed includes node information (node ID, PAN ID,
 * extended PAN ID, radio channel, radio power, and EUI64) and security
 * information (network key, global key, network key sequence number,
 * network frame counter, andl global frame counter).
 ******************************************************************************/
void getTcBackupInfo(sl_cli_command_arg_t *arguments);

/**************************************************************************//**
 * Prints network parameters and defining values of the trust center node to the
 * serial command line interface.
 *
 * The network must be actively running or it will print an error. This command
 * reports node ID, PAN ID, extended PAN ID, radio channel, radio power, and
 * the EUI64 of the trust center device.
 ******************************************************************************/
void getTcNodeInfo(void);

/**************************************************************************//**
 * Prints security information  of the trust center and network to the
 * serial command line interface.
 *
 * The network must be actively running or it will print an error. This command
 * reports the global key, the network key, the global frame counter, the network
 * frame counter, and the network key sequence number.
 ******************************************************************************/
void getTcSecurityInfo(void);

/******************************************************************************
 * Prints each of the saved parameters associated with the global backup struct.
 *
 * The global restoreData struct initializes with all fields empty. To restore
 * the network, the user must leverage the included CLI methods or
 * provide different application-specific functionality. At minimum, they must
 * specify the global or network keys for the network to properly initalize.
 *
 * Each of the save commands prints the stored value to the console, but this
 * method prints all of the currently saved values for the node ID, PAN ID,
 * extended PAN ID, radio channel, radio power, eui64, global key, network key,
 * global frame counter, network frame counter, and network key sequence
 * number to the console directly.
 ******************************************************************************/
void getTcBackupStoredFields(sl_cli_command_arg_t *arguments);

/**************************************************************************//**
 * Restores the trust center and network directly from the backup struct (the
 * restoreData global variable).
 *
 * To do so, it populates an initial security state object and a Zigbee
 * network parameters object with the content in the fields of the restoreData
 * object (which can be populated manually in DEBUG mode or through loading data
 * from a text file), sets the EUI64 of the node by editing the manufacturing
 * token (note that the custom EUI64 cannot have already been set, but erasing the
 * @userdata page will handle this), and then initializes the security state and
 * the network itself.
 ******************************************************************************/
void restoreTcFromBackup(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the network PAN ID to the backup struct.
 *
 * Argument for the PAN ID should be a 16-bit unsigned integer value that
 * can be specified with or without the characters "0x" preceding the value.
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcPanId(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the network radio power to the backup
 * struct.
 *
 * Argument for the radio power should be an 8-bit signed integer.
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcRadioPower(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the trust center node ID to the backup
 * struct.
 *
 * Argument for the radio channel should be an 8-bit unsigned integer.
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcRadioChannel(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the network extended PAN ID to the backup
 * struct.
 *
 * Argument for the extended PAN ID should be an 8-byte integer passed as hex
 * string. So, an Extended PAN ID of 1122334455667788 should be specified as
 * the argument {11 22 33 44 55 66 77 88}.
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcEPanId(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the trust center node's eui64 to the
 * backup struct.
 *
 * Argument for the eui64 should be an 8-byte integer passed as hex
 * string. So, an eui64 value of 1122334455667788 should be specified as
 * the argument {11 22 33 44 55 66 77 88}.
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcEUI64(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the trust center's global link key
 * to the backup struct.
 *
 * Argument for the global link key should be a 16-byte integer passed as a hex
 * string. So, a global link key value of 1122334455667788 11223334455667788
 * should be specified as the argument
 * {11 22 33 44 55 66 77 88 11 22 33 44 55 66 77 88}.
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcGlobalKey(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the global frame counter to the backup
 * struct.
 *
 * The argument for this function  should be a 32-bit unsigned integer that
 * can include, but does not require the inclusion of, a preceding "0x".
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcGlobalFC(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the network key to the backup struct.
 *
 * Argument for the network key should be a 16-byte integer passed as a hex
 * string. So, a global link key value of 1122334455667788 11223334455667788
 * should be specified as the argument
 *
 * {11 22 33 44 55 66 77 88 11 22 33 44 55 66 77 88}.
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcNetworkKey(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the network frame counter to the backup
 * struct.
 *
 * The argument for this function should be a 32-bit unsigned integer that
 * can include, but does not require the inclusion of, a preceding "0x".
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcNetworkFC(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the network key sequence number to the
 * backup struct.
 *
 * The argument for the network frame counter should be a 8-bit unsigned integer.
 *
 * This method will print the saved value to the CLI. To confirm that this
 * value, and any other saved backup values, are stored as intended, leverage
 * the getTcBackupStoredFields() function with the serial command
 * "custom tc-swap-out print backup" to print all saved backup parameters.
 ******************************************************************************/
void saveTcNetworkKeySeqNum(sl_cli_command_arg_t *arguments);

/******************************************************************************
 * Saves the user-specified value of the trust center node's eui64 to the
 * token NVM3KEY_STACK_RESTORED_EUI64
 *
 * Argument for the eui64 should be an 8-byte integer passed as hex
 * string. So, an eui64 value of 1122334455667788 should be specified as
 * the argument {11 22 33 44 55 66 77 88}.
 *
 * This token can be rewritten
 ******************************************************************************/
void saveTcRestoredEUI64(sl_cli_command_arg_t *arguments);

/**
 * @brief Initialises the CLI groups and command tables for Trust Center swap out
 */
bool trust_center_swap_out_cli_register(void);

#endif // #ifndef TRUST_CENTER_SWAP_OUT_CALLBACKS_H
