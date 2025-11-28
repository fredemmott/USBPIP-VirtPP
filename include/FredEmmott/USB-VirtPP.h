// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifdef __cplusplus
#include <cstdint>

extern "C" {
#else
#include <stdint.h>
#endif

struct FREDEMMOTT_USB_VIRTPP_Instance;
typedef FREDEMMOTT_USB_VIRTPP_Instance* FREDEMMOTT_USB_VIRTPP_InstanceHandle;

struct FREDEMMOTT_USB_VIRTPP_Device;
typedef const FREDEMMOTT_USB_VIRTPP_Device* FREDEMMOTT_USB_VIRTPP_DeviceHandle;

struct FREDEMMOTT_USB_VIRTPP_Request;
typedef const FREDEMMOTT_USB_VIRTPP_Request*
FREDEMMOTT_USB_VIRTPP_RequestHandle;


void* FREDEMMOTT_USB_VIRTPP_Device_GetUserData(
  FREDEMMOTT_USB_VIRTPP_DeviceHandle);
FREDEMMOTT_USB_VIRTPP_InstanceHandle FREDEMMOTT_USB_VIRTPP_Device_GetInstance(
  FREDEMMOTT_USB_VIRTPP_DeviceHandle);
void* FREDEMMOTT_USB_VIRTPP_Device_GetInstanceUserData(
  FREDEMMOTT_USB_VIRTPP_DeviceHandle);

typedef int32_t FREDEMMOTT_USB_VIRTPP_Result;
#define FREDEMMOTT_USB_VIRTPP_SUCCESS (0)
#define FREDEMMOTT_USB_VIRTPP_SUCCEEDED(x) \
  ((x) == FREDEMMOTT_USB_VIRTPP_SUCCESS)

/****** Instance:: types *****/

struct FREDEMMOTT_USB_VIRTPP_Instance_Callbacks {
  void (*OnLogMessage)(int severity, const char* message, size_t messageLength);
};


struct FREDEMMOTT_USB_VIRTPP_Instance_InitData {
  FREDEMMOTT_USB_VIRTPP_Instance_Callbacks mCallbacks;
  void* mUserData;
  uint16_t mPortNumber;// set to zero to auto-assign
};

/****** Instance:: methods *****/

FREDEMMOTT_USB_VIRTPP_InstanceHandle FREDEMMOTT_USB_VIRTPP_Instance_Create(
  const FREDEMMOTT_USB_VIRTPP_Instance_InitData*);
// Retrieve the actual port number being listened on. Useful for binding to portNumber 0
uint16_t FREDEMMOTT_USB_VIRTPP_Instance_GetPortNumber(
  FREDEMMOTT_USB_VIRTPP_InstanceHandle);
void* FREDEMMOTT_USB_VIRTPP_Instance_GetUserData(
  FREDEMMOTT_USB_VIRTPP_InstanceHandle);
void FREDEMMOTT_USB_VIRTPP_Instance_Run(FREDEMMOTT_USB_VIRTPP_InstanceHandle);
void FREDEMMOTT_USB_VIRTPP_Instance_RequestStop(
  FREDEMMOTT_USB_VIRTPP_InstanceHandle);
void FREDEMMOTT_USB_VIRTPP_Instance_Destroy(
  FREDEMMOTT_USB_VIRTPP_InstanceHandle);

/***** Device:: types *****/

struct FREDEMMOTT_USB_VIRTPP_Device_Callbacks {
  FREDEMMOTT_USB_VIRTPP_Result (*OnInputRequest)(
    FREDEMMOTT_USB_VIRTPP_RequestHandle,
    uint32_t endpoint,
    uint8_t requestType,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length
    );
};

struct FREDEMMOTT_USB_VIRTPP_Device_InterfaceConfig {
  uint8_t mClass;
  uint8_t mSubClass;
  uint8_t mProtocol;
};

struct FREDEMMOTT_USB_VIRTPP_Device_DeviceConfig {
  uint16_t mVendorID;
  uint16_t mProductID;
  uint16_t mDeviceVersion;/* BCD */
  uint8_t mDeviceClass;
  uint8_t mDeviceSubClass;
  uint8_t mDeviceProtocol;
  uint8_t mConfigurationValue;
  uint8_t mNumConfigurations;
  uint8_t mNumInterfaces;
};

#define FREDEMMOTT_USB_VIRTPP_LogSeverity_Debug (-16)
#define FREDEMMOTT_USB_VIRTPP_LogSeverity_Default (0)
#define FREDEMMOTT_USB_VIRTPP_LogSeverity_Error (16)

struct FREDEMMOTT_USB_VIRTPP_Device_InitData {
  void* mUserData;
  FREDEMMOTT_USB_VIRTPP_Device_Callbacks const* mCallbacks;
  FREDEMMOTT_USB_VIRTPP_Device_DeviceConfig const* mDeviceConfig;
  FREDEMMOTT_USB_VIRTPP_Device_InterfaceConfig const* mInterfaceConfigs;
};

/***** Device:: methods *****/

FREDEMMOTT_USB_VIRTPP_DeviceHandle FREDEMMOTT_USB_VIRTPP_Device_Create(
  FREDEMMOTT_USB_VIRTPP_InstanceHandle,
  FREDEMMOTT_USB_VIRTPP_Device_InitData const*);

void FREDEMMOTT_USB_VIRTPP_Device_Destroy(FREDEMMOTT_USB_VIRTPP_DeviceHandle);

/****** Request:: methods *****/

FREDEMMOTT_USB_VIRTPP_DeviceHandle FREDEMMOTT_USB_VIRTPP_Request_GetDevice(
  FREDEMMOTT_USB_VIRTPP_RequestHandle);
void* FREDEMMOTT_USB_VIRTPP_Request_GetDeviceUserData(
  FREDEMMOTT_USB_VIRTPP_RequestHandle);
FREDEMMOTT_USB_VIRTPP_InstanceHandle FREDEMMOTT_USB_VIRTPP_Request_GetInstance(
  FREDEMMOTT_USB_VIRTPP_RequestHandle);
void* FREDEMMOTT_USB_VIRTPP_Request_GetInstanceUserData(
  FREDEMMOTT_USB_VIRTPP_RequestHandle);

/** If you're using C++, there's an overload that takes `const T& data`, and infers the size */
FREDEMMOTT_USB_VIRTPP_Result FREDEMMOTT_USB_VIRTPP_Request_SendReply(
  FREDEMMOTT_USB_VIRTPP_RequestHandle,
  uint32_t status,
  void const* data,
  size_t dataSize);


/***** END *****/

#ifdef __cplusplus
}// extern "C"
template <class T>
FREDEMMOTT_USB_VIRTPP_Result FREDEMMOTT_USB_VIRTPP_Request_SendReply(
  FREDEMMOTT_USB_VIRTPP_RequestHandle handle,
  const uint32_t status,
  const T& data) {
  return FREDEMMOTT_USB_VIRTPP_Request_SendReply(
    handle,
    status,
    &data,
    sizeof(T));
}
#endif