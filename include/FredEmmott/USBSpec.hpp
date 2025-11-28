// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>

namespace FredEmmott::USBSpec {
#pragma pack(push, 1)
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
