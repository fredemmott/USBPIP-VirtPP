// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "detail-RequestType.hpp"
#include "detail-XPad.hpp"
#include "detail.hpp"

#include <FredEmmott/USBIP-VirtPP/XPad.h>

#include <chrono>

namespace {
enum class Interface : uint8_t {
  Gamepad = 0,
};
enum class Endpoint : uint8_t {
  Control = 0,
  GamepadIn = 1,
  GamepadOut = 2,
};
enum class StringIndex : uint8_t {
  LangID = 0,
  // These values are required by XUSB spec
  Manufacturer = 1,
  Product = 2,
  SerialNumber = 3,
  MSOS = 0xEE,
};
}// namespace

const FredEmmott_USBSpec_DeviceDescriptor&
FredEmmott_USBIP_VirtPP_XPad::GetDeviceDescriptor() {
  static constexpr FredEmmott_USBSpec_DeviceDescriptor ConstDescriptor {
    .bLength = sizeof(ConstDescriptor),
    .bDescriptorType = 0x01,
    .bcdUSB = 0x02'00,
    .bDeviceClass = 0xFF,
    .bDeviceSubClass = 0xFF,
    .bDeviceProtocol = 0xFF,
    .bMaxPacketSize0 = 0x08,
    .idVendor = 0x1209,// pid.codes open source
    .idProduct = 0x0003,
    .bcdDevice = 0x01'00,
    .iManufacturer = std::to_underlying(StringIndex::Manufacturer),
    .iProduct = std::to_underlying(StringIndex::Product),
    .iSerialNumber = std::to_underlying(StringIndex::SerialNumber),
    .bNumConfigurations = 1,
  };
  return ConstDescriptor;
}

#pragma pack(push, 1)
struct FredEmmott_USBIP_VirtPP_XPad::ConfigurationDescriptor {
  FredEmmott_USBSpec_ConfigurationDescriptor mConfigurationDescriptor {
    .bLength = FredEmmott_USBSpec_ConfigurationDescriptor_Size,
    .bDescriptorType = 0x02,
    .wTotalLength = sizeof(ConfigurationDescriptor),
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .bmAttributes = 0b1010'0000,
    .MaxPower = 0x32,
  };
  FredEmmott_USBSpec_InterfaceDescriptor mGamepadInterface {
    .bLength = FredEmmott_USBSpec_InterfaceDescriptor_Size,
    .bDescriptorType = 0x04,
    .bInterfaceNumber = std::to_underlying(Interface::Gamepad),
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = 0xFF,// Vendor-specific
    .bInterfaceSubClass = 0x5D,// XUSB
    .bInterfaceProtocol = 0x01,// XUSB GamePad
  };
  struct XUSBInterfaceDescriptor_t {
    static constexpr uint8_t InputReportCount = 0x03;
    static constexpr uint8_t OutputReportCount = 0x03;
    uint8_t bLength = sizeof(XUSBInterfaceDescriptor_t);
    uint8_t bDescriptorType = 0x21;
    uint16_t bcdXUSB = 0x01'00;
    uint8_t bDeviceSubtype = 0x01;// "Wired game controller"
    uint16_t wReports0 = 0x8100 | 0x20 | InputReportCount;
    uint8_t bReportSize0[InputReportCount] = {
      sizeof(GamepadInputReport),
      sizeof(GamepadLEDStatusReport),
      sizeof(GamepadRumbleLevelStatusReport),
    };
    uint16_t wReports = 0x0200 | 0x10 | OutputReportCount;
    uint8_t bReportSize1[OutputReportCount] = {
      sizeof(GamepadRumbleMotorControlReport),
      sizeof(GamepadLEDControlReport),
      sizeof(GamepadRumbleLevelControlReport),
    };
  } mGamepadXUSBInterface;
  FredEmmott_USBSpec_EndpointDescriptor mGamepadEndpointIn {
    .bLength = sizeof(mGamepadEndpointIn),
    .bDescriptorType = 0x05,
    .bEndpointAddress = 0x81,
    .bmAttributes = 0x03,
    .wMaxPacketSize = 0x0020,
    .bInterval = 0x04,
  };
  FredEmmott_USBSpec_EndpointDescriptor mGamepadEndpointOut {
    .bLength = sizeof(mGamepadEndpointOut),
    .bDescriptorType = 0x05,
    .bEndpointAddress = 0x02,
    .bmAttributes = 0x03,
    .wMaxPacketSize = 0x0020,
    .bInterval = 0x08,
  };
};
#pragma pack(pop)
const FredEmmott_USBIP_VirtPP_XPad::ConfigurationDescriptor&
FredEmmott_USBIP_VirtPP_XPad::GetConfigurationDescriptor() {
  static constexpr ConfigurationDescriptor ConstDescriptor {};
  return ConstDescriptor;
}

FredEmmott_USBIP_VirtPP_XPadHandle FredEmmott_USBIP_VirtPP_XPad_Create(
  const FredEmmott_USBIP_VirtPP_InstanceHandle instance,
  const FredEmmott_USBIP_VirtPP_XPad_InitData* initData) {
  if (!instance) {
    return nullptr;
  }
  if (!initData) {
    instance->LogError("Can't create XPad without instance");
    return nullptr;
  }
  auto ret
    = std::make_unique<FredEmmott_USBIP_VirtPP_XPad>(instance, *initData);
  if (ret->mUSBDevice) {
    return ret.release();
  }
  return nullptr;
}

void FredEmmott_USBIP_VirtPP_XPad_Destroy(
  FredEmmott_USBIP_VirtPP_XPadHandle handle) {
  delete handle;
}

void* FredEmmott_USBIP_VirtPP_XPad_GetUserData(
  FredEmmott_USBIP_VirtPP_XPadHandle handle) {
  return handle->mUserData;
}

FredEmmott_USBIP_VirtPP_XPad::FredEmmott_USBIP_VirtPP_XPad(
  FredEmmott_USBIP_VirtPP_InstanceHandle instance,
  const FredEmmott_USBIP_VirtPP_XPad_InitData& initData)
  : mUserData(initData.mUserData), mInstance(instance) {
  const FredEmmott_USBIP_VirtPP_Device_InitData usbDeviceInit {
    .mUserData = this,
    .mCallbacks = {&OnUSBInputRequestCallback, &OnUSBOutputRequestCallback},
    .mAutoAttach = initData.mAutoAttach,
    .mDeviceDescriptor = &GetDeviceDescriptor(),
    .mNumInterfaces = 1,
    .mInterfaceDescriptors = &GetConfigurationDescriptor().mGamepadInterface,
  };
  mUSBDevice = FredEmmott_USBIP_VirtPP_Device_Create(instance, &usbDeviceInit);
  if (!mUSBDevice) {
    return;
  }
}

FredEmmott_USBIP_VirtPP_XPad::~FredEmmott_USBIP_VirtPP_XPad() {
  FredEmmott_USBIP_VirtPP_Device_Destroy(mUSBDevice);
}

FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_XPad::OnControlInputRequest(
  FredEmmott_USBIP_VirtPP_RequestHandle request,
  uint8_t rawRequestType,
  uint8_t requestCode,
  uint16_t value,
  uint16_t index,
  uint16_t length) {
  const auto [direction, requestType, recipient]
    = RequestType::Parse(rawRequestType);
  using enum RequestType::Direction;
  using enum RequestType::Type;
  using enum RequestType::Recipient;
  if (requestType == Standard) {
    switch (requestCode) {
      case 0x00:// GET_STATUS
        return FredEmmott_USBIP_VirtPP_Request_SendReply(request, uint16_t {});
      case 0x06: /* GET_DESCRIPTOR */ {
        const auto descriptorType = static_cast<uint8_t>(value >> 8);
        const auto descriptorIndex = static_cast<uint8_t>(value & 0xff);
        switch (descriptorType) {
          case 0x01:// DEVICE
            return FredEmmott_USBIP_VirtPP_Request_SendReply(
              request, GetDeviceDescriptor());
          case 0x02:// CONFIGURATION
            return FredEmmott_USBIP_VirtPP_Request_SendReply(
              request, GetConfigurationDescriptor());
          case 0x03:// STRING
            switch (static_cast<StringIndex>(descriptorIndex)) {
              case StringIndex::LangID:
                return FredEmmott_USBIP_VirtPP_Request_SendStringReply(
                  request, L"\x0409");// en_US
              case StringIndex::Manufacturer:
                return FredEmmott_USBIP_VirtPP_Request_SendStringReply(
                  request, L"Fred Emmott");
              case StringIndex::Product:
                // Required for interoperability with some older games
                return FredEmmott_USBIP_VirtPP_Request_SendStringReply(
                  request, L"XBOX 360 For Windows");
              case StringIndex::SerialNumber:
                return FredEmmott_USBIP_VirtPP_Request_SendStringReply(
                  request, L"1234");
              case StringIndex::MSOS: {
#pragma pack(push, 1)
                constexpr struct MSOSReply_t {
                  uint8_t bLength = sizeof(MSOSReply_t);
                  uint8_t bDescriptorType = 0x03;// STRING
                  uint16_t qrSignature[7] = {'M', 'S', 'F', 'T', '1', '0', '0'};
                  uint8_t bVendorCode = 0x04;
                  uint8_t bPad = 0x00;
                } MSOSReply;
#pragma pack(pop)
                static_assert(sizeof(MSOSReply_t) == 0x12);
                return FredEmmott_USBIP_VirtPP_Request_SendReply(
                  request, MSOSReply);
              }
              default:
                __debugbreak();
            }
          case 0x06:// DEVICE_QUALIFIER
          case 0x0F:// BOS
            return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(
              request, -EPIPE);
          default:
            __debugbreak();
        }
        return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, -EPIPE);
      }
      default:
        __debugbreak();
    }
  }

  if (requestType == Vendor) {
    if (
      requestCode == 0x04 /* vendorCode from string */
      && index == 0x04 /* Compatible ID */) {
      // MS OS Compatible ID
#pragma pack(push, 1)
      constexpr struct reply_t {
        uint32_t dwLength = sizeof(reply_t);
        uint16_t bcdVersion = 0x0100;
        uint16_t wIndex = 0x0004;
        uint8_t bCount = 0x01;
        uint8_t reserved0[7] {};
        uint8_t bFirstInterfaceNumber = 0x00;
        uint8_t bNumInterfaces = 1;
        char compatibleId[8] {'X', 'U', 'S', 'B', '1', '0', '\0', '\0'};
        char subCompatibleID[8] {};
        uint8_t reserved1[6] {};
      } reply;
#pragma pack(pop)
      static_assert(reply.dwLength == 0x28);
      return FredEmmott_USBIP_VirtPP_Request_SendReply(request, reply);
    }
    if (recipient == Device && requestCode == 0x01) {
      const auto lol = reinterpret_cast<uintptr_t>(this);
      // High nibble of LSB is reserved
      const auto serial = ((lol >> 32) ^ lol) & 0xffff'ff0f;
      mInstance->Log("XPad serial number: {:#010x}", serial);
      return FredEmmott_USBIP_VirtPP_Request_SendReply(request, serial);
    }
    mInstance->Log(
      "Unhandled vendor control input request {:#04x}/{:#04x}",
      rawRequestType,
      requestCode);
    return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, -EPIPE);
  }
  __debugbreak();
  return -1;
}
FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_XPad::OnControlOutputRequest(
  FredEmmott_USBIP_VirtPP_RequestHandle request,
  uint8_t rawRequestType,
  uint8_t requestCode,
  uint16_t value,
  uint16_t index,
  uint16_t length,
  const void* data,
  uint32_t dataLength) {
  using enum RequestType::Direction;
  using enum RequestType::Type;
  using enum RequestType::Recipient;
  const auto [urbDirection, requestType, recipient]
    = RequestType::Parse(rawRequestType);

  if (requestType == Standard) {
    switch (requestCode) {
      case 0x09:// SET_CONFIGURATION no-op, we only support 1 configuration
      case 0x0A:// SET_IDLE: no-op
        // Not actually an error with code 0
        return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, 0);
      default:
        return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, -EPIPE);
    }
  }

  return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, -EPIPE);
}

FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_XPad::OnGamepadInputRequest(
  FredEmmott_USBIP_VirtPP_RequestHandle request,
  uint8_t rawRequestType,
  uint8_t requestCode,
  uint16_t value,
  uint16_t index,
  uint16_t length) {
  const auto [direction, requestType, recipient]
    = RequestType::Parse(rawRequestType);
  using enum RequestType::Direction;
  using enum RequestType::Type;
  using enum RequestType::Recipient;
  if (rawRequestType == 0 && requestCode == 0) {
    // Not really documented in the spec: with all zeroes, we can return
    // multiple reports. Just return everything.
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count();
    mXUSBReport.mGamepadInputReport.mState.bmButtons = 0;
    if (now % 2) {
      mXUSBReport.mGamepadInputReport.mState.bmButtons
        |= FredEmmott_USBIP_VirtPP_XPad_Button_A;
    }
    mXUSBReport.mGamepadInputReport.mState.bLeftTrigger = now % 0xff;

    return FredEmmott_USBIP_VirtPP_Request_SendReply(request, mXUSBReport);
  }
  return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, -EPIPE);
}

FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_XPad::OnGamepadOutputRequest(
  FredEmmott_USBIP_VirtPP_RequestHandle request,
  uint8_t rawRequestType,
  uint8_t requestCode,
  uint16_t value,
  uint16_t index,
  uint16_t length,
  const void* data,
  uint32_t dataLength) {
  if (dataLength < 3) {
    __debugbreak();
    return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, -EPIPE);
  }
  union Report {
    uint8_t bReportID {};
    GamepadRumbleMotorControlReport mRumbleMotors;
    GamepadLEDStatusReport mLEDs;
    GamepadRumbleLevelControlReport mRumbleLevel;
  };
  const Report& report = *static_cast<const Report*>(data);
  switch (report.bReportID) {
    case 0x00:// rumble
      // TODO
      return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, 0);
    case 0x01:// LEDS
      mInstance->Log("XPad LED state changed to {:#04x}", report.mLEDs.mState);
      mXUSBReport.mGamepadLEDStatusReport.mState = report.mLEDs.mState;
      return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, 0);
    case 0x02:// rumble level
      mInstance->Log("XPad rumble level changed to {:#04x}", report.mRumbleLevel.mState);
      mXUSBReport.mGamepadRumbleLevelStatusReport.mState = report.mRumbleLevel.mState;
      return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, 0);
  }
  return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, -EPIPE);
}

FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_XPad::OnUSBInputRequestCallback(
  const FredEmmott_USBIP_VirtPP_RequestHandle request,
  const uint32_t endpoint,
  const uint8_t requestType,
  const uint8_t requestCode,
  const uint16_t value,
  const uint16_t index,
  const uint16_t length) {
  auto& self
    = *static_cast<FredEmmott_USBIP_VirtPP_XPad*>(request->mDevice->mUserData);
  switch (static_cast<Endpoint>(endpoint)) {
    case Endpoint::Control:
      return self.OnControlInputRequest(
        request, requestType, requestCode, value, index, length);
    case Endpoint::GamepadIn:
      return self.OnGamepadInputRequest(
        request, requestType, requestCode, value, index, length);
    default:
      __debugbreak();
      return -1;
  }
}
FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_XPad::OnUSBOutputRequestCallback(
  FredEmmott_USBIP_VirtPP_RequestHandle request,
  const uint32_t endpoint,
  const uint8_t requestType,
  const uint8_t requestCode,
  const uint16_t value,
  const uint16_t index,
  const uint16_t length,
  const void* data,
  const uint32_t dataLength) {
  std::println(
    "XPad received OUTPUT request for EP {}: {:#04x}/{:#04x} - {} bytes",
    endpoint,
    requestType,
    requestCode,
    dataLength);
  auto& self
    = *static_cast<FredEmmott_USBIP_VirtPP_XPad*>(request->mDevice->mUserData);
  switch (static_cast<Endpoint>(endpoint)) {
    case Endpoint::Control:
      return self.OnControlOutputRequest(
        request,
        requestType,
        requestCode,
        value,
        index,
        length,
        data,
        dataLength);
    case Endpoint::GamepadOut:
      return self.OnGamepadOutputRequest(
        request,
        requestType,
        requestCode,
        value,
        index,
        length,
        data,
        dataLength);
    default:
      __debugbreak();
      return -1;
  }
}
