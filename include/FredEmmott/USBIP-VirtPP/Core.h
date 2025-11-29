// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <FredEmmott/USBSpec.h>

#ifdef __cplusplus
#include <cstdint>

#define FredEmmott_USBIP_VirtPP_LogSeverity_Debug (-16)
#define FredEmmott_USBIP_VirtPP_LogSeverity_Default (0)
#define FredEmmott_USBIP_VirtPP_LogSeverity_Error (16)

extern "C" {
#else
#include <stdint.h>
#endif

typedef int32_t FredEmmott_USBIP_VirtPP_Result;
#define FredEmmott_USBIP_VirtPP_SUCCESS (0)
#define FredEmmott_USBIP_VirtPP_SUCCEEDED(x) \
  ((x) == FredEmmott_USBIP_VirtPP_SUCCESS)

/****** Instance:: types *****/

struct FredEmmott_USBIP_VirtPP_Instance;
typedef FredEmmott_USBIP_VirtPP_Instance*
FredEmmott_USBIP_VirtPP_InstanceHandle;

struct FredEmmott_USBIP_VirtPP_Instance_Callbacks {
  void (*OnLogMessage)(int severity, const char* message, size_t messageLength);
};


struct FredEmmott_USBIP_VirtPP_Instance_InitData {
  void* mUserData;
  FredEmmott_USBIP_VirtPP_Instance_Callbacks mCallbacks;

  uint16_t mPortNumber;// set to zero to auto-assign
  bool mAllowRemoteConnections;
};

/****** Instance:: methods *****/

FredEmmott_USBIP_VirtPP_InstanceHandle FredEmmott_USBIP_VirtPP_Instance_Create(
  const FredEmmott_USBIP_VirtPP_Instance_InitData*);
// Retrieve the actual port number being listened on. Useful for binding to portNumber 0
uint16_t FredEmmott_USBIP_VirtPP_Instance_GetPortNumber(
  FredEmmott_USBIP_VirtPP_InstanceHandle);
void* FredEmmott_USBIP_VirtPP_Instance_GetUserData(
  FredEmmott_USBIP_VirtPP_InstanceHandle);
void FredEmmott_USBIP_VirtPP_Instance_Run(
  FredEmmott_USBIP_VirtPP_InstanceHandle);
void FredEmmott_USBIP_VirtPP_Instance_RequestStop(
  FredEmmott_USBIP_VirtPP_InstanceHandle);
void FredEmmott_USBIP_VirtPP_Instance_Destroy(
  FredEmmott_USBIP_VirtPP_InstanceHandle);

/***** END *****/

#ifdef __cplusplus
}// extern "C"
#endif