# CAN-USB 

Top-level repo for tracking the progress of the CAN-USB project, pre-configured with the correct CANBUS settings for any bitrate.

## CANUSB
This project is based on the CANUSB protocol developed by LAWICEL AB. It has been tested with official LAWICEL libraries for Linux (slcan) and custom windows apps (see wincan repository) and extends upon it to support 2 CANBUS channels with the same pinout as the Vector VN16xx series. The caveat is that, because of the choice of microcontroller, only one CANBUS channel can be selected at once, hence I have extended upon the original standard in, what I call, the CANBUSx 1.0 protocol. 

## CANBUSx 1.0
The commands in this version of the protocol are exactly the same as the ones used in the standard protocol, however, I have added the following command:

```bash
-cN[CR]
```

where N is the zero-indexed number of the channel (range of [0, 1]). It returns \r if sucessfull and \a if not. It can, and should, only be called if the channel is closed.

The idea for this protocol, in the future, is to support the CAN-FD and CAN-XL standards, as well as reading multiple channels at once.

## Limitations (known/intentional)

Since the STM32F303 microcontroller I have selected only has one CAN interface, only one CAN channel is supported.

Acceptance Code Register and Acceptance Mask Register commands (-M and -m) are not yet supported.

## Hardware

... is stored on a separate repo and included in this repo as a submodule to keep Altium from freaking out. 

## Software

... is stored in the software folder. It is an STMCube project and can be opened in the program without issue.