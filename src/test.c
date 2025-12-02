#include <FredEmmott/USBIP-VirtPP/Core.h>
#include <FredEmmott/USBIP-VirtPP/HIDDevice.h>
#include <FredEmmott/USBIP-VirtPP/Mouse.h>
#include <FredEmmott/USBIP-VirtPP/XPad.h>

#define AUTO_ATTACH FALSE
#define ALLOW_REMOTE_CONNECTIONS TRUE

#include <Windows.h>

#include <math.h>
#include <stdio.h>

struct state_t {
  FredEmmott_USBIP_VirtPP_InstanceHandle mInstance;
  FredEmmott_USBIP_VirtPP_MouseHandle mMouse;
  FredEmmott_USBIP_VirtPP_XPadHandle mXPad;
};

void FeedMouse(
  FredEmmott_USBIP_VirtPP_MouseHandle handle,
  void* userData,
  struct FredEmmott_USBIP_VirtPP_Mouse_State* state) {
  state->bDX += 1;
  state->bDY += 1;
  state->bDWheel += 1;
}

void FeedXPad(
  FredEmmott_USBIP_VirtPP_XPadHandle handle,
  void* userData,
  struct FredEmmott_USBIP_VirtPP_XPad_State* state) {
  LARGE_INTEGER perfFrequency;
  LARGE_INTEGER perfCount;
  QueryPerformanceFrequency(&perfFrequency);
  QueryPerformanceCounter(&perfCount);
  // 100ms units
  uint64_t now = perfCount.QuadPart / (perfFrequency.QuadPart / 10);
  const float angle = (float)(now * 3.1415926 / 180.0f);
  state->wThumbLeftX = (int16_t)(32767 * cos(angle));
  state->wThumbLeftY = (int16_t)(32767 * sin(angle));
}

DWORD WINAPI FeedAll(void* userData) {
  struct state_t* state = userData;
  while (1) {
    Sleep(100 /* ms */);
    FredEmmott_USBIP_VirtPP_Mouse_UpdateInPlace(
      state->mMouse, NULL, &FeedMouse);
    FredEmmott_USBIP_VirtPP_XPad_UpdateInPlace(state->mXPad, NULL, &FeedXPad);
  }
  return 0;
}

int main() {
  // Avoid need for explicit flush after each println
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  struct state_t state = {};

  const struct FredEmmott_USBIP_VirtPP_Instance_InitData instanceInit = {
    .mPortNumber = 1337,
    .mAllowRemoteConnections = ALLOW_REMOTE_CONNECTIONS,
  };
  state.mInstance = FredEmmott_USBIP_VirtPP_Instance_Create(&instanceInit);

  struct FredEmmott_USBIP_VirtPP_Mouse_InitData mouseInit = {
    .mAutoAttach = AUTO_ATTACH,
  };
  state.mMouse
    = FredEmmott_USBIP_VirtPP_Mouse_Create(state.mInstance, &mouseInit);

  struct FredEmmott_USBIP_VirtPP_XPad_InitData xpadInit = {
    .mAutoAttach = AUTO_ATTACH,
  };
  state.mXPad = FredEmmott_USBIP_VirtPP_XPad_Create(state.mInstance, &xpadInit);

  HANDLE feederThread = CreateThread(NULL, 0, &FeedAll, &state, 0, NULL);

  FredEmmott_USBIP_VirtPP_Instance_Run(state.mInstance);

  WaitForSingleObject(feederThread, INFINITE);
  CloseHandle(feederThread);

  FredEmmott_USBIP_VirtPP_XPad_Destroy(state.mXPad);
  FredEmmott_USBIP_VirtPP_Mouse_Destroy(state.mMouse);
  FredEmmott_USBIP_VirtPP_Instance_Destroy(state.mInstance);

  return 0;
}