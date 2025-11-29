// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include "USBSpec.h"

#ifdef __cplusplus
#include <cstdint>

extern "C" {
#else
#include <stdint.h>
#endif

struct FredEmmott_USBIP_VirtPP_Instance;
typedef FredEmmott_USBIP_VirtPP_Instance* FredEmmott_USBIP_VirtPP_InstanceHandle;

struct FredEmmott_USBIP_VirtPP_Device;
typedef const FredEmmott_USBIP_VirtPP_Device* FredEmmott_USBIP_VirtPP_DeviceHandle;

struct FredEmmott_USBIP_VirtPP_Request;
typedef const FredEmmott_USBIP_VirtPP_Request*
FredEmmott_USBIP_VirtPP_RequestHandle;

typedef int32_t FredEmmott_USBIP_VirtPP_Result;
#define FredEmmott_USBIP_VirtPP_SUCCESS (0)
#define FredEmmott_USBIP_VirtPP_SUCCEEDED(x) \
  ((x) == FredEmmott_USBIP_VirtPP_SUCCESS)

/****** Instance:: types *****/

struct FredEmmott_USBIP_VirtPP_Instance_Callbacks {
  void (*OnLogMessage)(int severity, const char* message, size_t messageLength);
};


struct FredEmmott_USBIP_VirtPP_Instance_InitData {
  void* mUserData;
  FredEmmott_USBIP_VirtPP_Instance_Callbacks mCallbacks;

  uint16_t mPortNumber;// set to zero to auto-assign
};

/****** Instance:: methods *****/

FredEmmott_USBIP_VirtPP_InstanceHandle FredEmmott_USBIP_VirtPP_Instance_Create(
  const FredEmmott_USBIP_VirtPP_Instance_InitData*);
// Retrieve the actual port number being listened on. Useful for binding to portNumber 0
uint16_t FredEmmott_USBIP_VirtPP_Instance_GetPortNumber(
  FredEmmott_USBIP_VirtPP_InstanceHandle);
void* FredEmmott_USBIP_VirtPP_Instance_GetUserData(
  FredEmmott_USBIP_VirtPP_InstanceHandle);
void FredEmmott_USBIP_VirtPP_Instance_Run(FredEmmott_USBIP_VirtPP_InstanceHandle);
void FredEmmott_USBIP_VirtPP_Instance_RequestStop(
  FredEmmott_USBIP_VirtPP_InstanceHandle);
void FredEmmott_USBIP_VirtPP_Instance_Destroy(
  FredEmmott_USBIP_VirtPP_InstanceHandle);

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

#define FredEmmott_USBIP_VirtPP_LogSeverity_Debug (-16)
#define FredEmmott_USBIP_VirtPP_LogSeverity_Default (0)
#define FredEmmott_USBIP_VirtPP_LogSeverity_Error (16)

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
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Device_Attach(FredEmmott_USBIP_VirtPP_DeviceHandle);
void FredEmmott_USBIP_VirtPP_Device_Destroy(FredEmmott_USBIP_VirtPP_DeviceHandle);
void* FredEmmott_USBIP_VirtPP_Device_GetUserData(
  FredEmmott_USBIP_VirtPP_DeviceHandle);
FredEmmott_USBIP_VirtPP_InstanceHandle FredEmmott_USBIP_VirtPP_Device_GetInstance(
  FredEmmott_USBIP_VirtPP_DeviceHandle);
void* FredEmmott_USBIP_VirtPP_Device_GetInstanceUserData(
  FredEmmott_USBIP_VirtPP_DeviceHandle);

/****** Request:: methods *****/

FredEmmott_USBIP_VirtPP_DeviceHandle FredEmmott_USBIP_VirtPP_Request_GetDevice(
  FredEmmott_USBIP_VirtPP_RequestHandle);
void* FredEmmott_USBIP_VirtPP_Request_GetDeviceUserData(
  FredEmmott_USBIP_VirtPP_RequestHandle);
FredEmmott_USBIP_VirtPP_InstanceHandle FredEmmott_USBIP_VirtPP_Request_GetInstance(
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

template<size_t N>
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Request_SendStringReply(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle,
  wchar_t const (&data)[N]) {
  return FredEmmott_USBIP_VirtPP_Request_SendStringReply(handle, data, N - 1);
}
#endif