// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifdef __cplusplus
#include <cstdint>

extern "C" {
#else
#include <stdint.h>
#endif

struct FREDEMMOTT_USBIP_VirtPP_Instance;
typedef FREDEMMOTT_USBIP_VirtPP_Instance* FREDEMMOTT_USBIP_VirtPP_InstanceHandle;

struct FREDEMMOTT_USBIP_VirtPP_Device;
typedef const FREDEMMOTT_USBIP_VirtPP_Device* FREDEMMOTT_USBIP_VirtPP_DeviceHandle;

struct FREDEMMOTT_USBIP_VirtPP_Request;
typedef const FREDEMMOTT_USBIP_VirtPP_Request*
FREDEMMOTT_USBIP_VirtPP_RequestHandle;


void* FREDEMMOTT_USBIP_VirtPP_Device_GetUserData(
  FREDEMMOTT_USBIP_VirtPP_DeviceHandle);
FREDEMMOTT_USBIP_VirtPP_InstanceHandle FREDEMMOTT_USBIP_VirtPP_Device_GetInstance(
  FREDEMMOTT_USBIP_VirtPP_DeviceHandle);
void* FREDEMMOTT_USBIP_VirtPP_Device_GetInstanceUserData(
  FREDEMMOTT_USBIP_VirtPP_DeviceHandle);

typedef int32_t FREDEMMOTT_USBIP_VirtPP_Result;
#define FREDEMMOTT_USBIP_VirtPP_SUCCESS (0)
#define FREDEMMOTT_USBIP_VirtPP_SUCCEEDED(x) \
  ((x) == FREDEMMOTT_USBIP_VirtPP_SUCCESS)

/****** Instance:: types *****/

struct FREDEMMOTT_USBIP_VirtPP_Instance_Callbacks {
  void (*OnLogMessage)(int severity, const char* message, size_t messageLength);
};


struct FREDEMMOTT_USBIP_VirtPP_Instance_InitData {
  FREDEMMOTT_USBIP_VirtPP_Instance_Callbacks mCallbacks;
  void* mUserData;
  uint16_t mPortNumber;// set to zero to auto-assign
};

/****** Instance:: methods *****/

FREDEMMOTT_USBIP_VirtPP_InstanceHandle FREDEMMOTT_USBIP_VirtPP_Instance_Create(
  const FREDEMMOTT_USBIP_VirtPP_Instance_InitData*);
// Retrieve the actual port number being listened on. Useful for binding to portNumber 0
uint16_t FREDEMMOTT_USBIP_VirtPP_Instance_GetPortNumber(
  FREDEMMOTT_USBIP_VirtPP_InstanceHandle);
void* FREDEMMOTT_USBIP_VirtPP_Instance_GetUserData(
  FREDEMMOTT_USBIP_VirtPP_InstanceHandle);
void FREDEMMOTT_USBIP_VirtPP_Instance_Run(FREDEMMOTT_USBIP_VirtPP_InstanceHandle);
void FREDEMMOTT_USBIP_VirtPP_Instance_RequestStop(
  FREDEMMOTT_USBIP_VirtPP_InstanceHandle);
void FREDEMMOTT_USBIP_VirtPP_Instance_Destroy(
  FREDEMMOTT_USBIP_VirtPP_InstanceHandle);

/***** Device:: types *****/

struct FREDEMMOTT_USBIP_VirtPP_Device_Callbacks {
  FREDEMMOTT_USBIP_VirtPP_Result (*OnInputRequest)(
    FREDEMMOTT_USBIP_VirtPP_RequestHandle,
    uint32_t endpoint,
    uint8_t requestType,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length
    );
};

struct FREDEMMOTT_USBIP_VirtPP_Device_InterfaceConfig {
  uint8_t mClass;
  uint8_t mSubClass;
  uint8_t mProtocol;
};

struct FREDEMMOTT_USBIP_VirtPP_Device_DeviceConfig {
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

#define FREDEMMOTT_USBIP_VirtPP_LogSeverity_Debug (-16)
#define FREDEMMOTT_USBIP_VirtPP_LogSeverity_Default (0)
#define FREDEMMOTT_USBIP_VirtPP_LogSeverity_Error (16)

struct FREDEMMOTT_USBIP_VirtPP_Device_InitData {
  void* mUserData;
  bool mAutoAttach;
  FREDEMMOTT_USBIP_VirtPP_Device_Callbacks const* mCallbacks;
  FREDEMMOTT_USBIP_VirtPP_Device_DeviceConfig const* mDeviceConfig;
  FREDEMMOTT_USBIP_VirtPP_Device_InterfaceConfig const* mInterfaceConfigs;
};

/***** Device:: methods *****/

FREDEMMOTT_USBIP_VirtPP_DeviceHandle FREDEMMOTT_USBIP_VirtPP_Device_Create(
  FREDEMMOTT_USBIP_VirtPP_InstanceHandle,
  FREDEMMOTT_USBIP_VirtPP_Device_InitData const*);
FREDEMMOTT_USBIP_VirtPP_Result FREDEMMOTT_USBIP_VirtPP_Device_Attach(FREDEMMOTT_USBIP_VirtPP_DeviceHandle);
void FREDEMMOTT_USBIP_VirtPP_Device_Destroy(FREDEMMOTT_USBIP_VirtPP_DeviceHandle);

/****** Request:: methods *****/

FREDEMMOTT_USBIP_VirtPP_DeviceHandle FREDEMMOTT_USBIP_VirtPP_Request_GetDevice(
  FREDEMMOTT_USBIP_VirtPP_RequestHandle);
void* FREDEMMOTT_USBIP_VirtPP_Request_GetDeviceUserData(
  FREDEMMOTT_USBIP_VirtPP_RequestHandle);
FREDEMMOTT_USBIP_VirtPP_InstanceHandle FREDEMMOTT_USBIP_VirtPP_Request_GetInstance(
  FREDEMMOTT_USBIP_VirtPP_RequestHandle);
void* FREDEMMOTT_USBIP_VirtPP_Request_GetInstanceUserData(
  FREDEMMOTT_USBIP_VirtPP_RequestHandle);

/** If you're using C++, there's an overload that takes `const T& data`, and infers the size */
FREDEMMOTT_USBIP_VirtPP_Result FREDEMMOTT_USBIP_VirtPP_Request_SendReply(
  FREDEMMOTT_USBIP_VirtPP_RequestHandle,
  void const* data,
  size_t dataSize);


/***** END *****/

#ifdef __cplusplus
}// extern "C"
template <class T>
FREDEMMOTT_USBIP_VirtPP_Result FREDEMMOTT_USBIP_VirtPP_Request_SendReply(
  FREDEMMOTT_USBIP_VirtPP_RequestHandle handle,
  const T& data) {
  return FREDEMMOTT_USBIP_VirtPP_Request_SendReply(
    handle,
    &data,
    sizeof(T));
}
#endif