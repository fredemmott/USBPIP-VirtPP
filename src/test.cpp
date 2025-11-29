#include <FredEmmott/HIDSpec.h>
#include <FredEmmott/USBIP-VirtPP/Core.h>
#include <FredEmmott/USBIP-VirtPP/HIDDevice.h>

#include <functional>
#include <wil/resource.h>
#include <cwchar>

// HID Report Descriptor (Minimal Boot Mouse: Buttons (1 byte) + X (1 byte) + Y (1 byte))
const uint8_t ReportDescriptor[] = {
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

extern "C" FredEmmott_USBIP_VirtPP_Result OnGetInputReport(
  const FredEmmott_USBIP_VirtPP_RequestHandle request,
  const uint8_t /*reportId*/,
  const uint16_t /*expectedLength*/) {
  // Simple mouse report: no buttons, X=+1, Y=+1
  const uint8_t mouseReport[] = {0x00, 0x01, 0x01};
  return FredEmmott_USBIP_VirtPP_Request_SendReply(request, mouseReport);
}

int main() {
  // Avoid need for explicit flush after each println
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
  constexpr FredEmmott_USBIP_VirtPP_Instance_InitData instanceInit{};
  auto instance = FredEmmott_USBIP_VirtPP_Instance_Create(&instanceInit);
  const auto destroyInstance = wil::scope_exit(
    std::bind_front(&FredEmmott_USBIP_VirtPP_Instance_Destroy, instance));

  // Setup HID device init
  constexpr FredEmmott_USBIP_VirtPP_HIDDevice_USBDeviceData usbDevice{
    .mVendorID = 0x4242,
    .mProductID = 0x4242,
    .mDeviceVersion = 0x01'00,
    .mLanguage = L"\x0409",
    .mManufacturer = L"Fred",
    .mProduct = L"FakeMouse",
    .mInterface = L"FakeInterface",
    .mSerialNumber = L"1234",
  };
  constexpr FredEmmott_USBIP_VirtPP_HIDReportDescriptorReference reportDescriptor  {
    ReportDescriptor,
    sizeof(ReportDescriptor),
  };
  constexpr FredEmmott_USBIP_VirtPP_HIDDevice_InitData hidInit{
    .mCallbacks = {&OnGetInputReport},
    .mAutoAttach = true,
    .mUSBDeviceData = usbDevice,
    .mReportCount = 1,
    .mReportDescriptors = { reportDescriptor },
  };

  const auto hid = FredEmmott_USBIP_VirtPP_HIDDevice_Create(instance, &hidInit);
  const auto destroyHID = wil::scope_exit(
    std::bind_front(&FredEmmott_USBIP_VirtPP_HIDDevice_Destroy, hid));

  FredEmmott_USBIP_VirtPP_Instance_Run(instance);

  return 0;
}