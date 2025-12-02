// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "detail-RequestType.hpp"
#include "detail.hpp"
#include "send-recv.hpp"

#include <FredEmmott/USBIP-VirtPP/Core.h>
#include <FredEmmott/USBIP.hpp>

#include <algorithm>
#include <future>
#include <print>
#include <ranges>

#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

namespace USBIP = FredEmmott::USBIP;
using namespace FredEmmott::USBVirtPP;

namespace {
auto MakeUSBIPDevice(
  uint32_t busId,
  uint32_t deviceId,
  const FredEmmott_USBSpec_DeviceDescriptor& deviceDescriptor,
  const uint8_t numInterfaces) {
  USBIP::Device ret {
    .mBusNum = busId,
    .mDevNum = deviceId,
    .mSpeed = USBIP::Speed::Full,
    .mVendorID = deviceDescriptor.idVendor,
    .mProductID = deviceDescriptor.idProduct,
    .mDeviceVersion = deviceDescriptor.bcdDevice,
    .mDeviceClass = deviceDescriptor.bDeviceClass,
    .mDeviceSubClass = deviceDescriptor.bDeviceSubClass,
    .mDeviceProtocol = deviceDescriptor.bDeviceProtocol,
    .mNumConfigurations = deviceDescriptor.bNumConfigurations,
    .mNumInterfaces = numInterfaces};
  std::format_to(
    ret.mPath, "/github.com/fredemmott/USBIP-VirtPP/{}/{}", busId, deviceId);
  std::format_to(ret.mBusID, "{}-{}", busId, deviceId);
  return ret;
}
}// namespace

extern "C" FredEmmott_USBIP_VirtPP_InstanceHandle
FredEmmott_USBIP_VirtPP_Instance_Create(
  const FredEmmott_USBIP_VirtPP_Instance_InitData* initData) {
  auto ret = std::make_unique<FredEmmott_USBIP_VirtPP_Instance>(initData);
  if (!ret->mListeningSocket) {
    return nullptr;
  }
  return ret.release();
}

FredEmmott_USBIP_VirtPP_Instance::~FredEmmott_USBIP_VirtPP_Instance() {
  if (mNeedWSACleanup)
    WSACleanup();
}

FredEmmott_USBIP_VirtPP_Instance::FredEmmott_USBIP_VirtPP_Instance(
  const FredEmmott_USBIP_VirtPP_Instance_InitData* initData)
  : mInitData(*initData) {
  WSADATA wsaData {};
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    LogError("WSAStartup failed: {}", WSAGetLastError());
    return;
  }
  mNeedWSACleanup = true;

  wil::unique_socket listeningSocket {
    socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)};
  if (!listeningSocket) {
    LogError(
      "Creating listening socket failed with error: {}", WSAGetLastError());
    return;
  }

  sockaddr_in server_addr {
    .sin_family = AF_INET,
    .sin_port = htons(initData->mPortNumber),
  };

  if (initData->mAllowRemoteConnections) {
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  }

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

uint16_t FredEmmott_USBIP_VirtPP_Instance_GetPortNumber(
  const FredEmmott_USBIP_VirtPP_InstanceHandle instance) {
  return instance->GetPortNumber();
}

uint16_t FredEmmott_USBIP_VirtPP_Instance::GetPortNumber() const {
  if (!mListeningSocket) {
    LogError("GetPortNumber() called without a listening socket");
    return 0;
  }
  sockaddr_in server_addr {.sin_family = AF_INET};
  socklen_t addr_len = sizeof(server_addr);
  if (
    getsockname(
      mListeningSocket.get(),
      reinterpret_cast<sockaddr*>(&server_addr),
      &addr_len)
    != 0) {
    LogError("getsockname failed with error: {}", WSAGetLastError());
    return 0;
  }
  return ntohs(server_addr.sin_port);
}

void FredEmmott_USBIP_VirtPP_Instance_Run(
  const FredEmmott_USBIP_VirtPP_InstanceHandle instance) {
  instance->Run();
}

void FredEmmott_USBIP_VirtPP_Instance::Run() {
  const auto future = std::async(
    std::launch::async, &FredEmmott_USBIP_VirtPP_Instance::AutoAttach, this);
  const wil::unique_event stopEvent {
    CreateEventW(nullptr, TRUE, FALSE, nullptr)};
  const std::stop_callback stopCallback(
    mStopSource.get_token(), std::bind_front(&SetEvent, stopEvent.get()));

  const wil::unique_event listenEvent {WSACreateEvent()};
  WSAEventSelect(mListeningSocket.get(), listenEvent.get(), FD_ACCEPT);

  std::vector events {stopEvent.get(), listenEvent.get()};

  std::vector<wil::unique_socket> clientSockets;
  std::vector<wil::unique_event> clientEvents;
  Log("Listening for USB/IP connections on port {}", this->GetPortNumber());
  while (!mStopSource.stop_requested()) {
    const auto wait
      = WaitForMultipleObjects(events.size(), events.data(), FALSE, INFINITE);
    const auto waitIdx = wait - WAIT_OBJECT_0;
    if (waitIdx < 0 || waitIdx >= events.size()) {
      __debugbreak();
      break;
    }

    ResetEvent(events.at(waitIdx));

    switch (waitIdx) {
      // stopEvent
      case 0:
        Log("Server stop requested, stopping");
        return;
      // listenEvent
      case 1: {
        // New connection
        wil::unique_socket clientSocket {
          accept(mListeningSocket.get(), nullptr, nullptr)};
        if (!clientSocket) {
          LogError("accept failed with error: {}", WSAGetLastError());
          __debugbreak();
          continue;
        }
        Log("USB/IP connection established");

        wil::unique_event clientEvent {WSACreateEvent()};
        WSAEventSelect(
          clientSocket.get(), clientEvent.get(), FD_READ | FD_CLOSE);
        events.push_back(clientEvent.get());
        clientSockets.push_back(std::move(clientSocket));
        clientEvents.push_back(std::move(clientEvent));
        continue;
      }
      default: {
        const auto socketIdx = waitIdx - 2;
        const auto clientSocket = clientSockets.at(socketIdx).get();
        WSANETWORKEVENTS socketEvents {};
        if (const auto ret = WSAEnumNetworkEvents(
              clientSocket, events.at(waitIdx), &socketEvents);
            ret != 0) {
          __debugbreak();
        }
        switch (socketEvents.lNetworkEvents) {
          case 0:
            // kernel race condition; even though we've just been told there's a
            // read or a close on this socket, nope, there's not
            continue;
          case FD_CLOSE:
            events.erase(events.begin() + waitIdx);
            clientSockets.erase(clientSockets.begin() + socketIdx);
            clientEvents.erase(clientEvents.begin() + socketIdx);
            Log("Client disconnected");
            continue;
          case FD_READ:
            break;
          default:
            __debugbreak();
        }

        if (const auto hr = this->OnClientSocketActive(clientSocket);
            !SUCCEEDED(hr)) {
          if (hr == HRESULT_FROM_WIN32(WSAECONNRESET)) {
            Log("Client disconnected");
            continue;
          }
          LogError("Failed to handle client socket: {}", hr);
          __debugbreak();
        }
        continue;
      }
    }
  }
}

HRESULT FredEmmott_USBIP_VirtPP_Instance::OnClientSocketActive(
  const SOCKET clientSocket) {
  if (mClientSocket != INVALID_SOCKET) {
    LogError("Multiple clients active at the same time");
    return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
  }
  if (clientSocket == INVALID_SOCKET || !clientSocket) {
    LogError("Invalid client socket");
    return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);
  }

  mClientSocket = clientSocket;
  const auto forgetSocketAtExit
    = wil::scope_exit([this] { mClientSocket = INVALID_SOCKET; });

  union {
    USBIP::CommandCode mCommandCode {};
    USBIP::OP_REQ_DEVLIST mOP_REQ_DEVLIST;
    USBIP::OP_REQ_IMPORT mOP_REQ_IMPORT;
    USBIP::USBIP_CMD_SUBMIT mUSBIP_CMD_SUBMIT;
    USBIP::USBIP_CMD_UNLINK mUSBIP_CMD_UNLINK;
  } request;

  if (const auto ret = RecvAll(clientSocket, &request.mCommandCode); !ret) {
    std::println(stderr, "Client disconnected or recv failed.");
    return ret.error();
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
      if (const auto ret
          = RecvRemainder(clientSocket, &request.mOP_REQ_DEVLIST);
          !ret) [[unlikely]]
        return ret.error();
      return this->OnDevListOp();
    case USBIP::CommandCode::OP_REQ_IMPORT:
      Log("-> Received REQ_IMPORT");
      if (const auto ret = RecvRemainder(clientSocket, &request.mOP_REQ_IMPORT);
          !ret) [[unlikely]]
        return ret.error();
      return this->OnImportOp(request.mOP_REQ_IMPORT);
    case USBIP::CommandCode::USBIP_CMD_SUBMIT:
      // Not logging here, way too spammy :)
      if (const auto ret
          = RecvRemainder(clientSocket, &request.mUSBIP_CMD_SUBMIT);
          !ret) [[unlikely]]
        return ret.error();
      return this->OnSubmitRequest(request.mUSBIP_CMD_SUBMIT);
    case USBIP::CommandCode::USBIP_CMD_UNLINK: {
      Log("-> Received CMD_UNLINK");
      if (const auto ret
          = RecvRemainder(clientSocket, &request.mUSBIP_CMD_UNLINK);
          !ret) [[unlikely]]
        return ret.error();
      USBIP::USBIP_RET_UNLINK response {};
      response.mHeader.mSequenceNumber
        = request.mUSBIP_CMD_UNLINK.mHeader.mSequenceNumber;
      if (const auto ret = SendAll(clientSocket, response); !ret) [[unlikely]]
        return ret.error();
      return S_OK;
    }
    case USBIP::CommandCode::USBIP_RET_SUBMIT:
    case USBIP::CommandCode::USBIP_RET_UNLINK:
      LogError("-> Received a RET instead of a CMD");
      return HRESULT_FROM_WIN32(ERROR_BAD_COMMAND);
    default:
      LogError(
        "-> Unhandled USB/IP command code: 0x{:x}",
        std::to_underlying(request.mCommandCode));
      return HRESULT_FROM_WIN32(ERROR_BAD_COMMAND);
  }
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Instance::OnDevListOp() {
  const USBIP::OP_REP_DEVLIST header {
    .mNumDevices = static_cast<uint32_t>(std::ranges::fold_left(
      mBusses, 0, [](auto acc, const auto& bus) { return acc + bus.size(); })),
  };
  if (const auto ret = SendAll(mClientSocket, header); !ret)
    return ret.error();
  for (auto&& [busIdx, bus]: std::views::enumerate(mBusses)) {
    for (auto&& [deviceIdx, device]: std::views::enumerate(bus)) {
      const auto& config = device->mDescriptor;
      const auto usbipDevice = MakeUSBIPDevice(
        busIdx + 1, deviceIdx + 1, config, device->mInterfaces.size());
      if (const auto ret = SendAll(mClientSocket, usbipDevice); !ret)
        return ret.error();
      for (auto&& iface: device->mInterfaces) {
        const USBIP::Interface wireInterface {
          .mClass = iface.bInterfaceClass,
          .mSubClass = iface.bInterfaceSubClass,
          .mProtocol = iface.bInterfaceProtocol,
        };
        if (const auto ret = SendAll(mClientSocket, wireInterface); !ret)
          return ret.error();
      }
    }
  }

  return {};
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Instance::OnImportOp(
  const FredEmmott::USBIP::OP_REQ_IMPORT& request) {
  const std::string_view busId {request.mBusID};

  for (auto&& [busIdx, bus]: std::views::enumerate(mBusses)) {
    for (auto&& [deviceIdx, device]: std::views::enumerate(bus)) {
      if (busId != std::format("{}-{}", busIdx + 1, deviceIdx + 1))
        continue;

      const auto usbipDevice = MakeUSBIPDevice(
        busIdx + 1,
        deviceIdx + 1,
        device->mDescriptor,
        device->mInterfaces.size());

      return SendAll(
               mClientSocket, USBIP::OP_REP_IMPORT {.mDevice = usbipDevice})
        .error_or(S_OK);
    }
  }

  LogError("Failed to find device with busID '{}'", busId);
  USBIP::OP_REP_IMPORT reply {};
  reply.mHeader.mStatus = 1;// per spec, 1 for error
  return SendAll(mClientSocket, reply).error_or(S_OK);
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Instance::OnInputRequest(
  FredEmmott_USBIP_VirtPP_Device& device,
  const FredEmmott::USBIP::USBIP_CMD_SUBMIT& request,
  FredEmmott_USBIP_VirtPP_Request& apiRequest) {
  return device.mCallbacks.OnInputRequest(
    &apiRequest,
    request.mHeader.mEndpoint.NativeValue(),
    request.mSetup.mRequestType,
    request.mSetup.mRequest,
    request.mSetup.mValue,
    request.mSetup.mIndex,
    request.mSetup.mLength);
}

FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_Instance::OnOutputRequest(
  FredEmmott_USBIP_VirtPP_Device& device,
  const FredEmmott::USBIP::USBIP_CMD_SUBMIT& request,
  FredEmmott_USBIP_VirtPP_Request& apiRequest) {
  thread_local uint8_t buffer[1024] {};
  const auto dataLength = request.mTransferBufferLength.NativeValue();
  if (dataLength > std::size(buffer)) {
    __debugbreak();
  }
  if (dataLength > 0) {
    if (const auto ret = RecvAll(mClientSocket, buffer, dataLength); !ret)
      [[unlikely]] {
      return ret.error();
    }
  }

  return device.mCallbacks.OnOutputRequest(
    &apiRequest,
    request.mHeader.mEndpoint.NativeValue(),
    request.mSetup.mRequestType,
    request.mSetup.mRequest,
    request.mSetup.mValue,
    request.mSetup.mIndex,
    request.mSetup.mLength,
    dataLength ? buffer : nullptr,
    dataLength);
}

FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_Instance::OnSubmitRequest(
  const FredEmmott::USBIP::USBIP_CMD_SUBMIT& request) {
  const auto busIndex = (request.mHeader.mDeviceID.NativeValue() >> 16) - 1;
  const auto deviceIndex
    = (request.mHeader.mDeviceID.NativeValue() & 0xffff) - 1;
  if (busIndex >= mBusses.size() || deviceIndex >= mBusses[busIndex].size())
    [[unlikely]] {
    LogError(
      "Received submit request for invalid device: bus {}, device {}",
      busIndex,
      deviceIndex);
    return HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);
  }
  auto& device = *mBusses.at(busIndex).at(deviceIndex);
  FredEmmott_USBIP_VirtPP_Request apiRequest {
    .mDevice = &device,
    .mSocket = mClientSocket,
    .mSequenceNumber = request.mHeader.mSequenceNumber,
    .mTransferBufferLength = request.mTransferBufferLength.NativeValue(),
  };

  if (request.mHeader.mDirection == USBIP::Direction::In) {
    return OnInputRequest(device, request, apiRequest);
  }

  return OnOutputRequest(device, request, apiRequest);
}

FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_Instance::OnUnlinkRequest(
  const USBIP::USBIP_CMD_UNLINK& request) {
  USBIP::USBIP_RET_UNLINK response {};
  response.mHeader.mSequenceNumber = request.mHeader.mSequenceNumber;
  return SendAll(mClientSocket, response).error_or(S_OK);
}

void FredEmmott_USBIP_VirtPP_Instance::AutoAttach() {
  for (auto&& [i, bus]: std::views::enumerate(mBusses)) {
    for (auto&& [j, device]: std::views::enumerate(bus)) {
      if (!device->mAutoAttach) {
        continue;
      }
      const auto busID = std::format("{}-{}", i + 1, j + 1);
      Log("Auto-attaching device {}", busID);
      std::ignore = device->Attach(busID);
    }
  }
}

void FredEmmott_USBIP_VirtPP_Instance_RequestStop(
  const FredEmmott_USBIP_VirtPP_InstanceHandle handle) {
  handle->mStopSource.request_stop();
}

void FredEmmott_USBIP_VirtPP_Instance_Destroy(
  const FredEmmott_USBIP_VirtPP_InstanceHandle handle) {
  delete handle;
}

void* FredEmmott_USBIP_VirtPP_Instance_GetUserData(
  const FredEmmott_USBIP_VirtPP_InstanceHandle handle) {
  return handle->mInitData.mUserData;
}

void FredEmmott_USBIP_VirtPP_RequestStopInstance(
  FredEmmott_USBIP_VirtPP_InstanceHandle instance) {
  instance->mStopSource.request_stop();
}