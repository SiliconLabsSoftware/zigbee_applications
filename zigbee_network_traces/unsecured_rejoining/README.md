# UNSecured/TC Rejoin: with Sleepy-End Device #
![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Zigbee-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2025.6.0-green)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-240.01%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-16.73%20KB-blue)

## Summary ##

This project shows Unsecured Rejoining process in the case where a SED missed a NWK Key update or loses its parent. This rejoining process can be two types : Secured Rejoining or Unsecured rejoined.
This project shows the **UN**Secured Rejoining in the case a SED missed a NWK Key Update.
This rejoin type is also called TC Rejoin.
For this project, we use the TC unique link keys.

## SDK version ##

[SiSDK v2025.6.0](https://github.com/SiliconLabs/simplicity_sdk/releases/tag/v2025.6.0)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)

## Hardware Required ##

- 3x Wireless Starter Kit Main Board
- 3x Silicon Labs EFR32xG21 Radio Board (BRD4180A)

## Connections Required ##

Connect the radio boards to the WSTK mainboards. Connect your desired gateway device via serial connection to a computer.

## Setup ##

> [!NOTE]
> Make sure that the [zigbee_applications](https://github.com/SiliconLabs/zigbee_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the EXAMPLE PROJECTS & DEMOS tab. Find the example project filtering by "rejoining".

2. Click the Create button on the project corresponding to your device type (coordinator, router and sleepy end device). The project creation dialog pops up -> click Create and Finish and the Project should be generated.

   ![create_project](image/create_project.png)

   - Build and flash the **zigbee_unsecured_rejoining_coordinator** application to one board (Coordinator).
   - Build and flash the **zigbee_unsecured_rejoining_router** application to another one (Router).
   - Build and flash the **zigbee_unsecured_rejoining_sleepy_end_device** application to the last one (Sleep End Device (SED)).

## How It Works ##

- Form the network on the coordinator by sending `form` through the serial
- If you want to capture packets, use `keys print` to get the NWK key and add it to your keys
- Use `open` to Open the network
- Send `join` to both the Router and SED to allow them to join the network
- When both the Router and SED joined the network, send `updateNwk` on the TC to launch update NWK Key
- Wait 10 to 20 seconds for the updateNwk to finish, until Switch Key packet is sent
- Send `rejoin` on the SED side to launch the process. You should see in the console the following message
![alt text](image/networkRejoinConsole.png "Unsecured Rejoin successful output")

### Interpretation ###

The rejoin process is composed of 4 messages which are Rejoin Request, Rejoin response, NWK Key and device announce.

![alt text](image/unsecuredTraceRejoin.png "Unsecured Rejoin Capture")

We can see that the NWK Key is send to the device. The message is unencrypted at the NWK layer and
crypted at the APP Layer using the unique TC Link key.

![alt text](image/nwkKeySend.png "Network Key Exchange")

> [!WARNING]
> You can't perform a unsecured rejoin while using only hashed TC Link keys and well-known key for joining. You need to use unique keys or install-code in order for the TC to accept an
unsecured-rejoin. This is done in order to increase security of the network.

## Traces & other documents ###

This directory also contains trace captures on Network Analyzer (NA) and Wireshark directly in the repositories

- trace_unsecured_rejoining.isd : Capture for NA
- trace_unsecured_rejoining.pcapng : Capture for Wireshark

**To use Wireshark Capture**
You need to add the well-known key to your wireshark keys to decode packets.
Go to : [Edit -> Preferences -> Protocols -> ZigBee -> Edit] and add :
**5A:69:67:42:65:65:41:6C:6C:69:61:6E:63:65:30:39** as the well-known key

To get more informations : [AN1233: Zigbee Security](https://www.silabs.com/documents/public/application-notes/an1233-zigbee-security.pdf)\
[UG391: Zigbee Application Framework dev guide](https://www.silabs.com/documents/public/user-guides/ug391-zigbee-app-framework-dev-guide.pdf)
