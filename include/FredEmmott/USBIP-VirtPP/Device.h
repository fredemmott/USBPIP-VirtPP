// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

/***** THIS FILE IS FOR IMPLEMENTING A RAW USB DEVICE ******
 *
 * You probably want HIDDevice.h instead.
 */

#include "Core.h"
#include "Request.h"

#ifdef __cplusplus
#include <cstdint>

extern "C" {
#endif

/***** Device:: types *****/

struct FredEmmott_USBIP_VirtPP_Device_Callbacks {
  FredEmmott_USBIP_VirtPP_Result (*OnInputRequest)(
    FredEmmott_USBIP_VirtPP_RequestHandle,
    uint32_t endpoint,
    uint8_t requestType,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length
    );
};

struct FredEmmott_USBIP_VirtPP_Device_InitData {
  void* mUserData;
  FredEmmott_USBIP_VirtPP_Device_Callbacks mCallbacks;

  bool mAutoAttach;
  FredEmmott_USBSpec_DeviceDescriptor const* mDeviceDescriptor;
  uint8_t mNumInterfaces;
  FredEmmott_USBSpec_InterfaceDescriptor const* mInterfaceDescriptors;
};

/***** Device:: methods *****/

FredEmmott_USBIP_VirtPP_DeviceHandle FredEmmott_USBIP_VirtPP_Device_Create(
  FredEmmott_USBIP_VirtPP_InstanceHandle,
  FredEmmott_USBIP_VirtPP_Device_InitData const*);
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Device_Attach(
  FredEmmott_USBIP_VirtPP_DeviceHandle);
void FredEmmott_USBIP_VirtPP_Device_Destroy(
  FredEmmott_USBIP_VirtPP_DeviceHandle);
void* FredEmmott_USBIP_VirtPP_Device_GetUserData(
  FredEmmott_USBIP_VirtPP_DeviceHandle);
FredEmmott_USBIP_VirtPP_InstanceHandle
FredEmmott_USBIP_VirtPP_Device_GetInstance(
  FredEmmott_USBIP_VirtPP_DeviceHandle);
void* FredEmmott_USBIP_VirtPP_Device_GetInstanceUserData(
  FredEmmott_USBIP_VirtPP_DeviceHandle);

/***** END *****/

#ifdef __cplusplus
}// extern "C"
#endif