// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "detail-hid.hpp"
#include "detail.hpp"
#include "detail-RequestType.hpp"

#include <FredEmmott/HIDSpec.h>
#include <FredEmmott/USBIP-VirtPP/Device.h>
#include <FredEmmott/USBIP-VirtPP/HIDDevice.h>
#include <FredEmmott/USBSpec.h>

#include <algorithm>
#include <print>
#include <vector>

namespace {
enum class StringIndex : uint8_t {
  LangID = 0,
  Manufacturer = 1,
  Product = 2,
  SerialNumber = 3,
  Interface = 4,
};
}// namespace

FredEmmott_USBIP_VirtPP_HIDDeviceHandle
FredEmmott_USBIP_VirtPP_HIDDevice_Create(
  FredEmmott_USBIP_VirtPP_InstanceHandle instance,
  const FredEmmott_USBIP_VirtPP_HIDDevice_InitData* init) {
  if (!instance) {
    return nullptr;
  }
  if (!init) {
    instance->LogError("HIDDevice_InitData is required");
    return nullptr;
  }
  auto ret
    = std::make_unique<FredEmmott_USBIP_VirtPP_HIDDevice>(instance, *init);
  if (ret->mUSBDevice) {
    return ret.release();
  }
  return nullptr;
}

FredEmmott_USBIP_VirtPP_HIDDevice::FredEmmott_USBIP_VirtPP_HIDDevice(
  FredEmmott_USBIP_VirtPP_InstanceHandle instance,
  const FredEmmott_USBIP_VirtPP_HIDDevice_InitData& init)
  : mInstance(instance), mInit(init) {
  if (init.mReportCount == 0) {
    instance->LogError("HIDDevice_InitData.mReportCount must be > 0");
    return;
  }

  InitializeDescriptors();

  const FredEmmott_USBIP_VirtPP_Device_InitData usbDeviceInit {
    .mUserData = this,
    .mCallbacks = {&OnUSBInputRequestCallback},
    .mAutoAttach = init.mAutoAttach,
    .mDeviceDescriptor = &mDeviceDescriptor,
    .mNumInterfaces = 1,
    .mInterfaceDescriptors = mInterfaceDescriptors,
  };

  mUSBDevice = FredEmmott_USBIP_VirtPP_Device_Create(instance, &usbDeviceInit);
  if (!mUSBDevice) {
    return;
  }
}

void FredEmmott_USBIP_VirtPP_HIDDevice_Destroy(
  const FredEmmott_USBIP_VirtPP_HIDDeviceHandle handle) {
  delete handle;
}

FredEmmott_USBIP_VirtPP_HIDDevice::~FredEmmott_USBIP_VirtPP_HIDDevice() {
  if (mUSBDevice) {
    FredEmmott_USBIP_VirtPP_Device_Destroy(mUSBDevice);
    mUSBDevice = nullptr;
  }
}

void FredEmmott_USBIP_VirtPP_HIDDevice::InitializeDeviceDescriptor() {
  mDeviceDescriptor = {
    .bLength = FredEmmott_USBSpec_DeviceDescriptor_Size,
    .bDescriptorType = 0x01,
    .bcdUSB = 0x02'00,
    .bDeviceClass = 0x03,
    .bMaxPacketSize0 = 0x40,
    .idVendor = mInit.mUSBDeviceData.mVendorID,
    .idProduct = mInit.mUSBDeviceData.mProductID,
    .bcdDevice = mInit.mUSBDeviceData.mDeviceVersion,
    .iManufacturer = std::to_underlying(StringIndex::Manufacturer),
    .iProduct = std::to_underlying(StringIndex::Product),
    .iSerialNumber = std::to_underlying(StringIndex::SerialNumber),
    .bNumConfigurations = 1,
  };
}

// Builds descriptors inside wrapper
void FredEmmott_USBIP_VirtPP_HIDDevice::InitializeDescriptors() {
  InitializeDeviceDescriptor();

  constexpr uint8_t configurationSize
    = FredEmmott_USBSpec_ConfigurationDescriptor_Size;
  constexpr uint8_t interfacesSize
    = FredEmmott_USBSpec_InterfaceDescriptor_Size;
  constexpr uint8_t hidDescriptorsSize
    = sizeof(FredEmmott_HIDSpec_HIDDescriptor);
  const uint8_t hidReportsSize
    = sizeof(FredEmmott_HIDSpec_HIDDescriptor_ReportDescriptor)
    * mInit.mReportCount;
  constexpr uint8_t endpointsSize = FredEmmott_USBSpec_EndPointDescriptor_Size * 2;
  const uint16_t totalSize = configurationSize + interfacesSize
    + hidDescriptorsSize + hidReportsSize + endpointsSize;

  mConfigurationDescriptorBlob.resize(totalSize);
  std::byte* cursor = mConfigurationDescriptorBlob.data();

  const auto configuration = mConfigurationDescriptor
    = reinterpret_cast<FredEmmott_USBSpec_ConfigurationDescriptor*>(cursor);
  cursor += configurationSize;
  const auto interfaces = mInterfaceDescriptors
    = reinterpret_cast<FredEmmott_USBSpec_InterfaceDescriptor*>(cursor);
  cursor += interfacesSize;
  const auto hidDescriptors
    = reinterpret_cast<FredEmmott_HIDSpec_HIDDescriptor*>(cursor);
  cursor += hidDescriptorsSize;
  const auto hidReports
    = reinterpret_cast<FredEmmott_HIDSpec_HIDDescriptor_ReportDescriptor*>(
      cursor);
  cursor += hidReportsSize;
  const auto endpoints
    = reinterpret_cast<FredEmmott_USBSpec_EndpointDescriptor*>(cursor);

  *configuration = {
    .bLength = FredEmmott_USBSpec_ConfigurationDescriptor_Size,
    .bDescriptorType = 0x02,// CONFIGURATION
    .wTotalLength = static_cast<uint16_t>(totalSize),
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = 1,
    .bmAttributes = 0x80 | 0x20,// bus-powered, remote wake
    .MaxPower = 0x32,// 100mA
  };

  interfaces[0] = {
    .bLength = FredEmmott_USBSpec_InterfaceDescriptor_Size,
    .bDescriptorType = 0x04,// INTERFACE
    .bNumEndpoints = 2,
    .bInterfaceClass = 3,// HID
    .iInterface = 4,// string index
  };

  *hidDescriptors = {
    .bLength = static_cast<uint8_t>(hidDescriptorsSize + hidReportsSize),
    .bDescriptorType = 0x21,
    .bcdHID = 0x01'11,
    .bNumDescriptors = mInit.mReportCount,
  };

  mHIDReportDescriptors.reserve(mInit.mReportCount);
  for (uint8_t i = 0; i < mInit.mReportCount; ++i) {
    hidReports[i] = {
      .bDescriptorType = 0x22,// HID Report
      .wDescriptorLength = mInit.mReportDescriptors[i].mByteCount,
    };
    mHIDReportDescriptors.emplace_back(
      mInit.mReportDescriptors[i].mData,
      mInit.mReportDescriptors[i].mByteCount);
  }

  endpoints[0] = {
    .bLength = FredEmmott_USBSpec_EndPointDescriptor_Size,
    .bDescriptorType = 0x05,// ENDPOINT
    .bEndpointAddress = 0x80 | 0x01,// IN, EP1
    .bmAttributes = 0x03,// Interrupt
    .wMaxPacketSize = 0x08,
    .bInterval = 0x0A,// 10ms
  };
  endpoints[1] = {
    .bLength = FredEmmott_USBSpec_EndPointDescriptor_Size,
    .bDescriptorType = 0x05,// ENDPOINT
    .bEndpointAddress = 0x02,// OUT, EP2
    .bmAttributes = 0x03,// Interrupt
    .wMaxPacketSize = 0x04,// 64
    .bInterval = 0x0A,// 10ms
  };
}

HRESULT FredEmmott_USBIP_VirtPP_HIDDevice::OnUSBInputRequest(
  const FredEmmott_USBIP_VirtPP_RequestHandle request,
  const uint32_t endpoint,
  const uint8_t rawRequestType,
  const uint8_t requestCode,
  const uint16_t value,
  const uint16_t index,
  const uint16_t length) {
  using enum RequestType::Direction;
  using enum RequestType::Type;
  using enum RequestType::Recipient;

  const auto [direction, requestType, recipient]
    = RequestType::Parse(rawRequestType);
  // EP0 control requests
  if (endpoint == 0) {

    if (requestType == Standard && requestCode == 0x00 /* GET_STATUS */) {
      return FredEmmott_USBIP_VirtPP_Request_SendReply(request, uint8_t {});
    }

    constexpr uint8_t GetDescriptor = 0x06;
    if (
      direction == DeviceToHost && requestType == Standard
      && requestCode == GetDescriptor) {
      const auto descriptorType = static_cast<uint8_t>(value >> 8);
      const auto descriptorIndex = static_cast<uint8_t>(value & 0xff);
      switch (descriptorType) {
        case 0x01:// DEVICE
          std::println(stdout, "-> DEVICE descriptor ({})", length);
          return FredEmmott_USBIP_VirtPP_Request_SendReply(
            request, mDeviceDescriptor);
        case 0x02: {
          std::println(stdout, "-> CONFIGURATION descriptor ({})", length);
          // CONFIGURATION
          return FredEmmott_USBIP_VirtPP_Request_SendReply(
            request,
            mConfigurationDescriptorBlob.data(),
            mConfigurationDescriptorBlob.size());
        }
        case 0x03: {
          std::println(
            stdout,
            "-> STRING descriptor ({} bytes, id {})",
            length,
            descriptorIndex);
          // STRING
          using enum StringIndex;
          const auto sendString
            = [request]<std::size_t N>(const wchar_t(&buf)[N]) {
                return FredEmmott_USBIP_VirtPP_Request_SendStringReply(
                  request, buf, wcsnlen_s(buf, N));
              };
          switch (static_cast<StringIndex>(descriptorIndex)) {
            case LangID:// LangID list
              return sendString(mInit.mUSBDeviceData.mLanguage);
            case Manufacturer:
              return sendString(mInit.mUSBDeviceData.mManufacturer);
            case Product:
              return sendString(mInit.mUSBDeviceData.mProduct);
            case SerialNumber:
              return sendString(mInit.mUSBDeviceData.mSerialNumber);
            case Interface:
              return sendString(mInit.mUSBDeviceData.mInterface);
          }
          // Indicate a stall by sending EPIPE
          mInstance->Log(
            "Unrecognized USB string descriptor index: {}", descriptorIndex);
          return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(
            request, -EPIPE);
        }
        case 0x22: {
          // HID descriptor
          std::println(stdout, "-> HID report descriptor ({})", length);
          // Report descriptor
          const auto [data, size] = mHIDReportDescriptors.at(descriptorIndex);
          return FredEmmott_USBIP_VirtPP_Request_SendReply(request, data, size);
        }
        default:
          mInstance->Log(
            "Unrecognized USB device descriptor type: {:#02x}", descriptorType);
          return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(
            request, -EPIPE);
      }
    }

    // Class GET_REPORT (IN)
    if (
      direction == DeviceToHost && requestType == Class
      && recipient == Interface && requestCode == 0x01) {
      const auto reportType = static_cast<uint8_t>(value >> 8);
      const auto reportId = static_cast<uint8_t>(value & 0xff);
      if (reportType == 0x01) {
        // INPUT
        if (mInit.mCallbacks.OnGetInputReport) {
          return mInit.mCallbacks.OnGetInputReport(request, reportId, length);
        }
      }
      // Not supported
      __debugbreak();
      return -1;
    }

    // Other control requests not handled here
    // Microsoft "Extended CompatID OS descriptor
    if (rawRequestType == 0xC0 && requestCode == 0x04) {
      return FredEmmott_USBIP_VirtPP_Request_SendErrorReply(request, -EPIPE);
    }
    __debugbreak();
    return -1;
  }

  // Interrupt IN endpoint (EP1 IN)
  if (endpoint == 1) {
    if (mInit.mCallbacks.OnGetInputReport) {
      return mInit.mCallbacks.OnGetInputReport(request, 0, length);
    }
    __debugbreak();
    return -1;
  }

  __debugbreak();
  return -1;
}

FredEmmott_USBIP_VirtPP_Result
FredEmmott_USBIP_VirtPP_HIDDevice::OnUSBInputRequestCallback(
  const FredEmmott_USBIP_VirtPP_RequestHandle request,
  const uint32_t endpoint,
  const uint8_t requestType,
  const uint8_t requestCode,
  const uint16_t value,
  const uint16_t index,
  const uint16_t length) {
  auto* usbDevice = FredEmmott_USBIP_VirtPP_Request_GetDevice(request);
  auto* obj = static_cast<FredEmmott_USBIP_VirtPP_HIDDevice*>(
    FredEmmott_USBIP_VirtPP_Device_GetUserData(usbDevice));
  return static_cast<FredEmmott_USBIP_VirtPP_Result>(obj->OnUSBInputRequest(
    request, endpoint, requestType, requestCode, value, index, length));
}

void* FredEmmott_USBIP_VirtPP_HIDDevice_GetUserData(
  const FredEmmott_USBIP_VirtPP_HIDDeviceHandle handle) {
  return handle->mInit.mUserData;
}

FredEmmott_USBIP_VirtPP_DeviceHandle
FredEmmott_USBIP_VirtPP_HIDDevice_GetUSBDevice(
  const FredEmmott_USBIP_VirtPP_HIDDeviceHandle handle) {
  return handle->mUSBDevice;
}

FredEmmott_USBIP_VirtPP_InstanceHandle
FredEmmott_USBIP_VirtPP_HIDDevice_GetInstance(
  const FredEmmott_USBIP_VirtPP_HIDDeviceHandle handle) {
  return handle->mInstance;
}

void* FredEmmott_USBIP_VirtPP_HIDDevice_GetInstanceUserData(
  const FredEmmott_USBIP_VirtPP_HIDDeviceHandle handle) {
  return handle->mInstance->mInitData.mUserData;
}

FredEmmott_USBIP_VirtPP_HIDDeviceHandle
FredEmmott_USBIP_VirtPP_Request_GetHIDDevice(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle) {
  return static_cast<FredEmmott_USBIP_VirtPP_HIDDevice*>(
    handle->mDevice->mUserData);
}

void* FredEmmott_USBIP_VirtPP_Request_GetHIDDeviceUserData(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle) {
  return static_cast<FredEmmott_USBIP_VirtPP_HIDDevice*>(
           handle->mDevice->mUserData)
    ->mInit.mUserData;
}