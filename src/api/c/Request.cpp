// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "detail.hpp"
#include "send-recv.hpp"

#include <FredEmmott/USBIP-VirtPP.h>
#include <FredEmmott/USBIP.hpp>

#include <algorithm>
#include <ranges>

namespace USBIP = FredEmmott::USBIP;
using namespace FredEmmott::USBVirtPP;

FredEmmott_USBIP_VirtPP_InstanceHandle FredEmmott_USBIP_VirtPP_Request_GetInstance(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice->mInstance;
}

FredEmmott_USBIP_VirtPP_DeviceHandle FredEmmott_USBIP_VirtPP_Request_GetDevice(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice;
}

void* FredEmmott_USBIP_VirtPP_Request_GetInstanceUserData(
  FredEmmott_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice->mInstance->mInitData.mUserData;
}

void* FredEmmott_USBIP_VirtPP_Request_GetDeviceUserData(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice->mUserData;
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Request_SendReply(
  const FredEmmott_USBIP_VirtPP_Request* const request,
  const void* const data,
  const size_t dataSize) {
  const auto socket = request->mDevice->mInstance->mClientSocket;
  const auto actualLength
    = std::min<uint32_t>(dataSize, request->mTransferBufferLength);
  USBIP::USBIP_RET_SUBMIT response{
    .mActualLength = actualLength,
  };
  response.mHeader.mSequenceNumber = request->mSequenceNumber;
  if (const auto ret = SendAll(socket, response); !ret)
  [[unlikely]] {
    return std::bit_cast<FredEmmott_USBIP_VirtPP_Result>(ret.error());
  }
  if (actualLength == 0) {
    return FredEmmott_USBIP_VirtPP_SUCCESS;
  }

  if (const auto ret = SendAll(socket, data, actualLength); !ret)
  [[unlikely]] {
    return std::bit_cast<FredEmmott_USBIP_VirtPP_Result>(ret.error());
  }

  return FredEmmott_USBIP_VirtPP_SUCCESS;
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Request_SendStringReply(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle,
  wchar_t const* data,
  size_t charCount) {
  const auto byteCount = (charCount * 2) + offsetof(_USB_STRING_DESCRIPTOR, bString);
  // TODO: check byteCount <= 0xff
  thread_local union {
    _USB_STRING_DESCRIPTOR descriptor;
    std::byte bytes[128];
  } reply;
  reply.descriptor = {
    .bLength = static_cast<uint8_t>(byteCount),
    .bDescriptorType = 0x03, // STRING
  };
  memcpy(reply.descriptor.bString, data, charCount * 2);
  return FredEmmott_USBIP_VirtPP_Request_SendReply(handle, reply.bytes, byteCount);
}
