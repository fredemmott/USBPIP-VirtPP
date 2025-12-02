// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include "win32-attach.hpp"

#include <wil/resource.h>

#include <expected>
#include <format>
#include <string>

#include <cfgmgr32.h>
#include <winioctl.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Cfgmgr32.lib")
namespace FredEmmott::USBIP::Win2Client {

namespace {
std::expected<std::wstring, HRESULT> GetUSBIPWin32Path() {
  // Taken from the usbip-win2 driver:
  constexpr GUID DeviceGUID {
    0xB4030C06,
    0xDC5F,
    0x4FCC,
    {0x87, 0xEB, 0xE5, 0x51, 0x5A, 0x09, 0x35, 0xC0}};

  std::wstring devicePaths;
  while (true) {
    ULONG cch {};
    if (const auto err = CM_Get_Device_Interface_List_SizeW(
          &cch,
          const_cast<GUID*>(&DeviceGUID),
          nullptr,
          CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
        err != CR_SUCCESS) {
      return std::unexpected {
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
        return std::unexpected {
          HRESULT_FROM_WIN32(CM_MapCrToWin32Err(err, ERROR_INVALID_PARAMETER))};
    }
    break;
  }
  const auto endOfPath = devicePaths.find(L'\0');
  if (endOfPath != devicePaths.size() - 2) {
    return std::unexpected {HRESULT_FROM_WIN32(ERROR_EMPTY)};
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
  const ULONG mSize {sizeof(AttachIOCTL)};
  int mPortOutput {};// USB/USB-IP port, *not* network port
  char mBusID[32] {};
  char mService[NI_MAXSERV] {};// a.k.a. network port
  char mHost[NI_MAXHOST] {"localhost"};
};

[[nodiscard]]
auto UnexpectedHR(const AttachError::Stage stage, const HRESULT hr) {
  return std::unexpected {AttachError {stage, hr}};
}
auto UnexpectedWin32(const AttachError::Stage stage, const DWORD win32) {
  return UnexpectedHR(stage, HRESULT_FROM_WIN32(win32));
}

}// namespace

std::expected<uint16_t, AttachError> Attach(
  const uint16_t tcpPortNumber,
  char const* busID,
  const std::size_t busIDLength) {
  using Stage = AttachError::Stage;

  if (
    tcpPortNumber == 0 || busIDLength == 0 || (!busID)
    || busID[busIDLength - 1] == 0
    || busIDLength > sizeof(AttachIOCTL::mBusID) - 1) {
    return UnexpectedWin32(Stage::Discovery, ERROR_INVALID_PARAMETER);
  }
  const auto targetPath = GetUSBIPWin32Path();
  if (!targetPath) {
    return UnexpectedHR(Stage::Discovery, targetPath.error());
  }

  AttachIOCTL ioctlData {};
  std::format_to(ioctlData.mService, "{}", tcpPortNumber);
  std::ranges::copy(busID, busID + busIDLength, ioctlData.mBusID);

  const wil::unique_hfile target {CreateFileW(
    targetPath->c_str(),
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    nullptr,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    nullptr)};
  if (!target) {
    return UnexpectedWin32(Stage::Open, GetLastError());
  }

  DWORD bytesReturned {};
  constexpr auto WritableLength
    = offsetof(AttachIOCTL, mPortOutput) + sizeof(AttachIOCTL::mPortOutput);
  if (!DeviceIoControl(
        target.get(),
        AttachIOCTL::IOCTLCode,
        &ioctlData,
        sizeof(ioctlData),
        &ioctlData,
        WritableLength,
        &bytesReturned,
        nullptr)) {
    return UnexpectedWin32(Stage::IOControl, GetLastError());
  }
  if (ioctlData.mPortOutput <= 0) {
    return UnexpectedWin32(Stage::ResponseValidation, ERROR_INVALID_INDEX);
  }

  return static_cast<uint16_t>(ioctlData.mPortOutput);
}
}// namespace FredEmmott::USBIP::Win2Client