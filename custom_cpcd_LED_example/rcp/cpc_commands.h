/***************************************************************************//**
 * @file cpc_commands.h
 * @brief
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

#ifndef CPC_COMMANDS_H_
#define CPC_COMMANDS_H_

// primary->secondary commands
#define CPC_COMMAND_LED_ON              '1'
#define CPC_COMMAND_LED_OFF             '0'
// secondary->primary notifications
#define CPC_NOTIFY_LED_ON               '1'
#define CPC_NOTIFY_LED_OFF              '0'

/*
 * Handshake only used to detect a primary connected
 * Primary sends it to the secondary when it connects
 */
#define CPC_COMMAND_HANDSHAKE           'h'

#define CPC_COMMAND_LEN                 1
#define CPC_NOTIFY_LEN                  1

#endif /* CPC_COMMANDS_H_ */
