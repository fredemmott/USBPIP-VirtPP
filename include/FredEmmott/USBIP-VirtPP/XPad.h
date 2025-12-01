// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include "Core.h"
#include "Request.h"

#ifdef __cplusplus
#include <cinttypes>
extern "C" {
#else
#include <inttypes.h>
#endif
struct FredEmmott_USBIP_VirtPP_XPad;
typedef FredEmmott_USBIP_VirtPP_XPad* FredEmmott_USBIP_VirtPP_XPadHandle;

struct FredEmmott_USBIP_VirtPP_XPad_Callbacks {};
struct FredEmmott_USBIP_VirtPP_XPad_InitData {
  void* mUserData;
  FredEmmott_USBIP_VirtPP_XPad_Callbacks mCallbacks;

  bool mAutoAttach;
};

FredEmmott_USBIP_VirtPP_XPadHandle FredEmmott_USBIP_VirtPP_XPad_Create(
  FredEmmott_USBIP_VirtPP_InstanceHandle,
  const FredEmmott_USBIP_VirtPP_XPad_InitData*);
void FredEmmott_USBIP_VirtPP_XPad_Destroy(FredEmmott_USBIP_VirtPP_XPadHandle);
void* FredEmmott_USBIP_VirtPP_XPad_GetUserData(
  FredEmmott_USBIP_VirtPP_XPadHandle);

enum FredEmmott_USBIP_VirtPP_XPad_Buttons : uint16_t {
  FredEmmott_USBIP_VirtPP_XPad_Button_DPadUp = (1 << 0),
  FredEmmott_USBIP_VirtPP_XPad_Button_DPadDown = (1 << 1),
  FredEmmott_USBIP_VirtPP_XPad_Button_DPadLeft = (1 << 2),
  FredEmmott_USBIP_VirtPP_XPad_Button_DPadRight = (1 << 3),
  FredEmmott_USBIP_VirtPP_XPad_Button_Start = (1 << 4),
  FredEmmott_USBIP_VirtPP_XPad_Button_Back = (1 << 5),
  FredEmmott_USBIP_VirtPP_XPad_Button_LeftThumb = (1 << 6),
  FredEmmott_USBIP_VirtPP_XPad_Button_RightThumb = (1 << 7),
  FredEmmott_USBIP_VirtPP_XPad_Button_LeftShoulder = (1 << 8),
  FredEmmott_USBIP_VirtPP_XPad_Button_RightShoulder = (1 << 9),
  /* The MS XUSB spec calls the 'Xe' button instead of the Guide button.
   *
   * Perhaps a reference to the "Xenon" codename. */
  FredEmmott_USBIP_VirtPP_XPad_Button_Guide = (1 << 10),
  /* If you press the bind button while the controller's plugged in, the
   * OS can set up the wireless pairing over the XUSB protocol */
  FredEmmott_USBIP_VirtPP_XPad_Button_Binding = (1 << 11),
  FredEmmott_USBIP_VirtPP_XPad_Button_A = (1 << 12),
  FredEmmott_USBIP_VirtPP_XPad_Button_B = (1 << 13),
  FredEmmott_USBIP_VirtPP_XPad_Button_X = (1 << 14),
  FredEmmott_USBIP_VirtPP_XPad_Button_Y = (1 << 15),
};

struct FredEmmott_USBIP_VirtPP_XPad_State {
  uint16_t bmButtons;
  uint8_t bLeftTrigger;
  uint8_t bRightTrigger;
  uint16_t wThumbLeftX;
  uint16_t wThumbLeftY;
  uint16_t wThumbRightX;
  uint16_t wThumbRightY;
};
static_assert(sizeof(FredEmmott_USBIP_VirtPP_XPad_State) == 12);

#ifdef __cplusplus
}// extern "C"
#endif