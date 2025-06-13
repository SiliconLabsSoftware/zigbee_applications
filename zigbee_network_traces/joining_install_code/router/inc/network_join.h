/***************************************************************************//**
 * @file network_join.h
 * @brief Header file
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

#ifndef NETWORK_JOIN_H_
#define NETWORK_JOIN_H_

/***************************************************************************//**
 * INCLUDES & DEFINES
 ******************************************************************************/

#include "sl_cli_types.h"
#include "app/framework/include/af.h"
#include "stdbool.h"
#include "sl_cli_handles.h"
#include "af-security.h"
#include "network-steering.h"

#define INFO_DELAY_MS 1000

/***************************************************************************//**
 * Function Prototypes
 ******************************************************************************/

void my_join_handler(sl_cli_command_arg_t *arguments);
void my_event_router_info_handler(sl_zigbee_event_context_t *context);

/***************************************************************************//**
 * Variables
 ******************************************************************************/

extern sl_zigbee_af_event_t event_router_info_ctrl;
extern sl_cli_command_group_t my_cli_command_group;

#endif // NETWORK_JOIN_H_
