// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

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
// Use -32 (linux -EPIPE) for 'STALL', e.g. for bad USB string descriptor requests
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Request_SendErrorReply(
  FredEmmott_USBIP_VirtPP_RequestHandle,
  int32_t status);

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