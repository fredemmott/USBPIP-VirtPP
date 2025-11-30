// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <Windows.h>
#include <Usb100.h>

typedef USB_CONFIGURATION_DESCRIPTOR FredEmmott_USBSpec_ConfigurationDescriptor;
#define FredEmmott_USBSpec_ConfigurationDescriptor_Size sizeof(USB_CONFIGURATION_DESCRIPTOR)

typedef USB_DEVICE_DESCRIPTOR FredEmmott_USBSpec_DeviceDescriptor;
#define FredEmmott_USBSpec_DeviceDescriptor_Size sizeof(USB_DEVICE_DESCRIPTOR)

typedef USB_INTERFACE_DESCRIPTOR FredEmmott_USBSpec_InterfaceDescriptor;
#define FredEmmott_USBSpec_InterfaceDescriptor_Size sizeof(USB_INTERFACE_DESCRIPTOR)

typedef USB_ENDPOINT_DESCRIPTOR FredEmmott_USBSpec_EndpointDescriptor;
/** If you port this to another platform, this should exclude `bRefresh` and
 * `bSynchAddress` fields. If you're targetting Linux `ch9.h`, you want
 * `USB_DT_ENDPOINT_SIZE`, not `USB_DT_ENDPOINT_AUDIO_SIZE`
 */
#define FredEmmott_USBSpec_EndPointDescriptor_Size sizeof(USB_ENDPOINT_DESCRIPTOR)

typedef USB_DEVICE_QUALIFIER_DESCRIPTOR FredEmmott_USBSpec_DeviceQualifierDescriptor;
#define FredEmmott_USBSpec_DeviceQualifierDescriptor_Size sizeof(USB_DEVICE_QUALIFIER_DESCRIPTOR)