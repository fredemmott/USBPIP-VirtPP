// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT

#include "detail-Mouse.hpp"
#include "detail.hpp"

#include <FredEmmott/USBIP-VirtPP/HIDDevice.h>
#include <FredEmmott/USBIP-VirtPP/Mouse.h>
#include <FredEmmott/USBIP-VirtPP/Request.h>

namespace {
constexpr uint8_t HIDReportDescriptor[] = {
  0x05, 0x01,// Usage Page (Generic Desktop Ctrls)
  0x09, 0x02,// Usage (Mouse)
  0xA1, 0x01,// Collection (Application)
  0x09, 0x01,//   Usage (Pointer)
  0xA1, 0x00,//   Collection (Physical)
  0x05, 0x09,//     Usage Page (Button)
  0x19, 0x01,//     Usage Minimum (0x01)
  0x29, 0x03,//     Usage Maximum (0x03)
  0x15, 0x00,//     Logical Minimum (0)
  0x25, 0x01,//     Logical Maximum (1)
  0x95, 0x03,//     Report Count (3)
  0x75, 0x01,//     Report Size (1)
  0x81, 0x02,//     Report Input (Data, Variable, Absolute)
  0x95, 0x05,//     Report Count (5)
  0x81, 0x03,//     Report Input (Constant, Variable, Absolute) ; padding
  0x05, 0x01,//     Usage Page (Generic Desktop Ctrls)
  0x09, 0x30,//     Usage (X)
  0x09, 0x31,//     Usage (Y)
  0x15, 0x81,//     Logical Minimum (-127)
  0x25, 0x7F,//     Logical Maximum (127)
  0x75, 0x08,//     Report Size (8)
  0x95, 0x02,//     Report Count (2)
  0x81, 0x06,//     Report Input (Data, Variable, Relative)
  0xC0,//   End Collection
  0xC0// End Collection
};
}

FredEmmott_USBIP_VirtPP_MouseHandle FredEmmott_USBIP_VirtPP_Mouse_Create(
  const FredEmmott_USBIP_VirtPP_InstanceHandle instance,
  const FredEmmott_USBIP_VirtPP_Mouse_InitData* initData) {
  if (!instance) {
    return nullptr;
  }
  if (!initData) {
    instance->LogError("Can't create Mouse without init data");
    return nullptr;
  }

  auto ret
    = std::make_unique<FredEmmott_USBIP_VirtPP_Mouse>(instance, *initData);
  if (ret->mHID) {
    return ret.release();
  }
  return nullptr;
}

void FredEmmott_USBIP_VirtPP_Mouse_Destroy(
  FredEmmott_USBIP_VirtPP_MouseHandle handle) {
  delete handle;
}

void* FredEmmott_USBIP_VirtPP_Mouse_GetUserData(
  FredEmmott_USBIP_VirtPP_MouseHandle handle) {
  return handle->mUserData;
}

FredEmmott_USBIP_VirtPP_Mouse::FredEmmott_USBIP_VirtPP_Mouse(
  FredEmmott_USBIP_VirtPP_InstanceHandle instance,
  const FredEmmott_USBIP_VirtPP_Mouse_InitData& initData)
  : mUserData(initData.mUserData) {
  constexpr FredEmmott_USBIP_VirtPP_HIDDevice_USBDeviceData usbDevice {
    .mVendorID = 0x1209,// pid.codes open source
    .mProductID = 0x0001,
    .mDeviceVersion = 0x0100,
    .mLanguage = L"\x0409",
    .mManufacturer = L"Fred Emmott",
    .mProduct = L"USBIP-VirtPP Virtual Mouse",
    .mInterface = L"USBIP-VirtPP Virtual Mouse",
    .mSerialNumber = L"1234",
  };
  const FredEmmott_USBIP_VirtPP_BlobReference reportDescriptorRef {
    HIDReportDescriptor,
    static_cast<uint16_t>(sizeof(HIDReportDescriptor)),
  };
  const FredEmmott_USBIP_VirtPP_HIDDevice_InitData hidInit {
    .mUserData = this,
    .mCallbacks = {&OnGetInputReport},
    .mAutoAttach = initData.mAutoAttach,
    .mUSBDeviceData = usbDevice,
    .mReportCount = 1,
    .mReportDescriptors = {reportDescriptorRef},
  };

  mHID = FredEmmott_USBIP_VirtPP_HIDDevice_Create(instance, &hidInit);
}

FredEmmott_USBIP_VirtPP_Mouse::~FredEmmott_USBIP_VirtPP_Mouse() {
  if (mHID) {
    FredEmmott_USBIP_VirtPP_HIDDevice_Destroy(mHID);
    mHID = nullptr;
  }
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Mouse_UpdateInPlace(
  FredEmmott_USBIP_VirtPP_MouseHandle handle,
  void* userData,
  void (*callback)(
    FredEmmott_USBIP_VirtPP_MouseHandle,
    void* userData,
    FredEmmott_USBIP_VirtPP_Mouse_State*)) {
  if (!handle) {
    return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
  }
  if (!callback) {
    return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
  }
  callback(handle, userData, &handle->mState);
  FredEmmott_USBIP_VirtPP_HIDDevice_MarkDirty(handle->mHID);
  return FredEmmott_USBIP_VirtPP_SUCCESS;
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Mouse_SetState(
  FredEmmott_USBIP_VirtPP_MouseHandle handle,
  const FredEmmott_USBIP_VirtPP_Mouse_State* state) {
  return FredEmmott_USBIP_VirtPP_Mouse_UpdateInPlace(
    handle,
    const_cast<FredEmmott_USBIP_VirtPP_Mouse_State*>(state),
    [](
      FredEmmott_USBIP_VirtPP_MouseHandle,
      void* stateIn,
      FredEmmott_USBIP_VirtPP_Mouse_State* stateOut) {
      memcpy(stateOut, stateIn, sizeof(FredEmmott_USBIP_VirtPP_Mouse_State));
    });
}

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Mouse::OnGetInputReport(
  const FredEmmott_USBIP_VirtPP_RequestHandle request,
  const uint8_t /*reportId*/,
  const uint16_t /*expectedLength*/) {
  auto* self = static_cast<FredEmmott_USBIP_VirtPP_Mouse*>(
    FredEmmott_USBIP_VirtPP_Request_GetHIDDeviceUserData(request));
  if (!self) {
    return -1;
  }
  const auto reply
    = FredEmmott_USBIP_VirtPP_Request_SendReply(request, self->mState);
  if (FredEmmott_USBIP_VirtPP_SUCCEEDED(reply)) {
    self->mState.bDX = 0;
    self->mState.bDY = 0;
  }
  return reply;
}
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Mouse_Move(
  const FredEmmott_USBIP_VirtPP_MouseHandle self,
  const int8_t dx,
  const int8_t dy) {
  using userData_t = std::tuple<int8_t, int8_t>;
  userData_t userData { dx, dy };

  return FredEmmott_USBIP_VirtPP_Mouse_UpdateInPlace(
    self,
    &userData,
    [](
      FredEmmott_USBIP_VirtPP_MouseHandle,
      void* userDataIn,
      FredEmmott_USBIP_VirtPP_Mouse_State* stateOut) {
      const auto [dx, dy] = *static_cast<const userData_t*>(userDataIn);
      stateOut->bDX += dx;
      stateOut->bDY += dy;
    });
}