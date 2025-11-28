// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <utility>

#pragma pack(push, 1)
namespace FredEmmott::USBIP {
namespace ByteOrder {
template <std::unsigned_integral T>
constexpr T to_network(const T value) {
  if constexpr (std::endian::native == std::endian::big) {
    return value;
  } else {
    return std::byteswap(value);
  }
}

template <std::unsigned_integral T>
constexpr T from_network(const T value) {
  // Name is only for clarity, they're either both no-ops, or both byteswaps
  return to_network(value);
}
}// namespace ByteOrder

constexpr auto Version = ByteOrder::to_network<uint16_t>(0x0111);

enum class SetupCommandCode : uint16_t {
  OP_REQ_DEVLIST = ByteOrder::to_network<uint16_t>(0x8005),
  OP_REP_DEVLIST = ByteOrder::to_network<uint16_t>(0x0005),
  OP_REQ_IMPORT = ByteOrder::to_network<uint16_t>(0x8003),
  OP_REP_IMPORT = ByteOrder::to_network<uint16_t>(0x0003),
};

namespace detail {
constexpr uint32_t Get4ByteCommandCode(const SetupCommandCode code) {
  struct Packed {
    uint16_t mVersion {};
    uint16_t mCode {};
  };
  const Packed packed {Version, std::to_underlying(code)};
  return std::bit_cast<uint32_t>(packed);
}
}// namespace detail

enum class CommandCode : uint32_t {
  OP_REQ_DEVLIST
  = detail::Get4ByteCommandCode(SetupCommandCode::OP_REQ_DEVLIST),
  OP_REP_DEVLIST
  = detail::Get4ByteCommandCode(SetupCommandCode::OP_REP_DEVLIST),
  OP_REQ_IMPORT = detail::Get4ByteCommandCode(SetupCommandCode::OP_REQ_IMPORT),
  OP_REP_IMPORT = detail::Get4ByteCommandCode(SetupCommandCode::OP_REP_IMPORT),
  USBIP_CMD_SUBMIT = ByteOrder::to_network<uint32_t>(0x0000'0001),
  USBIP_CMD_UNLINK = ByteOrder::to_network<uint32_t>(0x0000'0002),
  USBIP_RET_SUBMIT = ByteOrder::to_network<uint32_t>(0x0000'0003),
  USBIP_RET_UNLINK = ByteOrder::to_network<uint32_t>(0x0000'0004),
};

struct SetupHeader {
  uint16_t mUSBIPVersion {Version};
  SetupCommandCode mCommandCode {};
  uint32_t mStatus {};

  explicit SetupHeader(const SetupCommandCode code) : mCommandCode(code) {
  }
};

struct OP_REQ_DEVLIST {
  SetupHeader mHeader {SetupCommandCode::OP_REQ_DEVLIST};
};

enum class Speed : uint32_t {
  Unknown = 0,
  Low = ByteOrder::to_network<uint32_t>(1),
  Full = ByteOrder::to_network<uint32_t>(2),
  Wireless = ByteOrder::to_network<uint32_t>(3),
  Super = ByteOrder::to_network<uint32_t>(4),
  SuperPlus = ByteOrder::to_network<uint32_t>(5),
};

struct Device {
  char mPath[256] {};
  char mBusID[32] {};
  uint32_t mBusNum {};
  uint32_t mDevNum {};
  Speed mSpeed {};
  uint16_t mVendorID {};
  uint16_t mProductID {};
  uint16_t mDeviceVersion {};// `bcdDevice`, binary-coded decimal
  uint8_t mDeviceClass {};
  uint8_t mDeviceSubClass {};
  uint8_t mDeviceProtocol {};
  uint8_t mConfigurationValue {};
  uint8_t mNumConfigurations {};
  uint8_t mNumInterfaces {};
};

struct Interface {
  uint8_t mClass {};
  uint8_t mSubClass {};
  uint8_t mProtocol {};
  const uint8_t mPadding {};
};

struct OP_REP_DEVLIST {
  SetupHeader mHeader {SetupCommandCode::OP_REP_DEVLIST};
  uint32_t mNumDevices {};
};

struct OP_REQ_IMPORT {
  SetupHeader mHeader {SetupCommandCode::OP_REQ_IMPORT};
  char mBusID[32] {};
};

struct OP_REP_IMPORT {
  const SetupHeader mHeader {SetupCommandCode::OP_REP_IMPORT};
  Device mDevice {};
};

enum class Direction : uint32_t {
  Out = 0,
  In = ByteOrder::to_network<uint32_t>(1),
};

struct BasicHeader {
  CommandCode mCommandCode {};
  uint32_t mSequenceNumber {};
  uint32_t mDeviceID {};
  Direction mDirection {};
  uint32_t mEndpoint {};
};

struct USBIP_CMD_SUBMIT {
  struct Setup {
    uint8_t mRequestType {};
    uint8_t mRequest {};
    uint16_t mValue {};
    uint16_t mIndex {};
    uint16_t mLength {};
  };

  BasicHeader mHeader {CommandCode::USBIP_CMD_SUBMIT};
  uint32_t mTransferFlags {};
  uint32_t mTransferBufferLength {};
  uint32_t mStartFrame {};
  uint32_t mNumberOfPackets {};
  uint32_t mInterval {};
  Setup mSetup {};
};

struct USBIP_RET_SUBMIT {
  BasicHeader mHeader {CommandCode::USBIP_RET_SUBMIT};
  uint32_t mStatus {};
  uint32_t mActualLength {};
  uint32_t mStartFrame {};
  uint32_t mNumberOfPackets {};
  uint32_t mErrorCount {};
  const char mPadding[8] {};
};

struct USBIP_CMD_UNLINK {
  BasicHeader mHeader {CommandCode::USBIP_CMD_UNLINK};
  uint32_t mUnlinkSequenceNumber {};
  const char mPadding[24] {};
};

struct USBIP_RET_UNLINK {
  BasicHeader mHeader {CommandCode::USBIP_RET_UNLINK};
  uint32_t mStatus {};
  const char mPadding[24] {};
};
}// namespace FredEmmott::USBIP

#pragma pack(pop)
