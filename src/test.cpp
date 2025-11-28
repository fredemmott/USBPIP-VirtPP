#include <FredEmmott/USBIP-VirtPP.h>
#include <FredEmmott/USBSpec.hpp>

#include <print>
#include <functional>
#include <future>
#include <wil/resource.h>

#pragma comment(lib, "ws2_32.lib")

namespace USB = FredEmmott::USBSpec;

namespace USBStrings {
enum class Indices : uint8_t {
  LangID = 0,
  Manufacturer,
  Product,
  SerialNumber,
  Interface,
};

constexpr auto LangID = USB::StringDescriptor{L"\x0409"};// en_US
constexpr auto Manufacturer = USB::StringDescriptor{L"Fred"};
constexpr auto Product = USB::StringDescriptor{L"FakeMouse"};
constexpr auto SerialNumber = USB::StringDescriptor{L"6969"};
constexpr auto Interface = USB::StringDescriptor{L"FakeInterface"};
};// namespace USBStrings

// HID Report Descriptor (Minimal Boot Mouse: Buttons (1 byte) + X (1 byte) + Y
// (1 byte))
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

constexpr FredEmmott_USBSpec_DeviceDescriptor MouseDeviceDescriptor{
  .bLength = FredEmmott_USBSpec_DeviceDescriptor_Size,
  .bDescriptorType = 0x01,// DEVICE
  .bcdUSB = 0x02'00,
  .bMaxPacketSize0 = 0x40,
  .idVendor = 0x4242,
  .idProduct = 0x4242,
  .bcdDevice = 0x01'00,
  .iManufacturer
  = std::to_underlying(USBStrings::Indices::Manufacturer),
  .iProduct = std::to_underlying(USBStrings::Indices::Product),
  .iSerialNumber
  = std::to_underlying(USBStrings::Indices::SerialNumber),
  .bNumConfigurations = 1,
};

constexpr struct MouseConfigurationDescriptor {
  FredEmmott_USBSpec_ConfigurationDescriptor mConfiguration{
    .bLength = FredEmmott_USBSpec_ConfigurationDescriptor_Size,
    .bDescriptorType = 0x02,
    .wTotalLength = sizeof(MouseConfigurationDescriptor),
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0x80 | 0x20,// bus-powered, remote wakeup
    .MaxPower = 0x32,// 100ma
  };
  FredEmmott_USBSpec_InterfaceDescriptor mInterface {
    .bLength = FredEmmott_USBSpec_InterfaceDescriptor_Size,
    .bDescriptorType = 0x04,
    .bNumEndpoints = 1,
    .bInterfaceClass = 3,// HID
    .iInterface = std::to_underlying(USBStrings::Indices::Interface),
  };
  USB::HIDDescriptor mHID{
    .mLength = sizeof(USB::HIDDescriptor) + sizeof(USB::HIDDescriptor::Report),
    .mHIDSpecVersion = 0x01'11,
    .mNumDescriptors = 1,
  };
  USB::HIDDescriptor::Report mHIDReportDescriptor{
    .mType = 0x22,// HID Report
    .mLength = sizeof(ReportDescriptor),
  };
  FredEmmott_USBSpec_EndpointDescriptor mEndpoint{
    .bLength = FredEmmott_USBSpec_EndPointDescriptor_Size,
    .bDescriptorType = 0x05,
    .bEndpointAddress = 0x80 | 0x01,// In, endpoint 1
    .bmAttributes = 0x03,// interrupt
    .wMaxPacketSize = 0x00'04,
    .bInterval = 0x0a,// 10ms polling
  };
} MouseConfigurationDescriptor;

extern "C" FredEmmott_USBIP_VirtPP_Result OnInputRequest(
  const FredEmmott_USBIP_VirtPP_RequestHandle handle,
  const uint32_t endpoint,
  const uint8_t requestType,
  const uint8_t request,
  const uint16_t value,
  const uint16_t index,
  const uint16_t length) {
  if (endpoint == 0) {
    std::println(
      "-> CMD_SUBMIT (Control EP0, Type: 0x{:x}, Req: 0x{:x}, Len: "
      "{})",
      requestType,
      request,
      length);

    constexpr uint8_t HostToDevice = 0x00;
    constexpr uint8_t DeviceToHost = 0x80;
    constexpr uint8_t DeviceToInterface = 0x81;

    constexpr uint8_t GetDescriptor = 0x06;

    if (requestType == DeviceToHost && request == GetDescriptor) {
      const auto descriptorType = (uint8_t)(value >> 8);
      const auto descriptorIndex = (uint8_t)(value & 0xFF);
      std::println(
        "Getting type {} index {}",
        descriptorType,
        descriptorIndex);

      if (descriptorType == 0x01) {
        std::println("   - Responding with DEVICE Descriptor.");
        return FredEmmott_USBIP_VirtPP_Request_SendReply(
          handle,
          MouseDeviceDescriptor);
      }
      if (descriptorType == 0x02) {
        std::println("   - Responding with CONFIGURATION Descriptor.");
        return FredEmmott_USBIP_VirtPP_Request_SendReply(
          handle,
          MouseConfigurationDescriptor);
      }
      if (descriptorType == 0x03) {
        // String Descriptor
        switch (static_cast<USBStrings::Indices>(descriptorIndex)) {
          case USBStrings::Indices::LangID:
            std::println("   - Responding with LANGID String Descriptor.");
            return FredEmmott_USBIP_VirtPP_Request_SendReply(
              handle,
              USBStrings::LangID);
          case USBStrings::Indices::Manufacturer:
            std::println(
              "   - Responding with MANUFACTURER String Descriptor.");
            return FredEmmott_USBIP_VirtPP_Request_SendReply(
              handle,
              USBStrings::Manufacturer);
          case USBStrings::Indices::Product:
            std::println("   - Responding with PRODUCT String Descriptor.");
            return FredEmmott_USBIP_VirtPP_Request_SendReply(
              handle,
              USBStrings::Product);
          case USBStrings::Indices::SerialNumber:
            std::println(
              "   - Responding with SERIALNUMBER String Descriptor.");
            return FredEmmott_USBIP_VirtPP_Request_SendReply(
              handle,
              USBStrings::SerialNumber);
          case USBStrings::Indices::Interface:
            std::println("   - Responding with INTERFACE String Descriptor.");
            return FredEmmott_USBIP_VirtPP_Request_SendReply(
              handle,
              USBStrings::Interface);
        }
        // TODO
        __debugbreak();
        return -1;
      }

      if (descriptorType == 0x22) {
        // HID Report Descriptor
        std::println("   - Responding with HID REPORT Descriptor bytes).");
        return FredEmmott_USBIP_VirtPP_Request_SendReply(
          handle,
          ReportDescriptor);
      }
    }
    if (requestType == DeviceToInterface && request == GetDescriptor) {
      const auto descriptorType = (uint8_t)(value >> 8);
      const auto descriptorIndex = (uint8_t)(value & 0xFF);
      if (descriptorType == 0x22) {
        // HID report
        std::println("   - Responding with HID REPORT Descriptor bytes.");
        return FredEmmott_USBIP_VirtPP_Request_SendReply(
          handle,
          ReportDescriptor);
      }
      // TODO
      __debugbreak();
      return -1;
    }

    // If it's a request we don't handle, fail gracefully
    std::println(
      stderr,
      "   - Error: Unhandled control request");
    __debugbreak();
    return -1;
  }

  if (endpoint == 1) {
    const uint8_t mouseReport[] = {0x00, 0x01, 0x01};
    return FredEmmott_USBIP_VirtPP_Request_SendReply(handle, mouseReport);
  }

  // Unhandled endpoint or direction (e.g., EP1 OUT, which is not a standard
  // mouse feature)
  std::println(
    stderr,
    "-> Error: Unhandled URB CMD");
  __debugbreak();
  return -1;
}

int main() {
  // Avoid need for explicit flush after each println
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
  const FredEmmott_USBIP_VirtPP_Instance_InitData instanceInit{
    .mPortNumber = 1337,
  };
  auto instance = FredEmmott_USBIP_VirtPP_Instance_Create(&instanceInit);
  const auto destroyInstance = wil::scope_exit(
    std::bind_front(&FredEmmott_USBIP_VirtPP_Instance_Destroy, instance));

  const FredEmmott_USBIP_VirtPP_Device_Callbacks callbacks{
    .OnInputRequest = &OnInputRequest,
  };
  const FredEmmott_USBIP_VirtPP_Device_InitData deviceInit{
    .mAutoAttach = true,
    .mCallbacks = &callbacks,
    .mDeviceDescriptor = &MouseDeviceDescriptor,
    .mNumInterfaces = 1,
    .mInterfaceDescriptors = &MouseConfigurationDescriptor.mInterface,
  };
  const auto device = FredEmmott_USBIP_VirtPP_Device_Create(
    instance,
    &deviceInit);
  const auto destroyDevice = wil::scope_exit(
    std::bind_front(&FredEmmott_USBIP_VirtPP_Device_Destroy, device));

  FredEmmott_USBIP_VirtPP_Instance_Run(instance);

  return 0;
}