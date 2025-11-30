// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include "Core.h"
#include "Request.h"

#ifdef __cplusplus
#include <cinttypes>

extern "C" {
#else
#include <inttypes.h>
#endif

struct FredEmmott_USBIP_VirtPP_HIDDevice;
typedef FredEmmott_USBIP_VirtPP_HIDDevice*
FredEmmott_USBIP_VirtPP_HIDDeviceHandle;

struct FredEmmott_USBIP_VirtPP_HIDDevice_USBDeviceData {
  uint16_t mVendorID;
  uint16_t mProductID;
  uint16_t mDeviceVersion;/* BCD */
  wchar_t mLanguage[64];/* L"\x0409" for en_US */
  wchar_t mManufacturer[64];
  wchar_t mProduct[64];
  wchar_t mInterface[64];
  wchar_t mSerialNumber[64];
};

struct FredEmmott_USBIP_VirtPP_HIDDevice_Callbacks {
  FredEmmott_USBIP_VirtPP_Result (*OnGetInputReport)(
    FredEmmott_USBIP_VirtPP_RequestHandle,
    uint8_t reportId,
    uint16_t expectedLength);

  /* Called when the host sends an OUTPUT report via a class SET_REPORT
   * request with report type OUTPUT.
   *
   * Note: current transport only exposes control phase metadata; data-stage
   * payload delivery may be limited depending on backend implementation.
   */
  FredEmmott_USBIP_VirtPP_Result (*OnSetOutputReport)(
    FredEmmott_USBIP_VirtPP_RequestHandle,
    uint8_t reportId,
    const void* data,
    size_t dataSize);
};

struct FredEmmott_USBIP_VirtPP_HIDDevice_InitData {
  void* mUserData;
  FredEmmott_USBIP_VirtPP_HIDDevice_Callbacks mCallbacks;

  bool mAutoAttach;
  FredEmmott_USBIP_VirtPP_HIDDevice_USBDeviceData mUSBDeviceData;
  uint8_t mReportCount;
  FredEmmott_USBIP_VirtPP_BlobReference mReportDescriptors[1];
};

FredEmmott_USBIP_VirtPP_HIDDeviceHandle
FredEmmott_USBIP_VirtPP_HIDDevice_Create(
  FredEmmott_USBIP_VirtPP_InstanceHandle,
  const FredEmmott_USBIP_VirtPP_HIDDevice_InitData*);
void FredEmmott_USBIP_VirtPP_HIDDevice_Destroy(
  FredEmmott_USBIP_VirtPP_HIDDeviceHandle);

void* FredEmmott_USBIP_VirtPP_HIDDevice_GetUserData(
  FredEmmott_USBIP_VirtPP_HIDDeviceHandle);

FredEmmott_USBIP_VirtPP_DeviceHandle FredEmmott_USBIP_VirtPP_HIDDevice_GetUSBDevice(
  FredEmmott_USBIP_VirtPP_HIDDeviceHandle);

FredEmmott_USBIP_VirtPP_InstanceHandle FredEmmott_USBIP_VirtPP_HIDDevice_GetInstance(
  FredEmmott_USBIP_VirtPP_HIDDeviceHandle);
void* FredEmmott_USBIP_VirtPP_HIDDevice_GetInstanceUserData(
  FredEmmott_USBIP_VirtPP_HIDDeviceHandle);

FredEmmott_USBIP_VirtPP_HIDDeviceHandle FredEmmott_USBIP_VirtPP_Request_GetHIDDevice(
  FredEmmott_USBIP_VirtPP_RequestHandle);
void* FredEmmott_USBIP_VirtPP_Request_GetHIDDeviceUserData(
  FredEmmott_USBIP_VirtPP_RequestHandle);

#ifdef __cplusplus
}
#endif