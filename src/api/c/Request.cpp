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

FREDEMMOTT_USBIP_VirtPP_InstanceHandle FREDEMMOTT_USBIP_VirtPP_Request_GetInstance(
  const FREDEMMOTT_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice->mInstance;
}

FREDEMMOTT_USBIP_VirtPP_DeviceHandle FREDEMMOTT_USBIP_VirtPP_Request_GetDevice(
  const FREDEMMOTT_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice;
}

void* FREDEMMOTT_USBIP_VirtPP_Request_GetInstanceUserData(
  FREDEMMOTT_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice->mInstance->mInitData.mUserData;
}

void* FREDEMMOTT_USBIP_VirtPP_Request_GetDeviceUserData(
  const FREDEMMOTT_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice->mUserData;
}

FREDEMMOTT_USBIP_VirtPP_Result FREDEMMOTT_USBIP_VirtPP_Request_SendReply(
  const FREDEMMOTT_USBIP_VirtPP_Request* const request,
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
    return std::bit_cast<FREDEMMOTT_USBIP_VirtPP_Result>(ret.error());
  }
  if (actualLength == 0) {
    return FREDEMMOTT_USBIP_VirtPP_SUCCESS;
  }

  if (const auto ret = SendAll(socket, data, actualLength); !ret)
  [[unlikely]] {
    return std::bit_cast<FREDEMMOTT_USBIP_VirtPP_Result>(ret.error());
  }

  return FREDEMMOTT_USBIP_VirtPP_SUCCESS;
}