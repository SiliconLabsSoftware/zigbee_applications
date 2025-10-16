# ZigBee - SoC Coordinator TCSO #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Zigbee-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2025.6.2-green)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-293.71%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-19.35%20KB-blue)


## Overview ##
This example project demonstrates a **Zigbee 3.0 Trust Center Swap Out (TCSO)** implementation using a System-on-Chip (SoC) configuration for the trust center node. It provides a straightforward reference for developers working with Zigbee networks that require migration or replacement of the trust center while maintaining network security and continuity.

The application provides several command-line interface (CLI) commands for managing key trust center operations, including:
- Printing and saving essential trust center parameters
- Restoring and reforming Zigbee network after a swap out
- Simplifying testing and validation of trust center functionality

## Table of Contents ##

- [ZigBee - SoC Coordinator TCSO](#zigbee---soc-coordinator-tcso)
  - [Overview](#overview)
  - [Table of Contents](#table-of-contents)
  - [Purpose/Scope](#purposescope)
  - [How It Works](#how-it-works)
    - [Workflow Overview](#workflow-overview)
    - [Device EUID Tokens](#device-euid-tokens)
    - [Practical Deployment Consideration](#practical-deployment-consideration)
  - [Prerequisites](#prerequisites)
    - [Hardware Requirements](#hardware-requirements)
      - [Hardware Connection](#hardware-connection)
    - [Software Requirements](#software-requirements)
  - [SDK version](#sdk-version)
  - [Setup](#setup)
    - [Create a project based on an example project](#create-a-project-based-on-an-example-project)
  - [Testing](#testing)
    - [Join Switch to TC1](#join-switch-to-tc1)
    - [Replace TC1 by TC2](#replace-tc1-by-tc2)
  - [Resources](#resources)
  - [Report Bugs \& Get Support](#report-bugs--get-support)

## Purpose/Scope ##

Zigbee 3.0 introduces the Trust Center Swap Out (TCSO, also known as Trust Center Replacement) mechanism, which allows a Zigbee network to replace its trust center seamlessly, without disrupting the network’s security or functionality. In this process, the current trust center (***TC1***) is replaced by a new trust center (***TC2***), while preserving critical network and security parameters such as the operating channel, PAN ID, and network keys.

TCSO is particularly useful in scenarios where the original trust center can no longer continue operating. Common scenarios that may require replacing TC1 include:
- Hardware failure (TC1 becomes physically damaged or non-functional)
- Software corruption (TC1 firmware or configuration becomes unrecoverable)
- Network migration (moving to new hardware with better performance or updated features)
- Planned maintenance (replacing TC1 proactively before end-of-life or deprecation)

By enabling a secure transition from TC1 to TC2, TCSO ensures that Zigbee networks remain resilient, maintain security integrity, and minimize downtime. This project offers a practical, ready-to-use example that serves as a foundation for building Zigbee 3.0 TCSO applications.

## How It Works ##

This project provides a simple software architecture where the same application can be used to operate either as the current trust center (TC1) or the replacement trust center (TC2). The workflow is driven by CLI commands that allow developers to print, save, and restore essential TCSO information.

### Workflow Overview ###

1. Network Formation with TC1
   - TC1 is powered up and forms the Zigbee 3.0 network.
   - Remote devices join TC1, and secure communication is established.

2. Printing TCSO Information
   - Using CLI commands, developers can print critical TCSO information from TC1, including:
     - PAN ID
     - Radio channel
     - TX power
     - Extended PAN ID
     - EUID (Trust Center EUI-64)
     - Global key and its frame counter
     - Network key, frame counter, and sequence number

3. Trust Center Swap Out
   - Power off TC1.
   - Power up TC2 (running the same application binary).
   - Use CLI commands (simple copy-and-paste) to restore the saved TCSO data into TC2.
   - TC2 then forms the network with identical security and identity parameters.

4. Device Continuity
   - Since network and key information is preserved, previously joined devices will automatically recognize TC2 as the trust center.
   - Devices resume communication seamlessly without needing to be re-commissioned.

### Device EUID Tokens ###

The Zigbee stack determines the device’s EUID based on the following priority order of tokens:

1. STACK_RESTORED_EUI64
   - Introduced in Gecko SDK 4.1.3.
   - When not 0xFFFFFFFFFFFFFFFF, the stack uses this token as the EUID.
   - Can be written and updated freely by application code.
2. MFG_CUSTOM_EUI_64
   - Recommended for assigning a custom permanent EUID after manufacturing.
   - If this token is set (not 0xFFFFFFFFFFFFFFFF) and STACK_RESTORED_EUI64 is empty, the stack uses this as the EUID.
   - It can only be written once and can be reset only by using Silicon Labs’ commander.exe tool.
3. MFG_EMBER_EUI_64
   - The factory default EUID, permanently set before shipment.
   - Only takes effect when both STACK_RESTORED_EUI64 and MFG_CUSTOM_EUI_64 are empty.
   - Cannot be modified.

This application creates the CLI commands to demonstrate how to update and read the first 2 tokens.

### Practical Deployment Consideration ###

In real deployments, regular backup of TCSO information is strongly recommended. Storing the trust center parameters (keys, frame counters, and network settings) in a secure cloud server ensures that if TC1 becomes unavailable unexpectedly (e.g., hardware damage, site outage), a new trust center (TC2) can be brought online quickly with minimal disruption.

## Prerequisites ##

### Hardware Requirements ###

- To run this example project, you will need at least 3 Zigbee-enabled development boards: one for TC1, one for TC2, and one for a joining device. Any of the following kits are supported:

  | Part Family | EVK |
  |:-------------|:-------------:|
  | EFR32MG21 | [SLWRB4180B](https://www.silabs.com/development-tools/wireless/slwrb4180b-efr32xg21-wireless-gecko-radio-board?tab=overview) |
  | EFR32MG22 | [SLWRB4182A](https://www.silabs.com/development-tools/wireless/slwrb4182a-efr32xg22-wireless-gecko-radio-board?tab=overview) |
  | EFR32MG24 | [xG24-EK2703A](https://www.silabs.com/development-tools/wireless/efr32xg24-explorer-kit?tab=overview) |
  | EFR32MG24 | [xG24-RB4187C](https://www.silabs.com/development-tools/wireless/xg24-rb4187c-efr32xg24-wireless-gecko-radio-board?tab=overview) |
- You need at least three boards in total. It is acceptable (and often convenient) to use three of the same board type, or any mix of the supported boards.
- An [BRD4002A](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard?tab=overview#overview) is required if you choose to use radio board(4180B, 4182A, 4187C).

#### Hardware Connection ####

The boards shall be connected to the PC via USB cable in order to control the boards through CLI commands. The default baud rate is 115200 with no parity and the stop bits is 1.

### Software Requirements ###

- Simplicity Studio
  - Download the [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)
  - Follow the [Simplicity Studio User Guide](https://docs.silabs.com/simplicity-studio-5-users-guide/1.1.0/ss-5-users-guide-getting-started/install-ss-5-and-software#install-ssv5) to install Simplicity Studio IDE

## SDK version ##

- [SiSDK v2025.6.2](https://docs.silabs.com/sisdk-release-notes/2025.6.2/sisdk-release-notes-overview/)


## Setup ##

At least 3 boards are required - one is the operating coordinator, one is the ZigBee switch, and the other one is the new coordinator to replace the operating one.

The example project *Zigbee - Z3Switch* of ZigBee switch can be found from Simplicity Studio 5.

The firmware of this example shall be applied to the two coordinators to enable the TCSO functionality.

Use Silicon Labs commander.exe to commanMass-erase the flash and userdata page of the boards before the test. "***commander device masserase***" and "***commander device pageerase --region @userdata***" [UG162 Simplicity Commander](https://www.silabs.com/documents/public/user-guides/ug162-simplicity-commander-reference-guide.pdf)

### Create a project based on an example project ###

1. In Simplicity Studio from the Launcher Home, connect one of the supported EVK(brd4180b brd4182a brd2703a brd4187c), click on it, and click on the EXAMPLE PROJECTS & DEMOS tab. Find the example project with the filter "TCSO".

2. Click **Create** button on the *ZigBee - SoC Coordinator TCSO* example. Example project creation dialog pops up -> click Create and Finish and Project should be generated.

3. Build and flash the examples to the board.


**Note:**

- The bootloader is required for this example.

## Testing ##

### Join Switch to TC1 ###

1. Issue the CLI command "***plugin network-creator start 1***" on TC1 to form the centralized network. 

2. Use CLI command "***info***" to check TC1's node ID. If it's not 0x0000, either the network is not formed properly or it's in a network already. Use "***network leave***" to leave the network and go back to previous step.

3. Issue the CLI command "***plugin network-creator-security open-network***" to open the network. (Enable "permit join")

4. Issue the CLI command "***plugin network-steering start 0***" on Switch to start network steering. The switch should join the coordinator.

5. Once it's done, pushing BTN1 on Switch to toggle LED0 on TC1.

### Replace TC1 by TC2 ###

1. Issue the CLI command "***tcso print all***" on TC1 to print its network information. Here is an example:
   ```
   Trust Center Node Information: 
   Node ID: 0
   Pan ID: 0x1278
   Radio Channel: 25
   Radio Power: 3
   Extended Pan ID: 232526A2E07089F0
   ***** Getting EUID information *****
   Stack EUID : (>)705464FFFEEA8ECB 
      MFG EUI64 Token :      (>)705464FFFEEA8ECB
      Custom EUI64 Token :   (>)FFFFFFFFFFFFFFFF
      Restored EUI64 Token : (>)FFFFFFFFFFFFFFFF
   ************************************
   Global Key: DE 9F BC C1 F0 17 32 6A  1B 01 35 7E 10 C5 8C B8  
   Global Key FC : 0x 6 seqNum : 0x24
   Network Key: 40 C1 FF 6D 4C 18 E8 2B  7C 11 70 8B 28 08 2D F3  
   Network Key FC : 0x1CC seqNum : 0x0
   ```
   **Note:** If the coordinator has not formed the network, some of the parameters cannot be retrieved.

2. Shutdown TC1 and power on TC2. Make sure the network of TC2 is not up by issuing "***network leave***".

3. Issue the CLI command either "***tcso save eui64 {TC1's EUID}***" or "***tcso save restored-eui64 {TC1's EUID}***" to Change TC2's EUID. (TC1's EUID can be directly copied from step 1) Please check the section [Device EUID Tokens](#device-euid-tokens) to understand the details between the EUI64 tokens.

   The device will reboot itself to load the new EUID into stack.

   Example:
   ```
   tcso save eui64 {705464FFFEEA8ECB}
   or
   tcso save restored-eui64 {705464FFFEEA8ECB}
   ```
   
   **Note:** This command writes the MFG_CUSTOM_EUI_64 token, which can only be written once. Ensure it is currently set to 0xFFFFFFFFFFFFFFFF by running "***commander tokendump --tokengroup znet***".

   If it is not empty, reset it using the following command: "***commander flash --tokengroup znet --token MFG_CUSTOM_EUI_64:"FFFFFFFFFFFFFFFF"***".

   As STACK_RESTORED_EUI64 is rewritable, CLI command "***tcso save restored-eui64 {FFFFFFFFFFFFFFFF}***" is able to erase it directly.

4. At this step, we are going to save the network and security parameters in TC2.(Those being printed using "***tcso print all***".) Issue all the following CLI commands one by one to save TCSO information on TC2.

   ```
   tcso save pan-id [pan-id]
   tcso save radio-channel [channel]
   tcso save radio-power [power]
   tcso save extended-pan-id [extended-pan-id]
   tcso save global-key {global-key}
   tcso save global-frame-counter [counter]   // Add 0x2000
   tcso save network-key {network-key}
   tcso save network-frame-counter [counter]  // Add 0x2000
   tcso save network-key-sequence-number [network-key-sequence-number]
   ```
   Below is the example to write the information printed printed at step 1.

   ```
   tcso save pan-id 0x1278
   tcso save radio-channel 25
   tcso save radio-power 3
   tcso save extended-pan-id {232526A2E07089F0}
   tcso save global-key {DE 9F BC C1 F0 17 32 6A  1B 01 35 7E 10 C5 8C B8 }
   tcso save global-frame-counter 0x2006
   tcso save network-key {40 C1 FF 6D 4C 18 E8 2B  7C 11 70 8B 28 08 2D F3 }
   tcso save network-frame-counter 0x21CC
   tcso save network-key-sequence-number 0
   ```

5. The parameters saved in the previous step have not yet been stored in non-volatile memory. Use "***tcso print backup***" to review the temporary data in RAM.

6. Issue the CLI command "***tcso restore all***" to restore the TCSO information. TC2 will form the network and reboot itself.

7. Restart the Switch device to trigger the rejoin.

8. Push BTN1 on Switch to toggle LED0 on TC2. 

## Resources ##

- [Introduction of Trust Center Replacement](https://community.silabs.com/s/article/trust-center-replacement?language=en_US)
- [Another KBA which details frame counter and EUI64](https://community.silabs.com/s/article/trust-center-swap-out?language=en_US)
- [SoC example using ZNet 6.3](https://community.silabs.com/s/article/soc-trust-center-replacement) Especially thanks to Arif-Aztech who shared the example using ZNet 7.4.5.
- [KBA of Restored EUI64](https://community.silabs.com/s/article/restoring-a-custom-EUI64-after-a-trust-center-replacement)
- [UG162 Simplicity Commander](https://www.silabs.com/documents/public/user-guides/ug162-simplicity-commander-reference-guide.pdf)
- [ZigBee Security](https://docs.silabs.com/zigbee/latest/zigbee-security/)

## Report Bugs & Get Support ##

You are always encouraged and welcome to report any issues you find via the [Silicon Labs Community](https://www.silabs.com/community)
