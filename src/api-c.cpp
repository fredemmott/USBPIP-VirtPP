// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "api-detail.hpp"
#include "send-recv.hpp"

#include <FredEmmott/USBIP-VirtPP.h>
#include <FredEmmott/USBIP.hpp>

#include <algorithm>
#include <print>
#include <ranges>

#include <ws2tcpip.h>

namespace USBIP = FredEmmott::USBIP;
using namespace FredEmmott::USBVirtPP;

namespace {
auto MakeUSBIPDevice(
  uint32_t busId,
  uint32_t deviceId,
  const FREDEMMOTT_USBIP_VirtPP_Device_DeviceConfig& config) {
  USBIP::Device ret{
    .mBusNum = busId,
    .mDevNum = deviceId,
    .mSpeed = USBIP::Speed::Full,
    .mVendorID = config.mVendorID,
    .mProductID = config.mProductID,
    .mDeviceVersion = config.mDeviceVersion,
    .mDeviceClass = config.mDeviceClass,
    .mDeviceSubClass = config.mDeviceSubClass,
    .mDeviceProtocol = config.mDeviceProtocol,
    .mConfigurationValue = config.mConfigurationValue,
    .mNumConfigurations = config.mNumConfigurations,
    .mNumInterfaces = config.mNumInterfaces,
  };
  std::format_to(
    ret.mPath,
    "/github.com/fredemmott/USBIP-VirtPP/{}/{}",
    busId,
    deviceId);
  std::format_to(ret.mBusID, "{}-{}", busId, deviceId);
  return ret;
}
}

extern "C" FREDEMMOTT_USBIP_VirtPP_InstanceHandle FREDEMMOTT_USBIP_VirtPP_Instance_Create(
  const FREDEMMOTT_USBIP_VirtPP_Instance_InitData* initData) {
  auto ret = std::make_unique<FREDEMMOTT_USBIP_VirtPP_Instance>(initData);
  if (!ret->mListeningSocket) {
    return nullptr;
  }
  return ret.release();
}

FREDEMMOTT_USBIP_VirtPP_Instance::~FREDEMMOTT_USBIP_VirtPP_Instance() {
  if (mNeedWSACleanup)
    WSACleanup();
}

FREDEMMOTT_USBIP_VirtPP_Instance::FREDEMMOTT_USBIP_VirtPP_Instance(
  const FREDEMMOTT_USBIP_VirtPP_Instance_InitData* initData)
  : mInitData(*initData) {
  WSADATA wsaData{};
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    LogError("WSAStartup failed: {}", WSAGetLastError());
    return;
  }
  mNeedWSACleanup = true;

  wil::unique_socket listeningSocket{socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)};
  if (!listeningSocket) {
    LogError(
      "Creating listening socket failed with error: {}",
      WSAGetLastError());
    return;
  }

  sockaddr_in server_addr{
    .sin_family = AF_INET,
    .sin_port = htons(initData->mPortNumber),
  };
  // FIXME server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (
    bind(
      listeningSocket.get(),
      reinterpret_cast<sockaddr*>(&server_addr),
      sizeof(server_addr))
    == SOCKET_ERROR) {
    LogError("bind failed with error: {}", WSAGetLastError());
    return;
  }

  if (listen(listeningSocket.get(), SOMAXCONN) == SOCKET_ERROR) {
    LogError("listen failed with error: {}", WSAGetLastError());
    return;
  }
  mListeningSocket = std::move(listeningSocket);
}

uint16_t FREDEMMOTT_USBIP_VirtPP_Instance_GetPortNumber(
  const FREDEMMOTT_USBIP_VirtPP_InstanceHandle instance) {
  return instance->GetPortNumber();
}

uint16_t FREDEMMOTT_USBIP_VirtPP_Instance::GetPortNumber() const {
  if (!mListeningSocket) {
    LogError("GetPortNumber() called without a listening socket");
    return 0;
  }
  sockaddr_in server_addr{.sin_family = AF_INET};
  socklen_t addr_len = sizeof(server_addr);
  if (getsockname(
    mListeningSocket.get(),
    reinterpret_cast<sockaddr*>(&server_addr),
    &addr_len) != 0) {
    LogError("getsockname failed with error: {}", WSAGetLastError());
    return 0;
  }
  return ntohs(server_addr.sin_port);
}

void FREDEMMOTT_USBIP_VirtPP_Instance_Run(
  const FREDEMMOTT_USBIP_VirtPP_InstanceHandle instance) {
  instance->Run();
}

void FREDEMMOTT_USBIP_VirtPP_Instance::Run() {
  while (!mStopSource.stop_requested()) {
    Log("Listening for USB/IP connections on port {}", this->GetPortNumber());
    const wil::unique_socket clientSocket{
      accept(mListeningSocket.get(), nullptr, nullptr)};
    if (!clientSocket) {
      LogError("accept failed with error: {}", WSAGetLastError());
      continue;
    }
    this->HandleClient(clientSocket.get());
  }
}

void FREDEMMOTT_USBIP_VirtPP_Instance::HandleClient(const SOCKET clientSocket) {
  if (mClientSocket != INVALID_SOCKET) {
    LogError("Multiple clients connected");
    return;
  }
  if (clientSocket == INVALID_SOCKET || !clientSocket) {
    LogError("Invalid client socket");
    return;
  }

  mClientSocket = clientSocket;
  const auto forgetSocketAtExit = wil::scope_exit(
    [this] {
      mClientSocket = INVALID_SOCKET;
    });

  Log("USB/IP connection established");

  while (!mStopSource.stop_requested()) {
    union {
      USBIP::CommandCode mCommandCode{};
      USBIP::OP_REQ_DEVLIST mOP_REQ_DEVLIST;
      USBIP::OP_REQ_IMPORT mOP_REQ_IMPORT;
      USBIP::USBIP_CMD_SUBMIT mUSBIP_CMD_SUBMIT;
      USBIP::USBIP_CMD_UNLINK mUSBIP_CMD_UNLINK;
    } request;

    if (!RecvAll(clientSocket, &request.mCommandCode)) {
      std::println(stderr, "Client disconnected or recv failed.");
      break;// Exit loop on failure/disconnect
    }

    const auto RecvRemainder = [this]<class T>(const SOCKET sock, T* what) {
      const auto ret = RecvAll(
        sock,
        reinterpret_cast<char*>(what) + sizeof(USBIP::CommandCode),
        sizeof(T) - sizeof(USBIP::CommandCode));
      if (!ret) [[unlikely]] {
        LogError("-> Failed to read required data: {}", ret.error());
      }
      return ret;
    };

    switch (request.mCommandCode) {
      case USBIP::CommandCode::OP_REQ_DEVLIST:
        Log("-> Received REQ_DEVLIST");
        if (!RecvRemainder(clientSocket, &request.mOP_REQ_DEVLIST))
          return;
        if (!this->OnDevListOp())
          return;
        continue;
      case USBIP::CommandCode::OP_REQ_IMPORT:
        Log("-> Received REQ_IMPORT");
        if (!RecvRemainder(clientSocket, &request.mOP_REQ_IMPORT))
          return;
        if (!this->OnImportOp(request.mOP_REQ_IMPORT))
          return;
        continue;
      case USBIP::CommandCode::USBIP_CMD_SUBMIT:
        // Not logging here, way too spammy :)
        if (!RecvRemainder(clientSocket, &request.mUSBIP_CMD_SUBMIT))
          return;
        if (!this->OnSubmitRequest(request.mUSBIP_CMD_SUBMIT))
          return;
        continue;
      case USBIP::CommandCode::USBIP_CMD_UNLINK: {
        Log("-> Received CMD_UNLINK");
        if (!RecvRemainder(clientSocket, &request.mUSBIP_CMD_UNLINK))
          return;
        USBIP::USBIP_RET_UNLINK response{};
        response.mHeader.mSequenceNumber
          = request.mUSBIP_CMD_UNLINK.mHeader.mSequenceNumber;
        if (!SendAll(clientSocket, response))
          return;
        continue;
      }
      case USBIP::CommandCode::USBIP_RET_SUBMIT:
      case USBIP::CommandCode::USBIP_RET_UNLINK:
        LogError("-> Received a RET instead of a CMD");
        return;
      default:
        LogError(
          "-> Unhandled USB/IP command code: 0x{:x}",
          std::to_underlying(request.mCommandCode));
        return;
    }
  }
}

std::expected<void, HRESULT> FREDEMMOTT_USBIP_VirtPP_Instance::OnDevListOp() {
  const USBIP::OP_REP_DEVLIST header{
    .mNumDevices = static_cast<uint32_t>(std::ranges::fold_left(
      mBusses,
      0,
      [](auto acc, const auto& bus) { return acc + bus.size(); })),
  };
  if (const auto ret = SendAll(mClientSocket, header); !ret)
    return ret;
  for (auto&& [busIdx, bus]: std::views::enumerate(mBusses)) {
    for (auto&& [deviceIdx, device]: std::views::enumerate(bus)) {
      const auto& config = device.mConfig;
      const auto usbipDevice = MakeUSBIPDevice(
        busIdx + 1,
        deviceIdx + 1,
        config);
      if (const auto ret = SendAll(mClientSocket, usbipDevice); !ret)
        return ret;
      for (auto&& iface: device.mInterfaces) {
        const USBIP::Interface wireInterface{
          .mClass = iface.mClass,
          .mSubClass = iface.mSubClass,
          .mProtocol = iface.mProtocol,
        };
        if (const auto ret = SendAll(mClientSocket, wireInterface); !ret)
          return ret;
      }
    }
  }

  return {};
}

std::expected<void, HRESULT> FREDEMMOTT_USBIP_VirtPP_Instance::OnImportOp(
  const FredEmmott::USBIP::OP_REQ_IMPORT& request) {
  const std::string_view busId{request.mBusID};

  for (auto&& [busIdx, bus]: std::views::enumerate(mBusses)) {
    for (auto&& [deviceIdx, device]: std::views::enumerate(bus)) {
      if (busId != std::format("{}-{}", busIdx + 1, deviceIdx + 1))
        continue;

      const auto usbipDevice = MakeUSBIPDevice(
        busIdx + 1,
        deviceIdx + 1,
        device.mConfig);

      if (const auto ret = SendAll(
        mClientSocket,
        USBIP::OP_REP_IMPORT{.mDevice = usbipDevice}); !ret)
        return ret;
      return {};
    }
  }

  LogError("Failed to find device with busID '{}'", busId);
  USBIP::OP_REP_IMPORT reply{};
  reply.mHeader.mStatus = 1;// per spec, 1 for error
  return SendAll(mClientSocket, reply);
}

std::expected<void, HRESULT> FREDEMMOTT_USBIP_VirtPP_Instance::OnInputRequest(FREDEMMOTT_USBIP_VirtPP_Device& device, const FredEmmott::USBIP::USBIP_CMD_SUBMIT& request, const FREDEMMOTT_USBIP_VirtPP_Request apiRequest)
{
  const auto ret = device.mCallbacks.OnInputRequest(
    &apiRequest,
    request.mHeader.mEndpoint.NativeValue(),
    request.mSetup.mRequestType,
    request.mSetup.mRequest,
    request.mSetup.mValue,
    request.mSetup.mIndex,
    request.mSetup.mLength);
  if (FREDEMMOTT_USBIP_VirtPP_SUCCEEDED(ret)) [[likely]] {
    return {};
  }
  return std::unexpected{std::bit_cast<HRESULT>(ret)};
}

std::expected<void, HRESULT> FREDEMMOTT_USBIP_VirtPP_Instance::OnSubmitRequest(
  const FredEmmott::USBIP::USBIP_CMD_SUBMIT& request) {
  const auto busIndex = (request.mHeader.mDeviceID.NativeValue() >> 16) - 1;
  const auto deviceIndex = (request.mHeader.mDeviceID.NativeValue() & 0xffff) -
    1;
  if (busIndex >= mBusses.size() || deviceIndex >= mBusses[busIndex].size()) [[
    unlikely]] {
    LogError(
      "Received submit request for invalid device: bus {}, device {}",
      busIndex,
      deviceIndex);
    return std::unexpected{HRESULT_FROM_WIN32(ERROR_INVALID_INDEX)};
  }
  auto& device = mBusses.at(busIndex).at(deviceIndex);
  const FREDEMMOTT_USBIP_VirtPP_Request apiRequest{
    .mDevice = &device,
    .mSequenceNumber = request.mHeader.mSequenceNumber,
    .mTransferBufferLength = request.mTransferBufferLength.NativeValue(),
  };

  if (request.mHeader.mDirection == USBIP::Direction::In) {
    return OnInputRequest(device, request, apiRequest);
  }

  if (request.mSetup.mRequestType == 0) {
    constexpr auto SetConfiguration = 0x09;
    switch (request.mSetup.mRequest) {
      case SetConfiguration: // no-op, we only support 1 configuration
        if (const auto ret = FREDEMMOTT_USBIP_VirtPP_Request_SendReply(&apiRequest, 0, nullptr, 0); !FREDEMMOTT_USBIP_VirtPP_SUCCEEDED(ret)) [[unlikely]] {
          return std::unexpected { static_cast<HRESULT>(ret) };
        }
        return {};
      default:
        __debugbreak();
    }

  }

  if ((request.mSetup.mRequestType & 0x20) == 0x20 && request.mSetup.mRequest == 0x0a) {
    // SET_IDLE
    // We don't suport repeating.
    if (const auto ret = FREDEMMOTT_USBIP_VirtPP_Request_SendReply(&apiRequest, 0, nullptr, 0); !FREDEMMOTT_USBIP_VirtPP_SUCCEEDED(ret)) [[unlikely]] {
      return std::unexpected { static_cast<HRESULT>(ret) };
    }
    return {};
  }

  LogError("TODO: ouput requests");
#ifndef _NDEBUG
  __debugbreak();
#endif
  return std::unexpected{HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS)};
}


std::expected<void, HRESULT> FREDEMMOTT_USBIP_VirtPP_Instance::OnUnlinkRequest(
  const USBIP::USBIP_CMD_UNLINK& request) {
  USBIP::USBIP_RET_UNLINK response{};
  response.mHeader.mSequenceNumber = request.mHeader.mSequenceNumber;
  return SendAll(mClientSocket, response);
}

void FREDEMMOTT_USBIP_VirtPP_Instance_RequestStop(
  const FREDEMMOTT_USBIP_VirtPP_InstanceHandle handle) {
  handle->mStopSource.request_stop();
}

void FREDEMMOTT_USBIP_VirtPP_Instance_Destroy(
  const FREDEMMOTT_USBIP_VirtPP_InstanceHandle handle) {
  delete handle;
}

FREDEMMOTT_USBIP_VirtPP_InstanceHandle FREDEMMOTT_USBIP_VirtPP_Device_GetInstance(
  const FREDEMMOTT_USBIP_VirtPP_DeviceHandle handle) {
  return handle->mInstance;
}

FREDEMMOTT_USBIP_VirtPP_InstanceHandle FREDEMMOTT_USBIP_VirtPP_Request_GetInstance(
  const FREDEMMOTT_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice->mInstance;
}

FREDEMMOTT_USBIP_VirtPP_DeviceHandle FREDEMMOTT_USBIP_VirtPP_Request_GetDevice(
  const FREDEMMOTT_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice;
}

void* FREDEMMOTT_USBIP_VirtPP_Instance_GetUserData(
  const FREDEMMOTT_USBIP_VirtPP_InstanceHandle handle) {
  return handle->mInitData.mUserData;
}

void* FREDEMMOTT_USBIP_VirtPP_Device_GetInstanceUserData(
  const FREDEMMOTT_USBIP_VirtPP_DeviceHandle handle) {
  return handle->mInstance->mInitData.mUserData;
}

void* FREDEMMOTT_USBIP_VirtPP_Request_GetInstanceUserData(
  FREDEMMOTT_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice->mInstance->mInitData.mUserData;
}

void* FREDEMMOTT_USBIP_VirtPP_Device_GetUserData(
  const FREDEMMOTT_USBIP_VirtPP_DeviceHandle handle) {
  return handle->mUserData;
}

void* FREDEMMOTT_USBIP_VirtPP_Request_GetDeviceUserData(
  const FREDEMMOTT_USBIP_VirtPP_RequestHandle handle) {
  return handle->mDevice->mUserData;
}

void FREDEMMOTT_USBIP_VirtPP_RequestStopInstance(
  FREDEMMOTT_USBIP_VirtPP_InstanceHandle instance) {
  instance->mStopSource.request_stop();
}

FREDEMMOTT_USBIP_VirtPP_Result FREDEMMOTT_USBIP_VirtPP_Request_SendReply(
  const FREDEMMOTT_USBIP_VirtPP_Request* const request,
  const uint32_t status,
  const void* const data,
  const size_t dataSize) {
  const auto socket = request->mDevice->mInstance->mClientSocket;
  const auto actualLength
    = std::min<uint32_t>(dataSize, request->mTransferBufferLength);
  USBIP::USBIP_RET_SUBMIT response{
    .mStatus = status,
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

FREDEMMOTT_USBIP_VirtPP_Device::FREDEMMOTT_USBIP_VirtPP_Device(
  FREDEMMOTT_USBIP_VirtPP_InstanceHandle instance,
  const FREDEMMOTT_USBIP_VirtPP_Device_InitData* initData)
  : mInstance(instance) {
  if (!instance) {
    return;
  }
  if (!initData) {
    instance->LogError("Can't create device without init data");
    return;
  }

  if (!initData->mCallbacks) {
    instance->LogError("Can't create device without callbacks");
    return;
  }
  if (!initData->mDeviceConfig) {
    instance->LogError("Can't create device without device config");
    return;
  }

  mCallbacks = *initData->mCallbacks;
  if (!mCallbacks.OnInputRequest) {
    instance->LogError("Can't create device without OnInputRequest callback");
    mInstance = nullptr;
    return;
  }

  mConfig = *initData->mDeviceConfig;
  for (auto i = 0; i < mConfig.mNumInterfaces; ++i) {
    mInterfaces.emplace_back(initData->mInterfaceConfigs[i]);
  }
  mUserData = initData->mUserData;
  if (instance->mBusses.empty()) {
    instance->mBusses.emplace_back();
  }
  instance->mBusses.back().emplace_back(*this);
}

FREDEMMOTT_USBIP_VirtPP_DeviceHandle FREDEMMOTT_USBIP_VirtPP_Device_Create(
  const FREDEMMOTT_USBIP_VirtPP_InstanceHandle instance,
  const FREDEMMOTT_USBIP_VirtPP_Device_InitData* initData) {
  auto ret = std::make_unique<FREDEMMOTT_USBIP_VirtPP_Device>(instance, initData);
  if (!ret->mInstance) {
    return nullptr;
  }
  return ret.release();
}

void FREDEMMOTT_USBIP_VirtPP_Device_Destroy(
  const FREDEMMOTT_USBIP_VirtPP_DeviceHandle handle) {
  delete handle;
}