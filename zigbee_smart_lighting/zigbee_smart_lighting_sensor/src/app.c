/***************************************************************************//**
 * @file
 * @brief Core application logic.
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

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "sl_string.h"

#include "sl_board_control.h"
#include "sl_simple_button.h"
#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"
#include "mac-types.h"
#include "sl_zigbee_types.h"
#include "find-and-bind-initiator.h"
#include "network-steering.h"

#include "glib.h"
#include "app/framework/include/af.h"
#include "mikroe_pl_n823_01.h"
#include "sl_i2cspm_instances.h"
#include "app_timer.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#define LCD_MAX_CHARACTER_LEN        (16 + 1)
#define QUEUE_LENGTH                 12
#define SENSOR_ENDPOINT              (1)

#define FINDING_AND_BINDING_DELAY_MS 3000
#define LED_BLINK_PERIOD_MS          2000

#define APP_TIMER_INTERVAL_MS        1000

#define PIR_VOLTAGE_OUT_THRESHOLD    10
#define PIR_MAP_OUT_MIN_MV           0
#define PIR_MAP_OUT_MAX_MV           3300

#define led0_on()     sl_led_turn_on(&sl_led_led0);
#define led0_off()    sl_led_turn_off(&sl_led_led0);
#define led0_toggle() sl_led_toggle(&sl_led_led0);

#define led1_on()     sl_led_turn_on(&sl_led_led1);
#define led1_off()    sl_led_turn_off(&sl_led_led1);
#define led1_toggle() sl_led_toggle(&sl_led_led1);

/******************************************************************************/
/*                              PRIVATE DATA                                  */
/******************************************************************************/
static bool commissioning = false;

static sli_zigbee_event_t commissioning_event;
static sli_zigbee_event_t led_event;
static sli_zigbee_event_t finding_and_binding_event;
static sli_zigbee_event_t pir_event;
static sli_zigbee_event_t btn0_event;

static GLIB_Context_t glibContext;
static bool pir_start = false;

static mikroe_i2c_handle_t app_i2c_instance = NULL;

static app_timer_t app_timer;

/*******************************************************************************
 ***************************   LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

/**
 * @brief     Handler for app timer
 * @param     timer
 * @param     data
 */
static void app_timer_handler(app_timer_t *timer, void *data)
{
  (void) timer;
  (void) data;

  sl_zigbee_af_event_set_active(&pir_event);
}

/**
 * @brief   The function displays the current activated sensor state
 * @param   state	The state that is displayed. True means enable,
 * false for disable
 * @return  None
 */
void lcd_display_state(bool state)
{
  char str[LCD_MAX_CHARACTER_LEN];

  GLIB_clear(&glibContext);
  GLIB_setFont(&glibContext, (GLIB_Font_t *)&GLIB_FontNormal8x8);
  snprintf(str, LCD_MAX_CHARACTER_LEN, "    ZIGBEE    ");
  GLIB_drawString(&glibContext, str, sl_strlen(str), 5, 10, 0);
  snprintf(str, LCD_MAX_CHARACTER_LEN, "OCCUPANCY SENSOR");
  GLIB_drawString(&glibContext, str, sl_strlen(str), 0, 25, 0);
  snprintf(str, LCD_MAX_CHARACTER_LEN, "     SENSOR   ");
  GLIB_drawString(&glibContext, str, sl_strlen(str), 0, 60, 0);
  snprintf(str,
           LCD_MAX_CHARACTER_LEN,
           state == true?"     ENABLE   ":"    DISABLE    ");
  GLIB_drawString(&glibContext, str, sl_strlen(str), 1, 75, 0);
  DMD_updateDisplay();
}

/**
 * @brief       Initializes LCD to display sensor's state
 * @param       None
 * @return      None
 */
void lcd_init(void)
{
  sl_status_t status;

  // Enable the memory lcd
  status = sl_board_enable_display();
  EFM_ASSERT(status == SL_STATUS_OK);

  // Initialize the DMD module for the DISPLAY device driver
  status = DMD_init(0);
  if (DMD_OK != status) {
    sl_zigbee_app_debug_print("DMD_init error");
    return;
  }

  // Initialize the glib context
  status = GLIB_contextInit(&glibContext);
  if (GLIB_OK != status) {
    sl_zigbee_app_debug_print("GLIB_contextInit error");
    return;
  }

  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;

  lcd_display_state(false);
}

/**
 * @brief   Handler function for event btn0_event
 * @param   None
 * @return  None
 */
void btn0_event_handler(void)
{
  if (pir_start) {
    //      GPIO_PinOutSet(MOTION_B_PORT, MOTION_B_PIN);
    pir_start = false;
    sl_zigbee_app_debug_print("Disable sensor\r\n");
    app_timer_stop(&app_timer);
    lcd_display_state(false);
  } else {
    pir_start = true;
    sl_zigbee_app_debug_print("Enable sensor\r\n");
    app_timer_start(&app_timer,
                    APP_TIMER_INTERVAL_MS,
                    app_timer_handler,
                    NULL,
                    true);
    lcd_display_state(true);
  }
}

/**
 * @brief       Handler function for event led_event
 * @param       None
 * @return      None
 */
void led_event_handler(void)
{
  if (commissioning) {
    led0_toggle();
    sli_zigbee_event_set_delay_ms(&led_event, LED_BLINK_PERIOD_MS);
  } else if (sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    led0_on();
  }
}

/**
 * @brief       Handler function for motion event
 * @param       None
 * @return      None
 */
void pir_event_handler(void)
{
  sl_status_t status;
  uint16_t adc_val;
  float map_out;
  bool motion;
  static bool prev_motion;

  adc_val = mikroe_pl_n823_01_get_adc();
  map_out = mikroe_pl_n823_01_scale_results(adc_val,
                                            PIR_MAP_OUT_MIN_MV,
                                            PIR_MAP_OUT_MAX_MV);
  motion = map_out < PIR_VOLTAGE_OUT_THRESHOLD ? true : false;

  if (prev_motion == motion) {
    return;
  }

  prev_motion = motion;

  if (motion) {
    sl_zigbee_app_debug_print("[INFO]: Motion DETECTED.\r\n");
    led1_on();
    sl_zigbee_af_fill_command_on_off_cluster_on();
    sl_zigbee_app_debug_print("Command is zcl on-off ON\r\n");
  } else {
    led1_off();
    sl_zigbee_af_fill_command_on_off_cluster_off();
    sl_zigbee_app_debug_print("Command is zcl on-off OFF\r\n");
  }

  if (sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK) {
    sl_zigbee_af_get_command_aps_frame()->sourceEndpoint = SENSOR_ENDPOINT;
    status = sl_zigbee_af_send_command_unicast_to_bindings();
    sl_zigbee_app_debug_print("%p: 0x%X", "Send to bindings", status);
  }
}

/**
 * @brief       Handler function for event finding_and_binding_event
 * @param       None
 * @return      None
 */
void finding_and_binding_event_handler(void)
{
  sl_status_t status;
  status = sl_zigbee_af_find_and_bind_initiator_start(SENSOR_ENDPOINT);
  sl_zigbee_af_debug_print("Find and bind initiator %p: 0x%X", "start",
                           status);
  UNUSED_VAR(status);
}

/**
 * @brief       scheduleFindingAndBindingForInitiator() function.
 * Used to set finding_and_binding_event event for finding&binding process
 * @param   None
 * @return  None
 */
void scheduleFindingAndBindingForInitiator(void)
{
  sli_zigbee_event_set_delay_ms(&finding_and_binding_event,
                                FINDING_AND_BINDING_DELAY_MS);
}

/**
 * @brief       Handler function for event commissioning_event
 * @param       None
 * @return      None
 */
void commissioning_event_handler(void)
{
  sl_status_t status;

  if (sl_zigbee_af_network_state() != SL_ZIGBEE_JOINED_NETWORK) {
    // Clear binding table
    status = sl_zigbee_clear_binding_table();
    sl_zigbee_af_debug_print("%s 0x%x", "Clear binding table\n", status);

    // Leave network
    status = sl_zigbee_leave_network(SL_ZIGBEE_LEAVE_NWK_WITH_NO_OPTION);
    sl_zigbee_af_debug_print("%s 0x%x", "leave\n", status);

    status = sl_zigbee_af_network_steering_start();
    sl_zigbee_af_debug_print("%p network %p: 0x%X",
                             "Join",
                             "start",
                             status);
    UNUSED_VAR(status);
    sli_zigbee_event_set_active(&led_event);
    commissioning = true;
  } else {
    if (!commissioning) {
      scheduleFindingAndBindingForInitiator();
      sl_zigbee_af_debug_print("%p %p",
                               "FindAndBind",
                               "start");
      sli_zigbee_event_set_active(&led_event);
      commissioning = true;
    } else {
      sl_zigbee_app_debug_print("Already in commissioning mode");
    }
  }
}

/**
 * @brief       Override for sl_zigbee_af_main_init_cb() function
 * @param       None
 * @return      None
 */
void sl_zigbee_af_main_init_cb(void)
{
  sl_status_t status;
  sl_zigbee_af_event_init(&commissioning_event, commissioning_event_handler);
  sl_zigbee_af_event_init(&led_event, led_event_handler);
  sl_zigbee_af_event_init(&finding_and_binding_event,
                          finding_and_binding_event_handler);
  sl_zigbee_af_event_init(&pir_event, pir_event_handler);
  sl_zigbee_af_isr_event_init(&btn0_event, (void *)btn0_event_handler);

  // Initialize lcd
  lcd_init();

  // Initialize PIR sensor
  sl_zigbee_app_debug_print("==     MikroE PIR Click initialization     ==\n");
  sl_zigbee_app_debug_print("=============================================\n");

  app_i2c_instance = sl_i2cspm_mikroe;
  status = mikroe_pl_n823_01_init(app_i2c_instance);

  if (status != SL_STATUS_OK) {
    sl_zigbee_app_debug_print("MikroE PIR Click initialized failed!\n");
  }
}

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action.  The return code
 * from this callback is ignored by the framework.  The framework will always
 * process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
bool sl_zigbee_af_stack_status_cb(sl_status_t status)
{
  if (status == SL_STATUS_NETWORK_DOWN) {
    led0_off();
  } else if (status == SL_STATUS_NETWORK_UP) {
    led0_on();
  }
  sl_zigbee_app_debug_print("Network status: %d\n", status);
  // This value is ignored by the framework.
  return false;
}

/**
 * @brief        Push button event handler
 * @param        None
 * @return       None
 */
void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) == SL_SIMPLE_BUTTON_RELEASED) {
    if (&sl_button_btn0 == handle) {
      sli_zigbee_event_set_active(&btn0_event);
    } else if (&sl_button_btn1 == handle) {
      sli_zigbee_event_set_active(&commissioning_event);
    }
  }
}

/** @brief Complete
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to SL_STATUS_OK to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt. Ver.: always
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID. Ver.: always
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network. Ver.: always
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete. Ver.: always
 */
void sl_zigbee_af_network_steering_complete_cb(sl_status_t status,
                                               uint8_t totalBeacons,
                                               uint8_t joinAttempts,
                                               uint8_t finalState)
{
  sl_zigbee_app_debug_print("%p network %p: 0x%X", "Join", "complete", status);

  if (status != SL_STATUS_OK) {
    commissioning = false;
  } else {
    scheduleFindingAndBindingForInitiator();
  }
}

/** @brief Complete
 *
 * This callback is fired by the initiator when the Find and Bind process is
 * complete.
 *
 * @param status Status code describing the completion of the find and bind
 * process Ver.: always
 */
void sl_zigbee_af_find_and_bind_initiator_complete_cb(sl_status_t status)
{
  sl_zigbee_app_debug_print("Find and bind initiator %p: 0x%X",
                            "complete",
                            status);

  commissioning = false;
}
