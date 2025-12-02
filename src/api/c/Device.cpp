// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "detail.hpp"
#include "send-recv.hpp"
#include "win32-attach.hpp"

#include <FredEmmott/USBIP-VirtPP/Core.h>
#include <FredEmmott/USBIP.hpp>

#include <print>
#include <ranges>

namespace USBIP = FredEmmott::USBIP;
using namespace FredEmmott::USBVirtPP;

FredEmmott_USBIP_VirtPP_InstanceHandle
FredEmmott_USBIP_VirtPP_Device_GetInstance(
  const FredEmmott_USBIP_VirtPP_DeviceHandle handle) {
  return handle->mInstance;
}

void* FredEmmott_USBIP_VirtPP_Device_GetInstanceUserData(
  const FredEmmott_USBIP_VirtPP_DeviceHandle handle) {
  return handle->mInstance->mInitData.mUserData;
}

void* FredEmmott_USBIP_VirtPP_Device_GetUserData(
  const FredEmmott_USBIP_VirtPP_DeviceHandle handle) {
  return handle->mUserData;
}

FredEmmott_USBIP_VirtPP_Device::FredEmmott_USBIP_VirtPP_Device(
  FredEmmott_USBIP_VirtPP_InstanceHandle instance,
  const FredEmmott_USBIP_VirtPP_Device_InitData* initData)
  : mInstance(instance) {
  if (!instance) {
    return;
  }
  if (!initData) {
    instance->LogError("Can't create device without init data");
    return;
  }

  if (!initData->mDeviceDescriptor) {
    instance->LogError("Can't create device without device config");
    return;
  }

  mAutoAttach = initData->mAutoAttach;
  mCallbacks = initData->mCallbacks;
  if (!mCallbacks.OnInputRequest) {
    instance->LogError("Can't create device without OnInputRequest callback");
    mInstance = nullptr;
    return;
  }

  mDescriptor = *initData->mDeviceDescriptor;
  for (auto i = 0; i < initData->mNumInterfaces; ++i) {
    mInterfaces.emplace_back(initData->mInterfaceDescriptors[i]);
  }
  mUserData = initData->mUserData;
  if (instance->mBusses.empty()) {
    instance->mBusses.emplace_back();
  }
  instance->mBusses.back().emplace_back(this);
}

FredEmmott_USBIP_VirtPP_DeviceHandle FredEmmott_USBIP_VirtPP_Device_Create(
  const FredEmmott_USBIP_VirtPP_InstanceHandle instance,
  const FredEmmott_USBIP_VirtPP_Device_InitData* initData) {
  auto ret
    = std::make_unique<FredEmmott_USBIP_VirtPP_Device>(instance, initData);
  if (!ret->mInstance) {
    return nullptr;
  }
  return ret.release();
}

void FredEmmott_USBIP_VirtPP_Device_Destroy(
  const FredEmmott_USBIP_VirtPP_DeviceHandle handle) {
  delete handle;
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Device_Attach(
  const FredEmmott_USBIP_VirtPP_DeviceHandle handle) {
  return handle->Attach();
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Device::Attach(
  std::string_view busID) const {
  std::string generatedBusID;
  if (busID.empty()) {
    // Need parent-scoped storage
    const auto generated = GetBusID();
    if (!generated) {
      return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    busID = std::move(generated).value();
  }
  const auto usbPort = USBIP::Win2Client::Attach(
    mInstance->GetPortNumber(), busID.data(), busID.size());
  if (usbPort) {
    mInstance->Log(
      "+ Attached device {} to local server, on USB port {}", busID, *usbPort);
    return FredEmmott_USBIP_VirtPP_SUCCESS;
  }
  return usbPort.error().hr;
}

std::optional<std::string> FredEmmott_USBIP_VirtPP_Device::GetBusID() const {
  for (auto&& [i, bus]: std::views::enumerate(mInstance->mBusses)) {
    for (auto&& [j, device]: std::views::enumerate(bus)) {
      if (device == this) {
        return std::format("{}-{}", i + 1, j + 1);
      }
    }
  }
  return std::nullopt;
}