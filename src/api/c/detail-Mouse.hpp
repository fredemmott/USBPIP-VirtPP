// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <FredEmmott/USBIP-VirtPP/Mouse.h>
#include <FredEmmott/USBIP-VirtPP/HIDDevice.h>

struct FredEmmott_USBIP_VirtPP_Mouse final {
  FredEmmott_USBIP_VirtPP_Mouse() = delete;
  FredEmmott_USBIP_VirtPP_Mouse(
    FredEmmott_USBIP_VirtPP_InstanceHandle,
    const FredEmmott_USBIP_VirtPP_Mouse_InitData&);
  ~FredEmmott_USBIP_VirtPP_Mouse();

  void* mUserData {};

  FredEmmott_USBIP_VirtPP_HIDDeviceHandle mHID {};
  FredEmmott_USBIP_VirtPP_Mouse_State mState {};

  static FredEmmott_USBIP_VirtPP_Result OnGetInputReport(
    FredEmmott_USBIP_VirtPP_RequestHandle,
    uint8_t reportId,
    uint16_t expectedLength);
};
