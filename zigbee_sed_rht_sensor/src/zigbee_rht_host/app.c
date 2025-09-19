/***************************************************************************//**
 * @file app.c
 * @brief Zigbee RHT Sensor Host Example
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
#include <stdlib.h>

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/util/util.h"
#include "app/util/zigbee-framework/zigbee-device-common.h"

#include "stack/include/trust-center.h"
#include "zap-cluster-command-parser.h"

#ifdef SL_CATALOG_CLI_PRESENT
#include "stack/include/zigbee-security-manager.h"
#endif

// The number of tokens that can be written using ezspSetToken and read using
// ezspGetToken.
#define MFGSAMP_NUM_EZSP_TOKENS        8
// The size of the tokens that can be written using ezspSetToken and read using
// ezspGetToken.
#define MFGSAMP_EZSP_TOKEN_SIZE        8
// The number of manufacturing tokens.
#define MFGSAMP_NUM_EZSP_MFG_TOKENS    11
// The size of the largest EZSP Mfg token, EZSP_MFG_CBKE_DATA please refer to
// app/util/ezsp/ezsp-enum.h.
#define MFGSAMP_EZSP_TOKEN_MFG_MAXSIZE 92

#define APP_NEXT_NETWORK_KEY_INDEX     1

extern sl_status_t sl_zigbee_broadcast_next_network_key(
  sl_zigbee_key_data_t *key);

// ----------------------
// ZCL commands handling

static void ias_ace_cluster_arm_command_handler(uint8_t armMode,
                                                uint8_t *armDisarmCode,
                                                uint8_t zoneId)
{
  uint16_t armDisarmCode_length = sl_zigbee_af_string_length(armDisarmCode);
  uint16_t sender = sli_zigbee_af_get_sender();
  uint16_t i;

  sl_zigbee_app_debug_print("IAS ACE Arm Received %04X", armMode);

  // Start i at 1 to skip over leading character in the byte array as it is the
  // length byte
  for (i = 1; i < armDisarmCode_length; i++) {
    sl_zigbee_app_debug_print("%c", armDisarmCode[i]);
  }
  sl_zigbee_app_debug_print(" %02X\n", zoneId);

  sl_zigbee_af_fill_command_ias_ace_cluster_arm_response(armMode);
  sl_zigbee_af_send_command_unicast(SL_ZIGBEE_OUTGOING_DIRECT, sender);
}

static void ias_ace_cluster_bypass_command_handler(uint8_t numberOfZones,
                                                   uint8_t *zoneIds,
                                                   uint8_t *armDisarmCode)
{
  uint16_t sender = sli_zigbee_af_get_sender();
  uint8_t i;

  sl_zigbee_app_debug_print("IAS ACE Cluster Bypass for zones ");

  for (i = 0; i < numberOfZones; i++) {
    sl_zigbee_app_debug_print("%d ", zoneIds[i]);
  }
  sl_zigbee_app_debug_print("\n");

  sl_zigbee_af_fill_command_ias_ace_cluster_bypass_response(numberOfZones,
                                                            zoneIds,
                                                            numberOfZones);
  sl_zigbee_af_send_command_unicast(SL_ZIGBEE_OUTGOING_DIRECT, sender);
}

static uint32_t zcl_ias_ace_cluster_server_command_handler(
  sl_service_opcode_t opcode,
  sl_service_function_context_t *context)
{
  (void)opcode;

  sl_zigbee_af_cluster_command_t *cmd =
    (sl_zigbee_af_cluster_command_t *)context->data;

  switch (cmd->commandId) {
    case ZCL_ARM_COMMAND_ID:
    {
      sl_zcl_ias_ace_cluster_arm_command_t cmd_data;

      if (zcl_decode_ias_ace_cluster_arm_command(cmd, &cmd_data)
          != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
        return SL_ZIGBEE_ZCL_STATUS_UNSUP_COMMAND;
      }

      ias_ace_cluster_arm_command_handler(cmd_data.armMode,
                                          cmd_data.armDisarmCode,
                                          cmd_data.zoneId);
      return SL_ZIGBEE_ZCL_STATUS_SUCCESS;
    }
    case ZCL_BYPASS_COMMAND_ID:
    {
      sl_zcl_ias_ace_cluster_bypass_command_t cmd_data;

      if (zcl_decode_ias_ace_cluster_bypass_command(cmd, &cmd_data)
          != SL_ZIGBEE_ZCL_STATUS_SUCCESS) {
        return SL_ZIGBEE_ZCL_STATUS_UNSUP_COMMAND;
      }

      ias_ace_cluster_bypass_command_handler(cmd_data.numberOfZones,
                                             cmd_data.zoneIds,
                                             cmd_data.armDisarmCode);

      return SL_ZIGBEE_ZCL_STATUS_SUCCESS;
    }
  }

  return SL_ZIGBEE_ZCL_STATUS_UNSUP_COMMAND;
}

// ----------------------
// Implemented Callbacks

/** @brief Init
 * Application init function
 */
void sl_zigbee_af_main_init_cb(void)
{
  // Subscribe to ZCL commands for the IAS_ACE cluster, server side.
  sl_zigbee_subscribe_to_zcl_commands(ZCL_IAS_ACE_CLUSTER_ID,
                                      0xFFFF,
                                      ZCL_DIRECTION_CLIENT_TO_SERVER,
                                      zcl_ias_ace_cluster_server_command_handler);
}

/** @brief Report Attributes Callback
 *
 * This function is called by the application framework when a Report Attributes
 * command is received from an external device.  The application should return
 * true if the message was processed or false if it was not.
 * We only process the Relative humidity and temperature measurement cluster
 * attribute changes
 *
 * @param cluster_id The cluster identifier of this command.  Ver.: always
 * @param buffer Buffer containing the list of attribute report records.  Ver.:
 * always
 * @param buf_len The length in bytes of the list.  Ver.: always
 */
bool sl_zigbee_af_report_attributes_cb(sl_zigbee_af_cluster_id_t cluster_id,
                                       int8u *buffer,
                                       int16u buf_len)
{
  int16_t attribute;

  // Custom processing done only for cluster 0x405 and 0x402 (RH and Temperature
  // measurement clusters)
  if ((cluster_id == ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID)
      || (cluster_id == ZCL_TEMP_MEASUREMENT_CLUSTER_ID)) {
    attribute = (buffer[buf_len - 1] << 8) | (buffer[buf_len - 2]);

    if (cluster_id == ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID) {
      sl_zigbee_core_debug_println("RH: %2d.%2d %%",
                                   attribute / 100,
                                   (attribute % 100));
    } else {
      sl_zigbee_core_debug_println("Temperature: %2d.%2d C",
                                   attribute / 100,
                                   (attribute % 100));
    }
    return true;
  }

  return false;
}

#ifdef SL_CATALOG_CLI_PRESENT
// -------------------------------------
// Custom CLI commands and related code

// The manufacturing tokens are enumerated in app/util/ezsp/ezsp-protocol.h.
// The names are enumerated here to make it easier for the user.
static const char *ezsp_mfg_token_names[] =
{
  "EZSP_MFG_CUSTOM_VERSION...",
  "EZSP_MFG_STRING...........",
  "EZSP_MFG_BOARD_NAME.......",
  "EZSP_MFG_MANUF_ID.........",
  "EZSP_MFG_PHY_CONFIG.......",
  "EZSP_MFG_BOOTLOAD_AES_KEY.",
  "EZSP_MFG_ASH_CONFIG.......",
  "EZSP_MFG_EZSP_STORAGE.....",
  "EZSP_STACK_CAL_DATA.......",
  "EZSP_MFG_CBKE_DATA........",
  "EZSP_MFG_INSTALLATION_CODE"
};

// Called to dump all of the tokens. This dumps the indices, the names,
// and the values using ezspGetToken and ezspGetMfgToken. The indices
// are used for read and write functions below.
void mfg_app_token_dump(sl_cli_command_arg_t *arguments)
{
  (void)arguments;

  sl_status_t status;
  uint8_t token_data[MFGSAMP_EZSP_TOKEN_MFG_MAXSIZE];
  uint8_t index, i, tokenLength;

  // first go through the tokens accessed using ezspGetToken
  sl_zigbee_app_debug_println("(data shown little endian)");
  sl_zigbee_app_debug_println("Tokens:");
  sl_zigbee_app_debug_println("idx  value:");
  for (index = 0; index < MFGSAMP_NUM_EZSP_TOKENS; index++) {
    // get the token data here
    status = sl_zigbee_ezsp_get_token(index, token_data);
    sl_zigbee_app_debug_print("[%d]", index);
    if (status == SL_STATUS_OK) {
      // Print out the token data
      for (i = 0; i < MFGSAMP_EZSP_TOKEN_SIZE; i++) {
        sl_zigbee_app_debug_print(" %02X", token_data[i]);
      }
      sl_zigbee_app_debug_println("");
    } else {
      // handle when ezspGetToken returns an error
      sl_zigbee_app_debug_println(" ... error 0x%02X ...", status);
    }
  }

  // now go through the tokens accessed using ezspGetMfgToken
  // the manufacturing tokens are enumerated in app/util/ezsp/ezsp-protocol.h
  // this file contains an array (ezsp_mfg_token_names) representing the names.
  sl_zigbee_app_debug_println("Manufacturing Tokens:");
  sl_zigbee_app_debug_println("idx  token name                 len   value");
  for (index = 0; index < MFGSAMP_NUM_EZSP_MFG_TOKENS; index++) {
    // ezspGetMfgToken returns a length, be careful to only access
    // valid token indices.
    tokenLength = sl_zigbee_ezsp_get_mfg_token(index, token_data);
    sl_zigbee_app_debug_println("[%x] %s: 0x%x:",
                                index, ezsp_mfg_token_names[index],
                                tokenLength);

    // Print out the token data
    for (i = 0; i < tokenLength; i++) {
      if ((i != 0) && ((i % 8) == 0)) {
        sl_zigbee_app_debug_println("");
        sl_zigbee_app_debug_print("                                    :");
      }
      sl_zigbee_app_debug_print(" %02X", token_data[i]);
    }
    sl_zigbee_app_debug_println("");
  }
  sl_zigbee_app_debug_println("");
}

void change_nwk_key_command(sl_cli_command_arg_t *arguments)
{
  (void)arguments;

  sl_zigbee_key_data_t key;
  sl_status_t status = sl_zigbee_broadcast_next_network_key(&key);

  if (status != SL_STATUS_OK) {
    sl_zigbee_app_debug_println("Change Key Error %x", status);
  } else {
    sl_zigbee_app_debug_println("Change Key Success");
  }
}

static void dc_print_key(uint8_t label, uint8_t *key)
{
  uint8_t i;
  sl_zigbee_app_debug_println("key %x: ", label);
  for (i = 0; i < SL_ZIGBEE_ENCRYPTION_KEY_SIZE; i++) {
    sl_zigbee_app_debug_print("%02X", key[i]);
  }
  sl_zigbee_app_debug_println("");
}

void print_next_key_command(sl_cli_command_arg_t *arguments)
{
  (void)arguments;

  sl_status_t status;
  sl_zigbee_sec_man_context_t context;
  sl_zigbee_sec_man_key_t plaintext_key;

  sl_zigbee_sec_man_init_context(&context);

  context.core_key_type = SL_ZB_SEC_MAN_KEY_TYPE_NETWORK;
  context.key_index = 1;

  status = sl_zigbee_sec_man_export_key(&context, &plaintext_key);

  if (status != SL_STATUS_OK) {
    sl_zigbee_app_debug_println("Error getting key");
  } else {
    dc_print_key(1, plaintext_key.key);
  }
}

void version_command(sl_cli_command_arg_t *arguments)
{
  (void)arguments;

  sl_zigbee_app_debug_print("Version:  0.1 Alpha\n");
  sl_zigbee_app_debug_println(" %s", __DATE__);
  sl_zigbee_app_debug_println(" %s", __TIME__);
  sl_zigbee_app_debug_println("");
#ifdef SL_ZIGBEE_TEST
  sl_zigbee_app_debug_println(
    "Print formatter test : 0x%x=0x12, 0x%02x=0x1234 0x%04x=0x12345678",
    0x12,
    0x1234,
    0x12345678);
#endif // SL_ZIGBEE_TEST
}

void set_txpower_command(sl_cli_command_arg_t *arguments)
{
  int8_t dbm = sl_cli_get_argument_int8(arguments, 0);

  sl_zigbee_set_radio_power(dbm);
}

#endif
