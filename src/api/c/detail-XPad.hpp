// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include "guarded_data.hpp"
#include "handles.hpp"

#include <FredEmmott/USBIP-VirtPP/XPad.h>
#include <FredEmmott/USBSpec.h>

#include <mutex>
#include <queue>
#include <string_view>

struct FredEmmott_USBIP_VirtPP_XPad final {
  FredEmmott_USBIP_VirtPP_XPad() = delete;
  FredEmmott_USBIP_VirtPP_XPad(
    FredEmmott_USBIP_VirtPP_InstanceHandle,
    const FredEmmott_USBIP_VirtPP_XPad_InitData&);
  ~FredEmmott_USBIP_VirtPP_XPad();
  void* mUserData {};
  FredEmmott_USBIP_VirtPP_DeviceHandle mUSBDevice {};

  FredEmmott_USBIP_VirtPP_Result UpdateInPlace(
    void* userData,
    void (*callback)(
      FredEmmott_USBIP_VirtPP_XPadHandle,
      void* userData,
      FredEmmott_USBIP_VirtPP_XPad_State*));

 private:
  struct ConfigurationDescriptor;
  static const FredEmmott_USBSpec_DeviceDescriptor& GetDeviceDescriptor();
  static const ConfigurationDescriptor& GetConfigurationDescriptor();
#pragma pack(push, 1)
  struct GamepadInputReport {
    const uint8_t bReportID {0x00};
    const uint8_t bSize {sizeof(GamepadInputReport)};
    FredEmmott_USBIP_VirtPP_XPad_State mState {};
    const uint8_t mPadding[6] {};
  };
  static_assert(sizeof(GamepadInputReport) == 0x14);
  template <uint8_t TReportID, uint8_t TDefault = 0>
  struct GamepadStatusReport {
    static constexpr uint8_t ReportID {TReportID};
    const uint8_t bReportID {ReportID};
    const uint8_t bSize {sizeof(GamepadStatusReport)};
    uint8_t mState {TDefault};
  };
  using GamepadLEDStatusReport = GamepadStatusReport<0x1>;
  using GamepadRumbleLevelStatusReport = GamepadStatusReport<0x03>;
  struct GamepadRumbleMotorControlReport {
    static constexpr uint8_t ReportID {0x00};
    const uint8_t bReportID {ReportID};
    const uint8_t bSize {sizeof(GamepadRumbleMotorControlReport)};
    const uint8_t bType {};
    uint8_t bBigMotorMagnitude {};
    uint8_t bSmallMotorMagnitude {};
    uint8_t mReserved[3] {};
  };
  using GamepadLEDControlReport = GamepadStatusReport<0x01>;
  using GamepadRumbleLevelControlReport = GamepadStatusReport<0x02, 0x03>;

  struct XUSBInputReport {
    GamepadInputReport mGamepadInputReport {};
    GamepadLEDStatusReport mGamepadLEDStatusReport {};
    GamepadRumbleLevelStatusReport mGamepadRumbleLevelStatusReport {};
  };
  // OS always requests 32 bytes, regardless of XUSB descriptor
  static_assert(sizeof(XUSBInputReport) <= 32);
#pragma pack(pop)

  uint32_t mSerialNumber {};


  FredEmmott_USBIP_VirtPP_InstanceHandle mInstance {};
  XUSBInputReport mXUSBReport {};
  FredEmmott_USBIP_VirtPP_XPad_Callbacks mCallbacks{};

  guarded_data<std::queue<FredEmmott::USBVirtPP::unique_request>>
    mGamepadInputQueue;

  FredEmmott_USBIP_VirtPP_Result OnControlInputRequest(
    FredEmmott_USBIP_VirtPP_RequestHandle request,
    uint8_t rawRequestType,
    uint8_t requestCode,
    uint16_t value,
    uint16_t index,
    uint16_t length);
  FredEmmott_USBIP_VirtPP_Result OnControlOutputRequest(
    FredEmmott_USBIP_VirtPP_RequestHandle request,
    uint8_t rawRequestType,
    uint8_t requestCode,
    uint16_t value,
    uint16_t index,
    uint16_t length,
    const void* data,
    uint32_t dataLength);

  FredEmmott_USBIP_VirtPP_Result OnGamepadInputRequest(
    FredEmmott_USBIP_VirtPP_RequestHandle request,
    uint8_t rawRequestType,
    uint8_t requestCode,
    uint16_t value,
    uint16_t index,
    uint16_t length);
  FredEmmott_USBIP_VirtPP_Result OnGamepadOutputRequest(
    FredEmmott_USBIP_VirtPP_RequestHandle request,
    uint8_t rawRequestType,
    uint8_t requestCode,
    uint16_t value,
    uint16_t index,
    uint16_t length,
    const void* data,
    uint32_t dataLength);

  static FredEmmott_USBIP_VirtPP_Result OnUSBInputRequestCallback(
    FredEmmott_USBIP_VirtPP_RequestHandle request,
    uint32_t endpoint,
    uint8_t requestType,
    uint8_t requestCode,
    uint16_t value,
    uint16_t index,
    uint16_t length);
  static FredEmmott_USBIP_VirtPP_Result OnUSBOutputRequestCallback(
    FredEmmott_USBIP_VirtPP_RequestHandle,
    uint32_t endpoint,
    uint8_t requestType,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length,
    const void* data,
    uint32_t dataLength);
};