/***************************************************************************//**
 * @file button_multicast.c
 * @brief File to create a group and send Multicast Message.
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
 *  -
 *  -
 * HOW TO USE :
 *
 ******************************************************************************/

#include "button_multicast.h"

/***************************************************************************//**
 * Variable declarations / Includes.
 ******************************************************************************/

static uint8_t current_btn_state = 0;

/***************************************************************************//**
 * Functions & events.
 ******************************************************************************/

extern sl_zigbee_af_event_t send_command;
extern void send_command_handler(sl_zigbee_af_event_t *event);

/** @brief
 *
 * Application Callback when push-button pressed
 */
void sl_button_on_change(const sl_button_t *handle)
{
  if (SL_SIMPLE_BUTTON_INSTANCE(BUTTON0) == handle) {
    if (sl_simple_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
      sl_zigbee_core_debug_println("Button0 is pressed");
      if (current_btn_state == 0) { // ON Command
        sl_zigbee_core_debug_println("Command is zcl on-off ON");
        sl_zigbee_af_fill_command_on_off_cluster_on();
        current_btn_state = 1;
      } else { // OFF command
        sl_zigbee_core_debug_println("Command is zcl on-off OFF");
        sl_zigbee_af_fill_command_on_off_cluster_off();
        current_btn_state = 0;
      }
      sl_zigbee_af_set_command_endpoints(1, 1);
      sl_zigbee_af_event_set_active(&send_command);
    }
  }
}

/** @brief        Callback when triggering send_command event
 *  @param        Triggering event
 */
void send_command_handler(sl_zigbee_af_event_t *event)
{
  sl_802154_short_addr_t alias = SL_ZIGBEE_NULL_NODE_ID;
  uint8_t sequence = 0;
  sl_status_t status;

  status = sl_zigbee_af_send_command_multicast(GROUP_ID,
                                               alias,
                                               sequence);

  if (status == SL_STATUS_OK) {
    sl_zigbee_core_debug_println("Command is successfully sent");
  } else {
    sl_zigbee_core_debug_println("Failed to send");
    sl_zigbee_core_debug_println("Status code: 0x%x", status);
  }
}
