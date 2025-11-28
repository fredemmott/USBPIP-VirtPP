// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <FredEmmott/USB-VirtPP.h>
#include <FredEmmott/USBIP.hpp>
#include <stop_token>

#include <vector>

#define NOMINMAX
#include <winsock2.h>
#include <wil/resource.h>

#include <expected>
#include <format>
#include <print>

struct FREDEMMOTT_USB_VIRTPP_Device final {
  FREDEMMOTT_USB_VIRTPP_InstanceHandle mInstance{};

  FREDEMMOTT_USB_VIRTPP_Device_Callbacks mCallbacks{};
  FREDEMMOTT_USB_VIRTPP_Device_DeviceConfig mConfig{};
  std::vector<FREDEMMOTT_USB_VIRTPP_Device_InterfaceConfig> mInterfaces{};

  void* mUserData{};

  FREDEMMOTT_USB_VIRTPP_Device() = delete;
  explicit FREDEMMOTT_USB_VIRTPP_Device(FREDEMMOTT_USB_VIRTPP_InstanceHandle,
    const FREDEMMOTT_USB_VIRTPP_Device_InitData*);
  ~FREDEMMOTT_USB_VIRTPP_Device() = default;
};

struct FREDEMMOTT_USB_VIRTPP_Instance final {
  using Bus = std::vector<FREDEMMOTT_USB_VIRTPP_Device>;

  FREDEMMOTT_USB_VIRTPP_Instance_InitData mInitData{};

  std::stop_source mStopSource;

  wil::unique_socket mListeningSocket{};
  SOCKET mClientSocket{INVALID_SOCKET};

  std::vector<Bus> mBusses{};

  FREDEMMOTT_USB_VIRTPP_Instance() = delete;
  explicit FREDEMMOTT_USB_VIRTPP_Instance(const FREDEMMOTT_USB_VIRTPP_Instance_InitData*);
  ~FREDEMMOTT_USB_VIRTPP_Instance();
  uint16_t GetPortNumber() const;

  template<class... Args>
  void LogSeverity(const int severity, std::format_string<Args...> fmt, Args&&... args) const {
    if (const auto callback = mInitData.mCallbacks.OnLogMessage) {
      const auto formatted = std::format(fmt, std::forward<Args>(args)...);
      callback(severity, formatted.c_str(), formatted.size());
    } else {
      std::println((severity >= FREDEMMOTT_USB_VIRTPP_LogSeverity_Error) ? stderr : stdout, fmt, std::forward<Args>(args)...);
    }
  }
  template<class... Args>
  void LogError(std::format_string<Args...> fmt, Args&&... args) const {
    LogSeverity(FREDEMMOTT_USB_VIRTPP_LogSeverity_Error, fmt, std::forward<Args>(args)...);
  }

  template<class... Args>
  void Log(std::format_string<Args...> fmt, Args&&... args) const {
    LogSeverity(FREDEMMOTT_USB_VIRTPP_LogSeverity_Default, fmt, std::forward<Args>(args)...);
  }

  void Run();

private:
  bool mNeedWSACleanup {false};

  void HandleClient(SOCKET);
  std::expected<void, HRESULT> OnDevListOp();
  std::expected<void, HRESULT> OnImportOp(const FredEmmott::USBIP::OP_REQ_IMPORT&);
  std::expected<void, HRESULT> OnInputRequest(
    FREDEMMOTT_USB_VIRTPP_Device& device,
    const FredEmmott::USBIP::USBIP_CMD_SUBMIT& request,
    FREDEMMOTT_USB_VIRTPP_Request apiRequest);
  std::expected<void, HRESULT> OnSubmitRequest(const FredEmmott::USBIP::USBIP_CMD_SUBMIT&);
  std::expected<void, HRESULT> OnUnlinkRequest(const FredEmmott::USBIP::USBIP_CMD_UNLINK&);
};

struct FREDEMMOTT_USB_VIRTPP_Request {
  FREDEMMOTT_USB_VIRTPP_DeviceHandle mDevice{};
  uint32_t mSequenceNumber{};
  uint32_t mTransferBufferLength{};
};