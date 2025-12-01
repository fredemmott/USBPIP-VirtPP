// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <tuple>

namespace RequestType {
enum class Recipient : uint8_t {
  Device = 0,
  Interface = 1,
  Endpoint = 2,
};
enum class Type : uint8_t {
  Standard = 0,
  Class = 1,
  Vendor = 2,
};
enum class Direction : uint8_t {
  HostToDevice = 0,
  DeviceToHost = 1,
};

constexpr auto Parse(const uint8_t value) {
  return std::tuple {
    static_cast<Direction>(value >> 7),
    static_cast<Type>((value >> 5) & 0b11),
    static_cast<Recipient>(value & 0b11111),
  };
}
}// namespace RequestType