# Basic Network Policies captures Project #
![Type badge](https://img.shields.io/badge/Type-Virtual%20application-green)
![Technology badge](https://img.shields.io/badge/Technology-Zigbee-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/Simplicity_SDK-v2024.12.0-green)

## Summary ##

This repository provides projects and network traces about basic network Policies on Zigbee.

## List of Projects ##

| Category | Example name | Description |
|:--|:-------------|:---------------|
| **Joining Policies:** | | |
| 1 | [joining_install_code](./joining_install_code) | Join a network with install-code |
| 2 | [joining_well_known_key](./joining_well_known_key) | Join with the well-known key "ZigbeeAlliance09" |
| **Rejoining Policies:** | | |
| 1 | [secured_rejoining](./secured_rejoining) | Secured Rejoining of a SED aged out of the child table of the parent |
| 2 | [unsecured_rejoining](./unsecured_rejoining) | Unsecured rejoining of a SED which missed a NWK Key update |
| **TC Policies:** | | |
| 1 | [tc_global_link_key](./tc_global_link_key) | Use a global TC link key for all nodes |
| 2 | [tc_hashed_link_key](./tc_hashed_link_key) | Use a global TC Link key which is hashed by the eui64 of each node |
| 3 | [tc_unique_link_key](./tc_unique_link_key) | Use a random unique TC link key for each node joining the network |
| **NWK Key Update:** | | |
| 1 | [network_key_update](./network_key_update) | Demonstrate the network update with broadcast |
| **App Link Keys:** | | |
| 1 | [app_link_key](./app_link_key) | Use a unique APP Link key to secure communication between 2 nodes on a network |
| **Messaging:** | | |
| 1 | [multicast_switch_light](./multicast_switch_light) | Project to send multicast message to a specific Group |

## Structure of Projects ##

You can see in all the projects the following structures:

- **src**: Source code files used for the project
- **SimplicityStudio**: Project files for all the projects
- Traces directly into the directory
- **README**: Explains how to use the project

## SDK version ##

- [Simplicity SDK v2024.12.0](https://github.com/SiliconLabs/simplicity_sdk/releases/tag/v2024.12.0)

## Hardware Required ##

- 3x Wireless Starter Kit Main Board
- 3x Silicon Labs EFR32xG21 Radio Board (BRD4180A)

## Connections Required ##

Connect the radio boards to the WSTK mainboards.

## Setup ##

Look at each guidelines present in each projects in this repository.

## How It Works ##

Some Wireshark/Network Analyzer traces are directly available in order to explore the different packet exchanges

## Create your own CLI Commands ##

Here are the steps on how to use/create custom CLI commands, follow the examples in the various projects of this repository for more information.

- Create the handler responsible to handle the command and gets the sl_cli_command_arg_t* arguments.
- Create the CLI command info of type sl_cli_command_info_t with the SL_CLI_COMMAND
- Create the entries which are going to define the command name to put in the CLI and the callback to the correct handler
- Create the group of commands which is going to have the entries of commands in its arguments
- In the emberAfMainInitCallback function, sl_cli_command_add_command_group to the sl_cli_handles

## Usefull links and Applications Notes ##

Here's one useful Application Note on the Zigbee Framework which gives guidance on custom commands and events:

- <https://www.silabs.com/documents/public/user-guides/ug491-zigbee-app-framework-dev-guide-sdk-7x.pdf>
