// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "detail.hpp"
#include "send-recv.hpp"

#include <FredEmmott/USBIP-VirtPP.h>
#include <FredEmmott/USBIP.hpp>

#include <print>
#include <ranges>

#include <winioctl.h>
#include <cfgmgr32.h>

#pragma comment(lib, "Cfgmgr32.lib")

namespace USBIP = FredEmmott::USBIP;
using namespace FredEmmott::USBVirtPP;

namespace {
std::expected<std::wstring, HRESULT> GetUSBIPWin32Path() {
  // Taken from the usbip-win2 driver:
  constexpr GUID DeviceGUID{
    0xB4030C06, 0xDC5F, 0x4FCC, {0x87, 0xEB, 0xE5, 0x51, 0x5A, 0x09, 0x35, 0xC0}
  };

  std::wstring devicePaths;
  while (true) {
    ULONG cch{};
    if (const auto err = CM_Get_Device_Interface_List_SizeW(
      &cch,
      const_cast<GUID*>(&DeviceGUID),
      nullptr,
      CM_GET_DEVICE_INTERFACE_LIST_PRESENT); err != CR_SUCCESS) {
      return std::unexpected{
        HRESULT_FROM_WIN32(CM_MapCrToWin32Err(err, ERROR_INVALID_PARAMETER))};
    }
    if (cch == 0) {
      __debugbreak();
      break;
    }
    devicePaths.resize(cch, L'\0');
    switch (const auto err = CM_Get_Device_Interface_ListW(
      const_cast<GUID*>(&DeviceGUID),
      nullptr,
      devicePaths.data(),
      cch,
      CM_GET_DEVICE_INTERFACE_LIST_PRESENT)) {
      case CR_SUCCESS:
        break;
      case CR_BUFFER_SMALL:
        continue;
      default:
        return std::unexpected{
          HRESULT_FROM_WIN32(CM_MapCrToWin32Err(err, ERROR_INVALID_PARAMETER))};
    }
    break;
  }
  const auto endOfPath = devicePaths.find(L'\0');
  if (endOfPath != devicePaths.size() - 2) {
    return std::unexpected{HRESULT_FROM_WIN32(ERROR_EMPTY)};
  }
  devicePaths.resize(devicePaths.size() - 2);
  return devicePaths;
}

// a.k.a PLUGIN_HARDWARE in usbip-win2
struct AttachIOCTL {
  static constexpr uint32_t IOCTLCode = CTL_CODE(
    FILE_DEVICE_UNKNOWN,
    0x800,
    METHOD_BUFFERED,
    FILE_READ_DATA | FILE_WRITE_DATA);
  const ULONG mSize{sizeof(AttachIOCTL)};
  int mPortOutput{};// USB/USB-IP port, *not* network port
  char mBusID[32]{};
  char mService[NI_MAXSERV]{};// a.k.a. network port
  char mHost[NI_MAXHOST]{"localhost"};
};
}

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
  instance->mBusses.back().emplace_back(*this);
}

FredEmmott_USBIP_VirtPP_DeviceHandle FredEmmott_USBIP_VirtPP_Device_Create(
  const FredEmmott_USBIP_VirtPP_InstanceHandle instance,
  const FredEmmott_USBIP_VirtPP_Device_InitData* initData) {
  auto ret = std::make_unique<FredEmmott_USBIP_VirtPP_Device>(
    instance,
    initData);
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
  return static_cast<FredEmmott_USBIP_VirtPP_Result>(handle->Attach().error_or(
    S_OK));
}

std::expected<void, HRESULT> FredEmmott_USBIP_VirtPP_Device::Attach(
  const std::string_view busID
  ) const {
  // I decided to directly use the IOCTL primarily because of the use of C++
  // STL containers in the libusbip API definition; lesser concerns are the
  // use of the C++ ABI more broadly as opposed to C API/ABI, and tying it
  // into the build system.
  //
  // I love the modern C++ features, but I don't want to either build it myself
  // or tie to the same C++ version/API version for this one IOCTL.
  AttachIOCTL ioctlData{};
  std::format_to(ioctlData.mService, "{}", mInstance->GetPortNumber());

  if (!busID.empty()) {
    strncpy_s(ioctlData.mBusID, busID.data(), busID.size());
  } else if (const auto generated = GetBusID()) {
    strncpy_s(ioctlData.mBusID, generated->data(), generated->size());
  } else {
    return std::unexpected{HRESULT_FROM_WIN32(ERROR_INVALID_INDEX)};
  }

  const auto targetPath = GetUSBIPWin32Path();
  if (!targetPath) {
    return std::unexpected{targetPath.error()};
  }

  const wil::unique_hfile target{
    CreateFileW(
      targetPath->c_str(),
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr)
  };
  if (!target) {
    return std::unexpected{HRESULT_FROM_WIN32(GetLastError())};
  }

  DWORD bytesReturned{};
  constexpr auto WritableLength = offsetof(AttachIOCTL, mPortOutput) + sizeof(
    AttachIOCTL::mPortOutput);
  if (!DeviceIoControl(
    target.get(),
    AttachIOCTL::IOCTLCode,
    &ioctlData,
    sizeof(ioctlData),
    &ioctlData,
    WritableLength,
    &bytesReturned,
    nullptr)) {
    __debugbreak();
  }
  mInstance->Log("+ Attached device {} to local server", ioctlData.mBusID);

  return {};
}

std::optional<std::string> FredEmmott_USBIP_VirtPP_Device::GetBusID() const {
  for (auto&& [i, bus]: std::views::enumerate(mInstance->mBusses)) {
    for (auto&& [j, device]: std::views::enumerate(bus)) {
      if (&device == this) {
        return std::format("{}-{}", i + 1, j + 1);
      }
    }
  }
  return std::nullopt;
}