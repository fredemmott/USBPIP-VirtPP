#include <wil/resource.h>

#include <FredEmmott/USBIP-VirtPP/Core.h>
#include <FredEmmott/USBIP-VirtPP/Mouse.h>
#include <FredEmmott/USBIP-VirtPP/XPad.h>

#include <functional>
#include <future>
#include <numbers>

namespace {

constexpr bool AutoAttachEnabled = false;
constexpr bool AllowRemoteConnections = true;

namespace Mouse {
auto Create(const FredEmmott_USBIP_VirtPP_InstanceHandle instance) {
  constexpr FredEmmott_USBIP_VirtPP_Mouse_InitData init {
    .mAutoAttach = AutoAttachEnabled,
  };
  return FredEmmott_USBIP_VirtPP_Mouse_Create(instance, &init);
}
}// namespace Mouse

namespace XPad {
auto Create(const FredEmmott_USBIP_VirtPP_InstanceHandle instance) {
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

  const auto mouse = Mouse::Create(instance);
  const auto destroyMouse = wil::scope_exit(
    std::bind_front(&FredEmmott_USBIP_VirtPP_Mouse_Destroy, mouse));
  const auto xpad = XPad::Create(instance);
  const auto destroyXPad = wil::scope_exit(
    std::bind_front(&FredEmmott_USBIP_VirtPP_XPad_Destroy, xpad));

  const auto feeder = std::async([mouse, xpad]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      FredEmmott_USBIP_VirtPP_Mouse_UpdateInPlace(
        mouse,
        nullptr,
        [](
          FredEmmott_USBIP_VirtPP_MouseHandle,
          void*,
          FredEmmott_USBIP_VirtPP_Mouse_State* state) {
          state->bDX += 1;
          state->bDY += 1;
          state->bDWheel += 1;
        });
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