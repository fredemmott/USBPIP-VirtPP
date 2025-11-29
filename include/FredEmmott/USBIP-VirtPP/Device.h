// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

/***** THIS FILE IS FOR IMPLEMENTING A RAW USB DEVICE ******
 *
 * You probably want HIDDevice.h instead.
 */

#include "Core.h"

#ifdef __cplusplus
#include <cstdint>

extern "C" {
#endif

struct FredEmmott_USBIP_VirtPP_Device;
typedef const FredEmmott_USBIP_VirtPP_Device*
FredEmmott_USBIP_VirtPP_DeviceHandle;

struct FredEmmott_USBIP_VirtPP_Request;
typedef const FredEmmott_USBIP_VirtPP_Request*
FredEmmott_USBIP_VirtPP_RequestHandle;

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

/****** Request:: methods *****/

FredEmmott_USBIP_VirtPP_DeviceHandle FredEmmott_USBIP_VirtPP_Request_GetDevice(
  FredEmmott_USBIP_VirtPP_RequestHandle);
void* FredEmmott_USBIP_VirtPP_Request_GetDeviceUserData(
  FredEmmott_USBIP_VirtPP_RequestHandle);
FredEmmott_USBIP_VirtPP_InstanceHandle
FredEmmott_USBIP_VirtPP_Request_GetInstance(
  FredEmmott_USBIP_VirtPP_RequestHandle);
void* FredEmmott_USBIP_VirtPP_Request_GetInstanceUserData(
  FredEmmott_USBIP_VirtPP_RequestHandle);

/** If you're using C++, there's an overload that takes `const T& data`, and infers the size */
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Request_SendReply(
  FredEmmott_USBIP_VirtPP_RequestHandle,
  void const* data,
  size_t dataSize);
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Request_SendStringReply(
  FredEmmott_USBIP_VirtPP_RequestHandle,
  wchar_t const* data,
  size_t charCount);

/***** END *****/

#ifdef __cplusplus
}// extern "C"
template <class T>
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Request_SendReply(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle,
  const T& data) {
  return FredEmmott_USBIP_VirtPP_Request_SendReply(
    handle,
    &data,
    sizeof(T));
}

template <size_t N>
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Request_SendStringReply(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle,
  wchar_t const (&data)[N]) {
  return FredEmmott_USBIP_VirtPP_Request_SendStringReply(handle, data, N - 1);
}
#endif