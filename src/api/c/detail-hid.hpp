// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <FredEmmott/USBIP-VirtPP/HIDDevice.h>

#include <cstddef>
#include <vector>
#include <tuple>

struct FredEmmott_USBIP_VirtPP_HIDDevice final {
  FredEmmott_USBIP_VirtPP_HIDDevice() = delete;
  FredEmmott_USBIP_VirtPP_HIDDevice(
    FredEmmott_USBIP_VirtPP_InstanceHandle,
    const FredEmmott_USBIP_VirtPP_HIDDevice_InitData&
    );
  ~FredEmmott_USBIP_VirtPP_HIDDevice();

  // Shallow copy
  FredEmmott_USBIP_VirtPP_HIDDevice_InitData mInit{};

  FredEmmott_USBIP_VirtPP_InstanceHandle mInstance {};
  FredEmmott_USBIP_VirtPP_DeviceHandle mUSBDevice{};

  FredEmmott_USBSpec_DeviceDescriptor mDeviceDescriptor{};
  std::vector<std::byte> mConfigurationDescriptorBlob{};
  FredEmmott_USBSpec_InterfaceDescriptor* mInterfaceDescriptors{};
  FredEmmott_USBSpec_ConfigurationDescriptor* mConfigurationDescriptor{};

  std::vector<std::tuple<const void*, uint16_t>> mHIDReportDescriptors{};
private:
  void InitializeDescriptors();
  void InitializeDeviceDescriptor();

  HRESULT OnUSBInputRequest(
    FredEmmott_USBIP_VirtPP_RequestHandle request,
    uint32_t endpoint,
    uint8_t requestType,
    uint8_t requestCode,
    uint16_t value,
    uint16_t index,
    uint16_t length);

  static FredEmmott_USBIP_VirtPP_Result OnUSBInputRequestCallback(
    FredEmmott_USBIP_VirtPP_RequestHandle request,
    uint32_t endpoint,
    uint8_t requestType,
    uint8_t requestCode,
    uint16_t value,
    uint16_t index,
    uint16_t length);
};
