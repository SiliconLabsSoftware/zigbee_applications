# Application Link Keys #

![Type badge](https://img.shields.io/badge/Type-Virtual%20Application-green)
![Technology badge](https://img.shields.io/badge/Technology-Zigbee-green)
![License badge](https://img.shields.io/badge/License-Zlib-green)
![SDK badge](https://img.shields.io/badge/SDK-v2025.6.0-green)
![Build badge](https://img.shields.io/badge/Build-passing-green)
![Flash badge](https://img.shields.io/badge/Flash-240.13%20KB-blue)
![RAM badge](https://img.shields.io/badge/RAM-16.73%20KB-blue)

## Summary ##

Zigbee can also use an additional security feature for end-to-end communication between 2 devices
on the same network by using **Application Link Keys**. This key is going to encrypt the APS layer between 2 nodes on the network, and and also decrypt when receiving data. Only the 2 nodes are aware of this Link key, even the Trust Center does not have access to it. The trust center has only the role to give the keys to the two nodes.

## SDK version ##

- [SiSDK v2025.6.0](https://github.com/SiliconLabs/simplicity_sdk/releases/tag/v2025.6.0)

## Software Required ##

- [Simplicity Studio v5 IDE](https://www.silabs.com/developers/simplicity-studio)

## Hardware Required ##

- 3x Wireless Starter Kit Main Board
- 3x Silicon Labs EFR32xG21 Radio Board (BRD4180A)

## Connections Required ##

Connect the radio boards to the WSTK mainboards. Connect your desired gateway device via serial connection to a computer.

## Setup ##

### Create a project based on an example project ###

> [!NOTE]
> Make sure that the [zigbee_applications](https://github.com/SiliconLabs/zigbee_applications) repository is added to [Preferences > Simplicity Studio > External Repos](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-about-the-launcher/welcome-and-device-tabs).

1. From the Launcher Home, add your hardware to My Products, click on it, and click on the 'EXAMPLE PROJECTS & DEMOS' tab. Find the example project filtering by "link key".

2. Click the Create button on the project corresponding to your device type (coordinator, router and sleepy end device). The project creation dialog pops up -> click Create and Finish and the Project should be generated.

    ![create_project](image/create_project.png)

3. Build and flash the **Zigbee - Application Link Key - Coordinator** project to one board (Coordinator).

4. Build and flash the **Zigbee - Application Link Key - Router** project to another one (Router).

5. Build and flash the **Zigbee - Application Link Key - Sleepy End Device** project to the last one (Sleepy End Device).

## How It Works ##

To use app link key, you must set the variable emberAppLinkKeyRequestPolicy to EMBER_ALLOW_APP_LINK_KEY_REQUEST in order to allow app link key requests. You also need to install the "security link keys" component to enable link key table.
Devices store their APP link keys into the link key table. This encryption allows you to do end-to-end communication between two nodes in the same network.

- Form the network on the coordinator by sending `form` through the serial.
- If you want to capture packets, use `keys print` to get the NWK key and add it to your keys.
- Use `open` to Open the network
- Send `join` to both the Router and SED to allow them to join the network
- On the SED, send `appLKey {eui64 of router}` to request a key.

Additional Information:

In order to receive the APP Link key on a SED, you must enable the fast polling mode or poll at a high rate. It is done in appLinkKey:

```c
Waiting for the APS Link Key
currentTask = SL_ZIGBEE_AF_FORCE_SHORT_POLL;
sl_zigbee_af_add_to_current_app_tasks_cb(currentTask);
sl_zigbee_af_set_wake_timeout_bitmask_cb(currentTask);
```

### Interpretation ###

After doing the whole process, it is possible to see that all APP Link keys are stored into the link key table of the 2 nodes involved in the end-to-end communication with this key.

![alt text](image/keys_print_router.png "Keys Print Router")
![alt text](image/keys_print_sed.png "Keys Print SED")

## Traces & other documents ##

This directory also contains trace captures on Network Analyzer (NA) and Wireshark directly in the repositories

- trace_app_link_key.isd : Capture for NA
- trace_app_link_key.pcapng : Capture for Wireshark

**To use Wireshark Capture**
You need to add the well-known key to your wireshark keys to decode packets.
Go to : [Edit -> Preferences -> Protocols -> ZigBee -> Edit] and add :
**5A:69:67:42:65:65:41:6C:6C:69:61:6E:63:65:30:39** as the well-known key

To get more informations :

- [AN1233: Zigbee Security](https://www.silabs.com/documents/public/application-notes/an1233-zigbee-security.pdf)
- [UG391: Zigbee Application Framework dev guide](https://www.silabs.com/documents/public/user-guides/ug391-zigbee-app-framework-dev-guide.pdf)
