#include <wil/resource.h>

#include <FredEmmott/HIDSpec.h>
#include <FredEmmott/USBIP-VirtPP/Core.h>
#include <FredEmmott/USBIP-VirtPP/HIDDevice.h>
#include <FredEmmott/USBIP-VirtPP/XPad.h>

#include <functional>
#include <future>
#include <numbers>

namespace {

constexpr bool AutoAttachEnabled = false;
constexpr bool AllowRemoteConnections = true;

namespace Mouse {
// HID Report Descriptor (minimal mouse: Buttons (1 byte) + X (1 byte) + Y (1
// byte))
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

FredEmmott_USBIP_VirtPP_Result OnGetInputReport(
  const FredEmmott_USBIP_VirtPP_RequestHandle request,
  const uint8_t /*reportId*/,
  const uint16_t /*expectedLength*/) {
  // No-op mouse report: no buttons, X += 0, Y += 0
  const uint8_t mouseReport[] = {0x00, 0x00, 0x00};
  return FredEmmott_USBIP_VirtPP_Request_SendReply(request, mouseReport);
}

FredEmmott_USBIP_VirtPP_HIDDeviceHandle Create(
  FredEmmott_USBIP_VirtPP_InstanceHandle instance) {
  constexpr FredEmmott_USBIP_VirtPP_HIDDevice_USBDeviceData usbDevice {
    .mVendorID = 0x1209,// pid.codes open source
    .mProductID = 0x001,// Test PID 1
    .mDeviceVersion = 0x01'00,
    .mLanguage = L"\x0409",
    .mManufacturer = L"Fred",
    .mProduct = L"FakeMouse",
    .mInterface = L"FakeInterface",
    .mSerialNumber = L"1234",
  };
  constexpr FredEmmott_USBIP_VirtPP_BlobReference reportDescriptor {
    ReportDescriptor,
    sizeof(ReportDescriptor),
  };
  constexpr FredEmmott_USBIP_VirtPP_HIDDevice_InitData hidInit {
    .mCallbacks = {&OnGetInputReport},
    .mAutoAttach = AutoAttachEnabled,
    .mUSBDeviceData = usbDevice,
    .mReportCount = 1,
    .mReportDescriptors = {reportDescriptor},
  };

  return FredEmmott_USBIP_VirtPP_HIDDevice_Create(instance, &hidInit);
}
}// namespace Mouse

namespace GamePad {
// Verbatim from XInputHID Driver Documentation, included in the zip from
// https://aka.ms/gipdocs
constexpr uint8_t ReportDescriptor[] = {
  // clang-format off
  0x05, 0x01,       /* Usage Page (Generic Desktop) */
  0x09, 0x05,       /* Usage (Gamepad) */\
  0xA1, 0x01,       /* Collection (Application) */
  0x85, 0x01,       /*  Report ID (1 / keystroke) */

   /* This HID report is used for Windows and non-Windows OS.*/
  /* Left Thumbstick */
  0x09, 0x01,       /* Usage (Pointer) */
  0xA1, 0x00,       /* Collection (Physical) */
  0x09, 0x30,       /* Usage (X) */
  0x09, 0x31,       /* Usage (Y) */
  0x15, 0x00,       /* Logical Min (0) */
  0x27, 0xFF, 0xFF, 0x00, 0x00, /* Logical Max (0xFFFF) */
  0x95, 0x02,       /* Report Count (2) */
  0x75, 0x10,       /* Report Size (16) */
  0x81, 0x02,       /* Input (Data,Var,Abs) */
  0xC0,                   /* End Collection (Thumbstick) */

  /* Right Thumbstick */
  0x09, 0x01,       /* Usage (Pointer) */
  0xA1, 0x00,       /* Collection (Physical) */
  0x09, 0x32,       /* Usage (Z)  X and Y for Right thumbstick (16-bit) */
  0x09, 0x35,       /* Usage (Rz) */
  0x15, 0x00,       /* Logical Min (0) */
  0x27, 0xFF, 0xFF, 0x00, 0x00,  /* Logical Max (0xFFFF) */\
  0x95, 0x02,       /* Report Count (2) */
  0x75, 0x10,       /* Report Size (16) */\
  0x81, 0x02,       /* Input (Data,Var,Abs) */
  0xC0,                   /* End Collection (Thumbstick) */

  /* Left Trigger */
  0x05, 0x02,       /* Usage Page (Simulation Controls) */
  0x09, 0xC5,       /* Usage (Brake) */
  0x15, 0x00,       /* Logical Min (0) */
  0x26, 0xFF, 0x03, /* Logical Max (0x3FF) */
  0x95, 0x01,       /* Report Count (1) */
  0x75, 0x0A,       /* Report Size (10) */
  0x81, 0x02,       /* Input (Data,Var,Abs) */
  /* Padding 6 bits */\
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x00,       /* Logical Max (0) */
  0x75, 0x06,       /* Report Size (6) */
  0x95, 0x01,       /* Report Count (1) */
  0x81, 0x03,       /* Input (Constant) */

  /* Right Trigger */
  0x05, 0x02,       /* Usage Page (Simulation Controls) */
  0x09, 0xC4,       /* Usage (Accelerator)  */
  0x15, 0x00,       /* Logical Min (0) */
  0x26, 0xFF, 0x03, /* Logical Max (0x03FF) */
  0x95, 0x01,       /* Report Count (1) */
  0x75, 0x0A,       /* Report Size (10) */
  0x81, 0x02,       /* Input (Data,Var,Abs) */
  /* Padding 6 bits */
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x00,       /* Logical Max (0) */
  0x75, 0x06,       /* Report Size (6) */
  0x95, 0x01,       /* Report Count (1) */
  0x81, 0x03,       /* Input (Constant) */

  /* Hat Switch(DPAD)  */
  0x05, 0x01,       /* Usage Page (Generic Desktop) */
  0x09, 0x39,       /* Usage (Hat Switch) DPAD  */
  0x15, 0x01,       /* Logical Min (1)   Windows XInput requires 0 for center position. */
  0x25, 0x08,       /* Logical Max (8)   4 bits for 8 pressed position in order of "Right  Left  Down  Up" */
                                 /* Logical Min (0)   Some partners use 8 for center based on Angular units.  */
                                 /* Logical Max (7)   0 - 7 for 8 pressed position based on angular 45 degree for each step.  */
  0x35, 0x00,       /* Physical Min (0)  Logical values from 0 to 10    ===  bit0   bit2  bit1  bit0   */
  0x46, 0x3B, 0x01, /* Physical Max (315) */
  0x66, 0x14, 0x00, /* Unit (English Rotation: Angular Position [degrees]) */
  0x75, 0x04,       /* Report Size (4)  --- 4 buttons */
  0x95, 0x01,       /* Report Count (1) */
  0x81, 0x42,       /* Input (Variable, Null) (DPAD) */
  /* Padding 4 bits  */
  0x75, 0x04,       /* Report Size (4) */
  0x95, 0x01,       /* Report Count (1) */
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x00,       /* Logical Max (0) */
  0x35, 0x00,       /* Undefine Physical Min/Max (0) */
  0x45, 0x00,       /* Undefine Physical Min/Max (0) */
  0x65, 0x00,       /* Undefine Unit (None) */
  0x81, 0x03,       /* Input (Constant) */

  /* 15 Digital Buttons: A, B, X, Y, LShd(LeftShoulder)/LB(Left bumper), RShd/RB, */
  /*  View/BACK, Menu/START, LSB(Left stick button)/(Thumbstick Pressed), RSB, LThStk,  RThStk*/
  0x05, 0x09,       /* Usage Page (Button) */
  0x19, 0x01,       /* Usage Min (Button 1)  */
  0x29, 0x0F,       /* Usage Max (Button 15) */
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x01,       /* Logical Max (1) */
  0x75, 0x01,       /* Report Size (1) */
  0x95, 0x0F,       /* Report Count (15) */
  0x81, 0x02,       /* Input (Data,Var,Abs) */
  /* Padding 1 bits for D-Pad, Leftshoulder, RightShoulder */
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x00,       /* Logical Max (0) */
  0x75, 0x01,       /* Report Size (1) */
  0x95, 0x01,       /* Report Count (1) */
  0x81, 0x03,       /* Input (constant) */
  /* Share button */
  0x05, 0x0C,       /* Usage Page(Consumer) */
  0x0A, 0xB2, 0x00, /* Usage(Record) -- Share button */
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x01,       /* Logical Max (1) */
  0x95, 0x01,       /* Report Count (1) */
  0x75, 0x01,       /* Report Size (1) */
  0x81, 0x02,       /* Input (Data,Var,Abs) */
  /* Padding 7 bits */
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x00,       /* Logical Max (0) */
  0x75, 0x07,       /* Report Size (7) */
  0x95, 0x01,       /* Report Count (1) */
  0x81, 0x03,       /* Input (Constant) */

  /*********** Report#2 (Output): Xbox One Gamepad Rumble Motor ************/
  0x05, 0x0F,  /* Usage Page (Physical Interface) */
  0x09, 0x21,  /* Usage (Set Effect Report) */
  0x85, 0x02,    /*Report ID (2) */

  0xA1, 0x02,       /* Collection (Logical) */
  0x09, 0x97,       /* Usage (DC enable actuators) */
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x01,       /* Logical Max (1) */
  0x75, 0x04,       /* Report Size (4) */
  0x95, 0x01,       /* Report Count(1) */
  0x91, 0x02,       /* Output (Data,Var,Abs) */
  /* Padding 4 bits */
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x00,       /* Logical Max (0) */
  0x75, 0x04,       /* Report Size (4) */
  0x95, 0x01,       /* Report Count (1) */
  0x91, 0x03,       /* Output (Constant) */

  0x09, 0x70,       /* Usage (Magnitude in percentage) // Left Impulse, Right Impulse, Left Rumble, Right Rumble */
  0x15, 0x00,       /* Logical Min (0) */
  0x25, 0x64,       /* Logical Max (100) */
  0x75, 0x08,       /* Report Size (8) */
  0x95, 0x04,       /* Report Count (4) */
  0x91, 0x02,       /* Output (Data,Var,Abs) */

  0x09, 0x50,       /* Usage (Duration) On time duration, in steps of 10ms. */
  0x66, 0x01, 0x10, /* Unit time seconds */
  0x55, 0x0E,       /* Unit Exponent -2 for 10mS */
  0x15, 0x00,       /* Logical Min (0) */
  0x26, 0xFF, 0x00, /* Logical Max (255)  */
  0x75, 0x08,       /* Report Size (8) */
  0x95, 0x01,       /* Report Count (1)  */
  0x91, 0x02,       /* Output (Data,Var,Abs) */

  0x09, 0xA7,       /* Usage (Start Delay) On time duration, in steps of 10ms. */
  0x15, 0x00,       /* Logical Min (0) */
  0x26, 0xFF, 0x00, /* Logical Max (255)   */
  0x75, 0x08,       /* Report Size (8) */
  0x95, 0x01,       /* Report Count (1)  */
  0x91, 0x02,       /* Output (Data,Var,Abs)  */
  0x65, 0x00,       /* Undefine Unit (None) */
  0x55, 0x00,       /* Undefine Exponent (None) */

  0x09, 0x7C,       /* Usage (Loop Count)  Number of times to repeat.  0 = run once. */
  0x15, 0x00,       /* Logical Min (0) */
  0x26, 0xFF, 0x00, /* Logical Max (255)  */
  0x75, 0x08,       /* Report Size (8) */
  0x95, 0x01,       /* Report Count (1) */
  0x91, 0x02,       /* Output (Data,Var,Abs)  */
  0xC0, /* End Collection (Rumble)  */
  0xC0, /* End Collection (HID)  */
  // clang-format on
};

enum class Buttons : uint16_t {
  A = 1 << 0,
  B = 1 << 1,
  X = 1 << 2,
  Y = 1 << 3,
  LB = 1 << 4,
  RB = 1 << 5,
  View = 1 << 6,
  Back = View,
  Menu = 1 << 7,
  Start = Menu,
  LeftStick = 1 << 8,
  RightStick = 1 << 9,
  // LThumbStk = 1 << 10 // ???
  // RThumbStk = 1 << 11 // ???
  // ??? = 1 << 12
  // ??? = 1 << 13
  // ??? = 1 << 14
  // ??? = 1 << 15
};
constexpr Buttons operator|(const Buttons lhs, const Buttons rhs) {
  return static_cast<Buttons>(
    std::to_underlying(lhs) | std::to_underlying(rhs));
}

enum class DPad : uint8_t {
  Right = (1 << 0),
  Left = (1 << 1),
  Down = (1 << 2),
  Up = (1 << 3),
};
constexpr DPad operator|(const DPad lhs, const DPad rhs) {
  return static_cast<DPad>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

#pragma pack(push, 1)
struct InputReport {
  uint8_t mReportID;
  uint16_t mLeftStickX;
  uint16_t mLeftStickY;
  uint16_t mRightStickX;// a.k.a. Z
  uint16_t mRightStickY;// a.k.a. Rz
  uint16_t mLeftTrigger : 10;
  uint16_t mLeftTriggerPadding : 6;
  uint16_t mRightTrigger : 10;
  uint16_t mRightTriggerPadding : 6;
  // Center 0, button bits: R, L, D, U
  DPad mDPad : 4;
  uint8_t mDPadPadding : 4;
  Buttons mButtons : 15;
  uint16_t mButtonPadding : 1;
  uint8_t mShareButton : 1;
  uint8_t mShareButtonPadding : 7;
};
#pragma pack(pop)

FredEmmott_USBIP_VirtPP_Result OnGetInputReport(
  const FredEmmott_USBIP_VirtPP_RequestHandle request,
  const uint8_t /*reportID*/,
  const uint16_t /*expectedLength*/) {
  InputReport report {
    .mReportID = 0x01,// a requested ID of 0 or 1 are both fine
    .mDPad = DPad::Up,
    .mButtons = Buttons::A | Buttons::View,
  };
  return FredEmmott_USBIP_VirtPP_Request_SendReply(
    request, &report, sizeof(report));
}

FredEmmott_USBIP_VirtPP_HIDDeviceHandle Create(
  FredEmmott_USBIP_VirtPP_InstanceHandle instance) {
  constexpr FredEmmott_USBIP_VirtPP_HIDDevice_USBDeviceData usbDevice {
    .mVendorID = 0x1209,// pid.codes open source
    .mProductID = 0x002,// Test PID 2
    .mDeviceVersion = 0x01'00,
    .mLanguage = L"\x0409",
    .mManufacturer = L"Fred",
    .mProduct = L"FakeGamePad",
    .mInterface = L"FakeGPInterface",
    .mSerialNumber = L"3039373030323736323435313533",
  };
  constexpr FredEmmott_USBIP_VirtPP_BlobReference reportDescriptor {
    ReportDescriptor,
    sizeof(ReportDescriptor),
  };
  constexpr FredEmmott_USBIP_VirtPP_HIDDevice_InitData hidInit {
    .mCallbacks = {&OnGetInputReport, nullptr},
    .mAutoAttach = AutoAttachEnabled,
    .mUSBDeviceData = usbDevice,
    .mReportCount = 1,
    .mReportDescriptors = {reportDescriptor},
  };

  return FredEmmott_USBIP_VirtPP_HIDDevice_Create(instance, &hidInit);
}
}// namespace GamePad
namespace XPad {
FredEmmott_USBIP_VirtPP_XPadHandle Create(
  const FredEmmott_USBIP_VirtPP_InstanceHandle instance) {
  constexpr FredEmmott_USBIP_VirtPP_XPad_InitData init {
    .mAutoAttach = AutoAttachEnabled,
  };
  return FredEmmott_USBIP_VirtPP_XPad_Create(instance, &init);
}
}// namespace XPad
}// namespace

int main() {
  // Avoid need for explicit flush after each println
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
  constexpr FredEmmott_USBIP_VirtPP_Instance_InitData instanceInit {
    .mPortNumber = 1337,
    .mAllowRemoteConnections = AllowRemoteConnections,
  };
  auto instance = FredEmmott_USBIP_VirtPP_Instance_Create(&instanceInit);
  const auto destroyInstance = wil::scope_exit(
    std::bind_front(&FredEmmott_USBIP_VirtPP_Instance_Destroy, instance));

  const auto mouse = wil::scope_exit(
    std::bind_front(
      &FredEmmott_USBIP_VirtPP_HIDDevice_Destroy, Mouse::Create(instance)));
  const auto gamepad = wil::scope_exit(
    std::bind_front(
      &FredEmmott_USBIP_VirtPP_HIDDevice_Destroy, GamePad::Create(instance)));
  const auto xpad = XPad::Create(instance);
  const auto destroyXPad = wil::scope_exit(
    std::bind_front(&FredEmmott_USBIP_VirtPP_XPad_Destroy, xpad));

  const auto feeder = std::async([xpad]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      FredEmmott_USBIP_VirtPP_XPad_UpdateInPlace(
        xpad,
        nullptr,
        [](
          FredEmmott_USBIP_VirtPP_XPadHandle,
          void*,
          FredEmmott_USBIP_VirtPP_XPad_State* state) {
          const auto now
            = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();
          constexpr auto radius = 32767;
          const auto angle = (now / 10.0) * std::numbers::pi_v<float> / 180.0;
          state->wThumbLeftX = static_cast<int16_t>(radius * cos(angle));
          state->wThumbLeftY = static_cast<int16_t>(radius * sin(angle));
        });
    }
  });

  FredEmmott_USBIP_VirtPP_Instance_Run(instance);

  return 0;
}