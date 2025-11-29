// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#pragma pack(push, 1)
struct FredEmmott_HIDSpec_HIDDescriptor {
  uint8_t bLength;
  uint8_t bDescriptorType; /* Always 0x21 */
  uint16_t bcdHID;
  uint8_t bCountryCode;
  uint8_t bNumDescriptors;
};
struct FredEmmott_HIDSpec_HIDDescriptor_ReportDescriptor {
  uint8_t bDescriptorType; /* Always 0x22 */
  uint16_t wDescriptorLength;
};
#pragma pack(pop)