// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <FredEmmott/USBIP-VirtPP/Core.h>
#include <FredEmmott/USBIP-VirtPP/Device.h>
#include <FredEmmott/USBIP.hpp>
#include "logging.hpp"

#include <format>
#include <optional>
#include <stop_token>
#include <string_view>
#include <vector>

#include <mutex>

// clang-format off
#include <winsock2.h>
#include <wil/resource.h>
// clang-format on

struct FredEmmott_USBIP_VirtPP_Device final {
  bool mAutoAttach {};

  FredEmmott_USBIP_VirtPP_InstanceHandle mInstance {};

  FredEmmott_USBIP_VirtPP_Device_Callbacks mCallbacks {};
  FredEmmott_USBSpec_DeviceDescriptor mDescriptor {};
  std::vector<FredEmmott_USBSpec_InterfaceDescriptor> mInterfaces {};

  std::mutex mReplyMutex;

  void* mUserData {};

  FredEmmott_USBIP_VirtPP_Device() = delete;
  explicit FredEmmott_USBIP_VirtPP_Device(
    FredEmmott_USBIP_VirtPP_InstanceHandle,
    const FredEmmott_USBIP_VirtPP_Device_InitData*);
  ~FredEmmott_USBIP_VirtPP_Device() = default;

  [[nodiscard]]
  FredEmmott_USBIP_VirtPP_Result Attach(std::string_view busID = {}) const;

 private:
  std::optional<std::string> GetBusID() const;
};

struct FredEmmott_USBIP_VirtPP_Instance final {
  using Bus = std::vector<FredEmmott_USBIP_VirtPP_DeviceHandle>;

  FredEmmott_USBIP_VirtPP_Instance_InitData mInitData {};

  std::stop_source mStopSource;

  wil::unique_socket mListeningSocket {};
  SOCKET mClientSocket {INVALID_SOCKET};

  std::vector<Bus> mBusses {};

  FredEmmott_USBIP_VirtPP_Instance() = delete;
  explicit FredEmmott_USBIP_VirtPP_Instance(
    const FredEmmott_USBIP_VirtPP_Instance_InitData*);
  ~FredEmmott_USBIP_VirtPP_Instance();
  uint16_t GetPortNumber() const;

  void Run();

  template<class... Args>
  void LogError(std::format_string<Args...> fmt, Args&&... args) const {
    return FredEmmott::USBVirtPP::LogError(this, fmt, std::forward<Args>(args)...);
  }

  template<class... Args>
  void Log(std::format_string<Args...> fmt, Args&&... args) const {
    return FredEmmott::USBVirtPP::Log(this, fmt, std::forward<Args>(args)...);
  }

 private:
  bool mNeedWSACleanup {false};

  [[nodiscard]] HRESULT OnClientSocketActive(SOCKET);
  [[nodiscard]] FredEmmott_USBIP_VirtPP_Result OnDevListOp();
  [[nodiscard]] FredEmmott_USBIP_VirtPP_Result OnImportOp(
    const FredEmmott::USBIP::OP_REQ_IMPORT&);

  [[nodiscard]]
  FredEmmott_USBIP_VirtPP_Result OnInputRequest(
    FredEmmott_USBIP_VirtPP_Device& device,
    const FredEmmott::USBIP::USBIP_CMD_SUBMIT& request,
    FredEmmott_USBIP_VirtPP_Request& apiRequest);
  [[nodiscard]]
  FredEmmott_USBIP_VirtPP_Result OnOutputRequest(
    FredEmmott_USBIP_VirtPP_Device& device,
    const FredEmmott::USBIP::USBIP_CMD_SUBMIT& request,
    FredEmmott_USBIP_VirtPP_Request& apiRequest);

  [[nodiscard]]
  FredEmmott_USBIP_VirtPP_Result OnSubmitRequest(
    const FredEmmott::USBIP::USBIP_CMD_SUBMIT&);
  [[nodiscard]] FredEmmott_USBIP_VirtPP_Result OnUnlinkRequest(
    const FredEmmott::USBIP::USBIP_CMD_UNLINK&);

  void AutoAttach();
};

struct FredEmmott_USBIP_VirtPP_Request {
  FredEmmott_USBIP_VirtPP_DeviceHandle mDevice {};
  SOCKET mSocket { INVALID_SOCKET };
  uint32_t mSequenceNumber {};
  uint32_t mTransferBufferLength {};
};