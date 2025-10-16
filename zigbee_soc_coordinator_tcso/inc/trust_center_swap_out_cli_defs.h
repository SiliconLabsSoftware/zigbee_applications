/***************************************************************************//**
 * @file
 * @brief trust_center_swap_out_cli_defs.h
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
 *******************************************************************************
 * Special thanks to Muhammad.Arif@aztech.com
 ******************************************************************************/

#ifndef TRUST_CENTER_SWAP_OUT_CALLBACKS_CLI_DEFS_H__
#define TRUST_CENTER_SWAP_OUT_CALLBACKS_CLI_DEFS_H__
#include "trust_center_swap_out_callbacks.h"

/**********************************************************************************************************************************************************************
 * Forward Declarations
 **********************************************************************************************************************************************************************/
static const sl_cli_command_entry_t customTrustCenterSwapOutCommandsTable[];
static const sl_cli_command_info_t group_top_submenu;
static const sl_cli_command_info_t group_print_submenu;
static const sl_cli_command_info_t group_save_submenu;
static const sl_cli_command_info_t group_restore_submenu;
static const sl_cli_command_info_t cmd_tc_print;
static const sl_cli_command_info_t cmd_backup_stored_fields;
static const sl_cli_command_info_t cmd_tc_save_pan_id;
static const sl_cli_command_info_t cmd_tc_save_node_id;
static const sl_cli_command_info_t cmd_tc_save_radio_power;
static const sl_cli_command_info_t cmd_tc_save_radio_channel;
static const sl_cli_command_info_t cmd_tc_save_epan_id;
static const sl_cli_command_info_t cmd_tc_save_eui64;
static const sl_cli_command_info_t cmd_tc_save_global_key;
static const sl_cli_command_info_t cmd_tc_save_network_key;
static const sl_cli_command_info_t cmd_tc_save_global_fc;
static const sl_cli_command_info_t cmd_tc_save_network_fc;
static const sl_cli_command_info_t cmd_tc_save_network_key_seq_num;
static const sl_cli_command_info_t cmd_tc_save_restored_eui64;
static const sl_cli_command_info_t cmd_tc_restore;

/**********************************************************************************************************************************************************************
 * Command Group
 **********************************************************************************************************************************************************************/
static sl_cli_command_group_t customTrustCenterSwapOutCommandsGroup = {
  NULL,
  false,
  customTrustCenterSwapOutCommandsTable
};

/**********************************************************************************************************************************************************************
 * Command Entry Tables
 **********************************************************************************************************************************************************************/
static const sl_cli_command_entry_t customTrustCenterSwapOutCommandsTable[] = {
  { "tcso", &group_top_submenu, false },
  { NULL, NULL, false }
};
static const sl_cli_command_entry_t customTrustCenterSwapOutSubmenuTable[] = {
  { "print", &group_print_submenu, false },
  { "save", &group_save_submenu, false },
  { "restore", &group_restore_submenu, false },
  { NULL, NULL, false }
};
static const sl_cli_command_entry_t customTrustCenterSwapOutPrintTable[] = {
  { "all", &cmd_tc_print, false },
  { "backup", &cmd_backup_stored_fields, false },
  { NULL, NULL, false }
};
static const sl_cli_command_entry_t customTrustCenterSwapOutSaveTable[] = {
  { "pan-id", &cmd_tc_save_pan_id, false },
  { "radio-power", &cmd_tc_save_radio_power, false },
  { "radio-channel", &cmd_tc_save_radio_channel, false },
  { "extended-pan-id", &cmd_tc_save_epan_id, false },
  { "eui64", &cmd_tc_save_eui64, false },
  { "global-key", &cmd_tc_save_global_key, false },
  { "network-key", &cmd_tc_save_network_key, false },
  { "global-frame-counter", &cmd_tc_save_global_fc, false },
  { "network-frame-counter", &cmd_tc_save_network_fc, false },
  { "network-key-sequence-number", &cmd_tc_save_network_key_seq_num, false },
  { "restored-eui64", &cmd_tc_save_restored_eui64, false },
  { NULL, NULL, false }
};
static const sl_cli_command_entry_t customTrustCenterSwapOutRestoreTable[] = {
  { "all", &cmd_tc_restore, false },
  { NULL, NULL, false }
};

/**********************************************************************************************************************************************************************
 * Command Info Types
 **********************************************************************************************************************************************************************/
static const sl_cli_command_info_t group_top_submenu = SL_CLI_COMMAND_GROUP(
  customTrustCenterSwapOutSubmenuTable,
  "Commands to facilitate TC backup and restoration");
static const sl_cli_command_info_t group_print_submenu = SL_CLI_COMMAND_GROUP(
  customTrustCenterSwapOutPrintTable,
  "Print parameters associated with current network and live trust center.");
static const sl_cli_command_info_t group_save_submenu = SL_CLI_COMMAND_GROUP(
  customTrustCenterSwapOutSaveTable,
  "Save parameters associated with current network and live trust center.");
static const sl_cli_command_info_t group_restore_submenu = SL_CLI_COMMAND_GROUP(
  customTrustCenterSwapOutRestoreTable,
  "Restore parameters associated with current network and live trust center.");

static const sl_cli_command_info_t cmd_tc_print = SL_CLI_COMMAND(
  getTcBackupInfo,
  "Prints current network and security information about the live Trust Center.",
  "",
  { SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_backup_stored_fields = SL_CLI_COMMAND(
  getTcBackupStoredFields,
  "Print the fields associated with the backup object.",
  "",
  { SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_pan_id = SL_CLI_COMMAND(
  saveTcPanId,
  "Set the PAN ID of the restore data.",
  "pan-id",
  { SL_CLI_ARG_UINT16, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_radio_power = SL_CLI_COMMAND(
  saveTcRadioPower,
  "Set the radio power of the restore data.",
  "radio-power",
  { SL_CLI_ARG_INT8, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_radio_channel = SL_CLI_COMMAND(
  saveTcRadioChannel,
  "Set the radio channel of the restore data.",
  "radio-channel",
  { SL_CLI_ARG_UINT8, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_epan_id = SL_CLI_COMMAND(
  saveTcEPanId,
  "Set the extended PAN ID of the restore data.",
  "extended-pan-id",
  { SL_CLI_ARG_HEX, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_eui64 = SL_CLI_COMMAND(
  saveTcEUI64,
  "Set the EUI64 of the restore data.",
  "eui64",
  { SL_CLI_ARG_HEX, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_global_key = SL_CLI_COMMAND(
  saveTcGlobalKey,
  "Set the global key of the restore data.",
  "global-key",
  { SL_CLI_ARG_HEX, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_network_key = SL_CLI_COMMAND(
  saveTcNetworkKey,
  "Set the network key of the restore data.",
  "network-key",
  { SL_CLI_ARG_HEX, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_global_fc = SL_CLI_COMMAND(
  saveTcGlobalFC,
  "Set the global frame counter of the restore data.",
  "global-frame-counter",
  { SL_CLI_ARG_UINT32, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_network_fc = SL_CLI_COMMAND(
  saveTcNetworkFC,
  "Set the network frame counter of the restore data.",
  "network-frame-counter",
  { SL_CLI_ARG_UINT32, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_network_key_seq_num =
  SL_CLI_COMMAND(
    saveTcNetworkKeySeqNum,
    "Set the network key sequence number of the restore data.",
    "network-key-sequence-number",
    { SL_CLI_ARG_UINT8, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_save_restored_eui64 = SL_CLI_COMMAND(
  saveTcRestoredEUI64,
  "Write EUI64 in NVM3KEY_STACK_RESTORED_EUI64 which can be rewritten",
  "restored-eui64",
  { SL_CLI_ARG_HEX, SL_CLI_ARG_END });
static const sl_cli_command_info_t cmd_tc_restore = SL_CLI_COMMAND(
  restoreTcFromBackup,
  "Restore the network using the backup data.",
  NULL,
  { SL_CLI_ARG_END });

#pragma GCC diagnostic pop

#endif // TRUST_CENTER_SWAP_OUT_CALLBACKS_CLI_DEFS_H__
