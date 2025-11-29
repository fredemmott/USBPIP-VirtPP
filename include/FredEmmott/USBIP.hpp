// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <utility>

#pragma pack(push, 1)
namespace FredEmmott::USBIP {
template <std::integral T>
class BigEndian {
public:
  BigEndian() = default;

  constexpr BigEndian(const T value)
    : mValue(MaybeSwap(value)) {
  }

  [[nodiscard]] constexpr T BigEndianValue() const { return mValue; }
  [[nodiscard]] constexpr T NativeValue() const { return MaybeSwap(mValue); }
  [[nodiscard]] constexpr T LittleEndianValue() const { return std::byteswap(mValue); }

  constexpr bool operator==(const T other) const {
    return NativeValue() == other;
  }
  constexpr bool operator==(const BigEndian<T>& other) const {
    return mValue == other.mValue;
  }
private:
  static constexpr T MaybeSwap(const T value) {
    if constexpr (std::endian::native == std::endian::big) {
      return value;
    } else {
      return std::byteswap(value);
    }
  }

  T mValue{};
};

using beu16_t = BigEndian<uint16_t>;
static_assert(sizeof(beu16_t) == 2);
using beu32_t = BigEndian<uint32_t>;
static_assert(sizeof(beu32_t) == 4);
using beu64_t = BigEndian<uint64_t>;
static_assert(sizeof(beu64_t) == 8);

using bei16_t = BigEndian<int16_t>;
static_assert(sizeof(bei16_t) == 2);
using bei32_t = BigEndian<int32_t>;
static_assert(sizeof(bei32_t) == 4);
using bei64_t = BigEndian<int64_t>;
static_assert(sizeof(bei64_t) == 8);

constexpr auto operator""_be16(const uint64_t value) {
  return BigEndian{static_cast<uint16_t>(value)}.BigEndianValue();;
}

constexpr auto operator""_be32(const uint64_t value) {
  return BigEndian{static_cast<uint32_t>(value)}.BigEndianValue();;
}

constexpr auto Version = 0x0111_be16;

enum class SetupCommandCode : uint16_t {
  OP_REQ_DEVLIST = 0x8005_be16,
  OP_REP_DEVLIST = 0x0005_be16,
  OP_REQ_IMPORT = 0x8003_be16,
  OP_REP_IMPORT = 0x0003_be16,
};

namespace detail {
constexpr uint32_t Get4ByteCommandCode(const SetupCommandCode code) {
  struct Packed {
    uint16_t mVersion{};
    uint16_t mCode{};
  };
  const Packed packed{Version, std::to_underlying(code)};
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
  USBIP_CMD_SUBMIT = 0x0000'0001_be32,
  USBIP_CMD_UNLINK = 0x0000'0002_be32,
  USBIP_RET_SUBMIT = 0x0000'0003_be32,
  USBIP_RET_UNLINK = 0x0000'0004_be32,
};

struct SetupHeader {
  uint16_t mUSBIPVersion{Version};
  SetupCommandCode mCommandCode{};
  beu32_t mStatus{};

  explicit SetupHeader(const SetupCommandCode code)
    : mCommandCode(code) {
  }
};

struct OP_REQ_DEVLIST {
  SetupHeader mHeader{SetupCommandCode::OP_REQ_DEVLIST};
};

enum class Speed : uint32_t {
  Unknown = 0,
  Low = 1_be32,
  Full = 2_be32,
  Wireless = 3_be32,
  Super = 4_be32,
  SuperPlus = 5_be32,
};

struct Device {
  char mPath[256]{};
  char mBusID[32]{};
  beu32_t mBusNum{};
  beu32_t mDevNum{};
  Speed mSpeed{};
  beu16_t mVendorID{};
  beu16_t mProductID{};
  beu16_t mDeviceVersion{};// `bcdDevice`, binary-coded decimal
  uint8_t mDeviceClass{};
  uint8_t mDeviceSubClass{};
  uint8_t mDeviceProtocol{};
  uint8_t mConfigurationValue{};
  uint8_t mNumConfigurations{};
  uint8_t mNumInterfaces{};
};

struct Interface {
  uint8_t mClass{};
  uint8_t mSubClass{};
  uint8_t mProtocol{};
  const uint8_t mPadding{};
};

struct OP_REP_DEVLIST {
  SetupHeader mHeader{SetupCommandCode::OP_REP_DEVLIST};
  beu32_t mNumDevices{};
};

struct OP_REQ_IMPORT {
  SetupHeader mHeader{SetupCommandCode::OP_REQ_IMPORT};
  char mBusID[32]{};
};

struct OP_REP_IMPORT {
  SetupHeader mHeader{SetupCommandCode::OP_REP_IMPORT};
  Device mDevice{};
};

enum class Direction : uint32_t {
  Out = 0,
  In = 1_be32,
};

struct BasicHeader {
  CommandCode mCommandCode{};
  uint32_t mSequenceNumber{};
  beu32_t mDeviceID{};
  Direction mDirection{};
  beu32_t mEndpoint{};
};

struct USBIP_CMD_SUBMIT {
  struct Setup {
    uint8_t mRequestType{};
    uint8_t mRequest{};
    uint16_t mValue{};
    uint16_t mIndex{};
    uint16_t mLength{};
  };

  BasicHeader mHeader{CommandCode::USBIP_CMD_SUBMIT};
  beu32_t mTransferFlags{};
  beu32_t mTransferBufferLength{};
  beu32_t mStartFrame{};
  beu32_t mNumberOfPackets{};
  beu32_t mInterval{};
  Setup mSetup{};
};

struct USBIP_RET_SUBMIT {
  BasicHeader mHeader{CommandCode::USBIP_RET_SUBMIT};
  bei32_t mStatus{};
  beu32_t mActualLength{};
  beu32_t mStartFrame{};
  beu32_t mNumberOfPackets{0xffff'ffff};// magic value for non-ISO transfers
  beu32_t mErrorCount{};
  const char mPadding[8]{};
};

struct USBIP_CMD_UNLINK {
  BasicHeader mHeader{CommandCode::USBIP_CMD_UNLINK};
  beu32_t mUnlinkSequenceNumber{};
  const char mPadding[24]{};
};

struct USBIP_RET_UNLINK {
  BasicHeader mHeader{CommandCode::USBIP_RET_UNLINK};
  beu32_t mStatus{};
  const char mPadding[24]{};
};
}// namespace FredEmmott::USBIP

#pragma pack(pop)