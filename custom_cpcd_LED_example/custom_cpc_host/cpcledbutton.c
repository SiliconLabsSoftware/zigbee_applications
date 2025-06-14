/***************************************************************************//**
 * @file cpcledbutton.c
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
#include "sl_cpc.h"
#include <stdlib.h>
#include <time.h>
#include "cpc_commands.h"
#include <ncurses.h>
#include <ctype.h>

static cpc_handle_t lib_handle;
static cpc_endpoint_t endpoint;

static void connectcpc()
{
  uint8_t retry = 0;
  int ret;
  do {
    ret = cpc_init(&lib_handle, NULL, false, NULL);
    if (ret == 0) {
      // speed up boot process if everything seems ok
      break;
    }
    nanosleep((const struct timespec[]){ { 0, 100000000L } }, NULL);
    retry++;
  } while ((ret != 0) && (retry < 5));

  if (ret < 0) {
    fprintf(stderr, "cpc_init returned with %d\n", ret);
    exit(-2);
  }

  ret = cpc_open_endpoint(lib_handle,
                          &endpoint,
                          SL_CPC_ENDPOINT_USER_ID_0,
                          1);
  if (ret < 0) {
    fprintf(stderr, "cpc_open_endpoint returned with %d\n", ret);
    exit(-3);
  }
}

static void sendCmd(char command)
{
  cpc_write_endpoint(endpoint,
                     &command,
                     CPC_COMMAND_LEN,
                     CPC_ENDPOINT_WRITE_FLAG_NONE);
}

// Read RCP data from CPC
static char getNotify()
{
  uint8_t data_from_cpc[SL_CPC_READ_MINIMUM_SIZE];
  int size = cpc_read_endpoint(endpoint,
                               data_from_cpc,
                               SL_CPC_READ_MINIMUM_SIZE,
                               CPC_ENDPOINT_READ_FLAG_NON_BLOCKING);

  if (size >= 1) {
    return data_from_cpc[0];
  }
  return '\0';
}

static void handshake()
{
  printf("Sending handshake\r\n");
  sendCmd(CPC_COMMAND_HANDSHAKE);
}

static void disconnectcpc()
{
  cpc_close_endpoint(&endpoint);
}

void printMessage(int row, int col, const char *message)
{
  mvprintw(row, col, message);
  clrtoeol();   // Clear the line
  refresh();
}

int main(int argc, char *argv[])
{
  connectcpc();
  handshake();

  // Set up terminal interface
  initscr();
  noecho();
  nodelay(stdscr, TRUE);
  bool printUsage = true;

  while (1) {
    char ch = getch();

    if (printUsage) {
      printMessage(0, 0, "Press 1 to turn on LED or 0 to turn it off.");
      printMessage(1, 0, "Press 'Q' to quit.");
      printUsage = false;
    }

    if (ch == '1') {
      sendCmd('1');
      printMessage(3, 0, "LED on command sent from Host");
    } else if (ch == '0') {
      sendCmd('0');
      printMessage(3, 0, "LED off command sent from Host");
    } else if (toupper(ch) == 'Q') {
      endwin();
      break;
    } else if ((ch >= 0x20) && (ch <= 0x7F)) {
      // If any other character is pressed
      printUsage = true;
    }

    // Recieve message from seconday
    char notifyCh = getNotify();
    // Check if message from secondary is a valid ASCII character
    if ((notifyCh >= 0x20) && (notifyCh <= 0x7F)) {
      char output[30];
      snprintf(output, 30, "Notify from secondary: %c", notifyCh);
      printMessage(5, 0, output);
    }

    // Sleep for a short duration to control update rate
    napms(100);
  }

  disconnectcpc();
  return 0;
}
