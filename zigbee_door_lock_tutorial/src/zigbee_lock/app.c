/***************************************************************************//**
 * @file app.c
 * @brief Top level application functions
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

#include "app/framework/include/af.h"
#include "sl_simple_led_instances.h"
#include "zap-id.h"
#define DOOR_LOCK_ENDPOINT 1
typedef enum {
  DOOR_NOT_FULLY_LOCKED = 0x00,
  DOOR_LOCKED = 0x01,
  DOOR_UNLOCKED = 0x02,
  DOOR_UNDEFINED=0xFF
} doorLockState_t;
doorLockState_t doorLockStatus;

/** @brief Complete network steering.
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to SL_STATUS_OK to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt. Ver.: always
 *
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID. Ver.: always
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network. Ver.: always
 *
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete. Ver.: always
 */
void sl_zigbee_af_network_steering_complete_cb(sl_status_t status,
                                               uint8_t totalBeacons,
                                               uint8_t joinAttempts,
                                               uint8_t finalState)
{
  UNUSED_VAR(status);
  UNUSED_VAR(totalBeacons);
  UNUSED_VAR(joinAttempts);
  UNUSED_VAR(finalState);
  sl_zigbee_app_debug_println("%s network %s: 0x%02X",
                              "Join",
                              "complete",
                              status);
}

/** @brief
 *
 * Application framework equivalent of ::sl_zigbee_radio_needs_calibrating_handler
 */
void sl_zigbee_af_radio_needs_calibrating_cb(void)
{
  sl_mac_calibrate_current_channel();
}

/** @brief Check Door Lock Pin
 *
 * This function checks if the door lock pin is correct. It will return TRUE if
 * the pin is correct or the pin token is set to the default pin and FALSE if
 * the pin is not correct.
 *
 * @param PIN
 */
// function to check the input pin is the correct one
boolean checkDoorLockPin(int8u *PIN)
{
  bool isPinCorrect = FALSE;
  bool isPinInUse = FALSE;
  uint8_t maxPinLength;
  uint8_t minPinLength;
  doorLockPin_t storedPin;

  halCommonGetToken(&isPinInUse, TOKEN_DOOR_LOCK_PIN_IN_USE);

  if (isPinInUse) {
    sl_zigbee_app_debug_println("Lock pin in use \n");
  } else {
    sl_zigbee_app_debug_println("Lock pin Not in use \n");
  }

  // Get the Max Pin Length to DOOR_LOCK_PIN_STRING_MAX_LENGTH
  sl_zigbee_af_read_server_attribute(DOOR_LOCK_ENDPOINT,
                                     ZCL_DOOR_LOCK_CLUSTER_ID,
                                     ZCL_MAX_PIN_LENGTH_ATTRIBUTE_ID,
                                     &maxPinLength,
                                     ZCL_DATA8_ATTRIBUTE_TYPE);

  // Get the Min Pin Length to DOOR_LOCK_PIN_STRING_LENGTH
  sl_zigbee_af_read_server_attribute(DOOR_LOCK_ENDPOINT,
                                     ZCL_DOOR_LOCK_CLUSTER_ID,
                                     ZCL_MIN_PIN_LENGTH_ATTRIBUTE_ID,
                                     &minPinLength,
                                     ZCL_DATA8_ATTRIBUTE_TYPE);

  // If no pin is initialized, we will return TRUE
  if (isPinInUse == TRUE) {
    // Get the Pin from the token
    halCommonGetToken(&storedPin, TOKEN_DOOR_LOCK_PIN);
    // First check that the PIN is within the min and Max range
    if ((storedPin[0] <= maxPinLength) && (storedPin[0] >= minPinLength)) {
      // Check the input pin against the stored pin
      int8_t result = memcmp(storedPin, PIN, storedPin[0]);
      if (result == 0) {
        isPinCorrect = TRUE;
      }
    }
  } else {
    isPinCorrect = TRUE;
  }
  return isPinCorrect;
}

/** @brief Door Lock command callback function
 *
 * This function lock the door if the door lock pin is correct.
 * It will return TRUE if the pin is correct or the pin token is set to
 * the default pin and FALSE if the pin is not correct.
 *
 * @param PIN
 */
boolean sl_zigbee_af_door_lock_cluster_lock_door_cb(int8u *PIN)
{
  // Check if the pin is correct
  boolean isPinCorrect = checkDoorLockPin(PIN);
  if (isPinCorrect == TRUE) {
    doorLockStatus = DOOR_LOCKED;
    // light on LED to simulate the lock
    sl_led_turn_on(&sl_led_led0);
    // We will set the Door Lock State attribute to "Locked"
    sl_zigbee_af_write_server_attribute(DOOR_LOCK_ENDPOINT,
                                        ZCL_DOOR_LOCK_CLUSTER_ID,
                                        ZCL_LOCK_STATE_ATTRIBUTE_ID,
                                        &doorLockStatus,
                                        ZCL_ENUM8_ATTRIBUTE_TYPE);
    // Send a Door Lock Response with status success
    sl_zigbee_af_fill_command_door_lock_cluster_lock_door_response(
      SL_ZIGBEE_ZCL_STATUS_SUCCESS);
    sl_zigbee_af_send_response();
  } else {
    // If the pin is incorrect, send a Door Lock Response with status failure
    sl_zigbee_af_fill_command_door_lock_cluster_lock_door_response(
      SL_ZIGBEE_ZCL_STATUS_FAILURE);
    sl_zigbee_af_send_response();
    return FALSE;
  }
  return TRUE;
}

/** @brief Door unLock command callback function
 *
 * This function unlock the door if lock pin is correct. It will return TRUE if
 * the pin is correct or the pin token is set to the default pin and FALSE if
 * the pin is not correct.
 *
 * @param PIN
 */
boolean sl_zigbee_af_door_lock_cluster_unlock_door_cb(int8u *PIN)
{
  doorLockState_t doorLockStatus;
  // Check if the pin is correct
  boolean isPinCorrect = checkDoorLockPin(PIN);
  if (isPinCorrect == TRUE) {
    doorLockStatus = DOOR_UNLOCKED;
    // LED off simulate door closed
    sl_led_turn_off(&sl_led_led0);
    // We will set the Door Lock State attribute to "unLocked"
    sl_zigbee_af_write_server_attribute(DOOR_LOCK_ENDPOINT,
                                        ZCL_DOOR_LOCK_CLUSTER_ID,
                                        ZCL_LOCK_STATE_ATTRIBUTE_ID,
                                        &doorLockStatus,
                                        ZCL_ENUM8_ATTRIBUTE_TYPE);
    // Send a Door Lock Response with status success
    sl_zigbee_af_fill_command_door_lock_cluster_lock_door_response(
      SL_ZIGBEE_ZCL_STATUS_SUCCESS);
    sl_zigbee_af_send_response();
  } else {
    // If the pin is incorrect, send a Door Lock Response with status failure
    sl_zigbee_af_fill_command_door_lock_cluster_lock_door_response(
      SL_ZIGBEE_ZCL_STATUS_FAILURE);
    sl_zigbee_af_send_response();
    return FALSE;
  }
  return TRUE;
}

boolean sl_zigbee_af_door_lock_cluster_set_pin_cb(int8u *pin)
{
  uint8_t maxPinLength;
  uint8_t minPinLength;
  // Get the Max Pin Length to DOOR_LOCK_PIN_STRING_MAX_LENGTH
  sl_zigbee_af_read_server_attribute(DOOR_LOCK_ENDPOINT,
                                     ZCL_DOOR_LOCK_CLUSTER_ID,
                                     ZCL_MAX_PIN_LENGTH_ATTRIBUTE_ID,
                                     &maxPinLength,
                                     ZCL_DATA8_ATTRIBUTE_TYPE);

  // Get the Min Pin Length to DOOR_LOCK_PIN_STRING_LENGTH
  sl_zigbee_af_read_server_attribute(DOOR_LOCK_ENDPOINT,
                                     ZCL_DOOR_LOCK_CLUSTER_ID,
                                     ZCL_MIN_PIN_LENGTH_ATTRIBUTE_ID,
                                     &minPinLength,
                                     ZCL_DATA8_ATTRIBUTE_TYPE);

  if ((pin[0] <= maxPinLength) && (pin[0] >= minPinLength)) {
    boolean isPinInUse = TRUE;
    // Set the pin tokens
    halCommonSetToken(TOKEN_DOOR_LOCK_PIN, pin);
    halCommonSetToken(TOKEN_DOOR_LOCK_PIN_IN_USE, &isPinInUse);
    // If the pin is incorrect, send a Set Pin Response with status success
    sl_zigbee_af_fill_command_door_lock_cluster_set_pin_response(
      SL_ZIGBEE_ZCL_STATUS_SUCCESS);
    sl_zigbee_af_send_response();
  } else {
    // If the pin is incorrect, send a Set Pin Response with status failure
    sl_zigbee_af_fill_command_door_lock_cluster_set_pin_response(
      SL_ZIGBEE_ZCL_STATUS_NOT_AUTHORIZED);
    sl_zigbee_af_send_response();
    return FALSE;
  }
  return TRUE;
}

boolean sl_zigbee_af_door_lock_cluster_get_pin_cb(int16u userId)
{
  bool isPinInUse = FALSE;
  doorLockPin_t storedPin;
  halCommonGetToken(&isPinInUse, TOKEN_DOOR_LOCK_PIN_IN_USE);
  // Put the stored pin into a buffer to send
  halCommonGetToken(&storedPin, TOKEN_DOOR_LOCK_PIN);
  // Send the Get Pin response containing the pin
  sl_zigbee_af_fill_command_door_lock_cluster_get_pin_response(0,
                                                               0,
                                                               0,
                                                               storedPin);
  sl_zigbee_af_send_response();
  return TRUE;
}

boolean sl_zigbee_af_door_lock_cluster_clear_pin_cb(int16u userId)
{
  if (userId == 1) {
    doorLockPin_t newPin = DOOR_LOCK_DEFAULT_PIN;
    boolean isPinInUse = FALSE;
    // Reset the pin and tokens
    halCommonSetToken(TOKEN_DOOR_LOCK_PIN, &newPin);
    halCommonSetToken(TOKEN_DOOR_LOCK_PIN_IN_USE, &isPinInUse);
    // If the pin is incorrect, send a Clear Pin Response with status success
    sl_zigbee_af_fill_command_door_lock_cluster_clear_pin_response(
      SL_ZIGBEE_ZCL_STATUS_SUCCESS);
    sl_zigbee_af_send_response();
  } else {
    // If the pin is incorrect, send a Clear Pin Response with status not authorized
    sl_zigbee_af_fill_command_door_lock_cluster_clear_pin_response(
      SL_ZIGBEE_ZCL_STATUS_NOT_AUTHORIZED);
    sl_zigbee_af_send_response();
  }
  return TRUE;
}

/** @brief Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 * @param clusterId   Ver.: always
 */
void sl_zigbee_af_cluster_init_cb(int8u endpoint,
                                  sl_zigbee_af_cluster_id_t clusterId)
{
  if (clusterId == ZCL_DOOR_LOCK_CLUSTER_ID) {
    uint8_t maxPinLength = DOOR_LOCK_PIN_STRING_MAX_LENGTH;
    uint8_t minPinLength = DOOR_LOCK_PIN_STRING_MIN_LENGTH;
    // Set the Max Pin Length to DOOR_LOCK_PIN_STRING_MAX_LENGTH
    sl_zigbee_af_write_server_attribute(DOOR_LOCK_ENDPOINT,
                                        ZCL_DOOR_LOCK_CLUSTER_ID,
                                        ZCL_MAX_PIN_LENGTH_ATTRIBUTE_ID,
                                        &maxPinLength,
                                        ZCL_DATA8_ATTRIBUTE_TYPE);

    // Set the Min Pin Length to DOOR_LOCK_PIN_STRING_LENGTH
    sl_zigbee_af_write_server_attribute(DOOR_LOCK_ENDPOINT,
                                        ZCL_DOOR_LOCK_CLUSTER_ID,
                                        ZCL_MIN_PIN_LENGTH_ATTRIBUTE_ID,
                                        &minPinLength,
                                        ZCL_DATA8_ATTRIBUTE_TYPE);
    // Read the Door Lock State
    sl_zigbee_af_read_server_attribute(DOOR_LOCK_ENDPOINT,
                                       ZCL_DOOR_LOCK_CLUSTER_ID,
                                       ZCL_LOCK_STATE_ATTRIBUTE_ID,
                                       &doorLockStatus,
                                       sizeof(doorLockStatus));

    // Set the LED on for locked, off for unlock
    if (doorLockStatus == DOOR_LOCKED) {
      sl_led_turn_on(&sl_led_led0);
    } else {
      sl_led_turn_off(&sl_led_led0);
    }
  }
}

uint32_t sl_zigbee_af_door_lock_cluster_server_command_parse(
  sl_service_opcode_t opcode,
  sl_service_function_context_t *context)
{
  (void)opcode;
  bool wasHandled = false;
  uint8_t *PIN;
  sl_zigbee_af_cluster_command_t *cmd =
    (sl_zigbee_af_cluster_command_t *)context->data;
  uint16_t payloadOffset = cmd->payloadStartIndex;
  if (!cmd->mfgSpecific) {
    switch (cmd->commandId) {
      case ZCL_LOCK_DOOR_COMMAND_ID:
      {
        sl_led_turn_on(&sl_led_led0);
        PIN = sl_zigbee_af_get_string(cmd->buffer,
                                      payloadOffset,
                                      cmd->bufLen);
        wasHandled = sl_zigbee_af_door_lock_cluster_lock_door_cb(PIN);
        break;
      }
      case ZCL_UNLOCK_DOOR_COMMAND_ID:
      {
        PIN = sl_zigbee_af_get_string(cmd->buffer,
                                      payloadOffset,
                                      cmd->bufLen);
        wasHandled = sl_zigbee_af_door_lock_cluster_unlock_door_cb(PIN);
        break;
      }
      case ZCL_SET_PIN_COMMAND_ID:
      {
        PIN = sl_zigbee_af_get_string(cmd->buffer,
                                      payloadOffset + 4,
                                      cmd->bufLen);
        wasHandled = sl_zigbee_af_door_lock_cluster_set_pin_cb(PIN);
        break;
      }
      case ZCL_GET_PIN_COMMAND_ID:
      {
        wasHandled = sl_zigbee_af_door_lock_cluster_get_pin_cb(1);
        break;
      }
      case ZCL_CLEAR_PIN_COMMAND_ID:
      {
        wasHandled = sl_zigbee_af_door_lock_cluster_clear_pin_cb(1);
        break;
      }
    }
  }
  return ((wasHandled)
          ? SL_ZIGBEE_ZCL_STATUS_SUCCESS
          : SL_ZIGBEE_ZCL_STATUS_UNSUP_COMMAND);
}

void sl_zigbee_af_main_init_cb(void)
{
  doorLockPin_t initPin = DOOR_LOCK_DEFAULT_PIN;
  boolean isPinInUse = FALSE;
  sl_zigbee_af_cluster_init_cb(DOOR_LOCK_ENDPOINT,
                               ZCL_DOOR_LOCK_CLUSTER_ID);
  // Set the pin tokens
  halCommonSetToken(TOKEN_DOOR_LOCK_PIN, &initPin);
  halCommonSetToken(TOKEN_DOOR_LOCK_PIN_IN_USE, &isPinInUse);
  sl_led_init(&sl_led_led0);
  sl_zigbee_subscribe_to_zcl_commands(ZCL_DOOR_LOCK_CLUSTER_ID,
                                      0xFFFF,
                                      ZCL_DIRECTION_CLIENT_TO_SERVER,
                                      sl_zigbee_af_door_lock_cluster_server_command_parse);
}
