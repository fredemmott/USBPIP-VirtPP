// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>

namespace FredEmmott::USBSpec {
#pragma pack(push, 1)
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
