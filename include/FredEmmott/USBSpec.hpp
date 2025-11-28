// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>

namespace FredEmmott::USBSpec {
#pragma pack(push, 1)
struct ConfigurationDescriptor {
  const uint8_t mLength {sizeof(ConfigurationDescriptor)};
  const uint8_t mDescriptorType {0x02};// CONFIGURATION
  uint16_t mTotalLength {};
  uint8_t mNumInterfaces {};
  uint8_t mConfigurationValue {};
  uint8_t mConfiguration {};
  uint8_t mAttributes {};
  uint8_t mMaxPower {};
};

struct DeviceDescriptor {
  const uint8_t mLength {sizeof(DeviceDescriptor)};
  const uint8_t mDescriptorType {0x01};// DEVICE
  uint16_t mUSBVersion {};
  uint8_t mClass {};
  uint8_t mSubClass {};
  uint8_t mProtocol {};
  uint8_t mMaxPacketSize0 {};
  uint16_t mVendorID {};
  uint16_t mProductID {};
  uint16_t mDeviceVersion {};
  uint8_t mManufacturerStringIndex {};
  uint8_t mProductStringIndex {};
  uint8_t mSerialNumberStringIndex {};
  uint8_t mNumConfigurations {};
};

struct InterfaceDescriptor {
  const uint8_t mLength {sizeof(InterfaceDescriptor)};
  const uint8_t mDescriptorType {0x04};// INTERFACE
  uint8_t mInterfaceNumber {};
  uint8_t mAlternateSetting {};
  uint8_t mNumEndpoints {};
  uint8_t mClass {};
  uint8_t mSubClass {};
  uint8_t mProtocol {};
  uint8_t mInterfaceStringIndex {};
};

struct EndpointDescriptor {
  const uint8_t mLength {sizeof(EndpointDescriptor)};
  const uint8_t mDescriptorType {0x05};// ENDPOINT
  uint8_t mEndpointAddress {};
  uint8_t mAttributes {};
  uint16_t mMaxPacketSize {};
  uint8_t mInterval {};
};

template <std::size_t N>
struct StringDescriptor {
  struct Header {
    const uint8_t mLength {sizeof(Header) + (2 * (N - 1))};
    const uint8_t mDescriptorType {0x03};// STRING
  } mHeader;

  wchar_t mData[N - 1];

  StringDescriptor() = delete;

  consteval explicit StringDescriptor(const wchar_t (&str)[N]) {
    std::copy(str, str + (N - 1), mData);
  }
};

struct HIDDescriptor {
  const uint8_t mLength {sizeof(HIDDescriptor)};
  const uint8_t mDescriptorType {0x21};// HID
  uint16_t mHIDSpecVersion {};
  uint8_t mCountryCode {};
  uint8_t mNumDescriptors {};

  struct Report {
    const uint8_t mType {0x22};// HIDReport
    uint16_t mLength;
  };
};
#pragma pack(pop)
}// namespace FredEmmott::USBSpec
