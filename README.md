# README

This is a WORK IN PROGRESS controller emulator using USB-IP for the kernel layer.

It can be used to emulate most USB devices (if you understand their USB protocol), and includes:
- virtual mice (HID)
- virtual joystick (HID)
- virtual XBox 360 controller (XUSB)
- arbitrary virtual USB HID devices - *advanced*
- arbitrary virtual non-HID USB devices - *advanced*

## How do I use this?

Don't. It's not usable yet.

## Developer notes

USB-IP is used because I can't currently sign Windows KMDF drivers, and UMDF can't implement busses. I want dynamic device lists.

I tried using XInputHID, but it seems to not actually be wired up in Windows to match the HID game devices.

XUSB is implemented based on Microsoft's documentation. The big missing thing was that if you get a request for endpoint 
