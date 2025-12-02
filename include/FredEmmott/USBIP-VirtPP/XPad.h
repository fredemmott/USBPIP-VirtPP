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
typedef struct FredEmmott_USBIP_VirtPP_XPad* FredEmmott_USBIP_VirtPP_XPadHandle;

struct FredEmmott_USBIP_VirtPP_XPad_Callbacks {
  /* The left and right motors are difference sizes.
   *
   * The left motor is the low-frequency motor (big), the right is the the
   * low-frequency motor (small)
   */
  void (*OnRumble)(
    FredEmmott_USBIP_VirtPP_XPadHandle,
    uint16_t big,
    uint16_t small);
};
struct FredEmmott_USBIP_VirtPP_XPad_InitData {
  void* mUserData;
  struct FredEmmott_USBIP_VirtPP_XPad_Callbacks mCallbacks;

  BOOL mAutoAttach;
};

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
  int16_t wThumbLeftX;
  int16_t wThumbLeftY;
  int16_t wThumbRightX;
  int16_t wThumbRightY;
};
static_assert(
  sizeof(struct FredEmmott_USBIP_VirtPP_XPad_State) == 12,
  "Incorrect size for XUSB gamepad state");

FredEmmott_USBIP_VirtPP_XPadHandle FredEmmott_USBIP_VirtPP_XPad_Create(
  FredEmmott_USBIP_VirtPP_InstanceHandle,
  const struct FredEmmott_USBIP_VirtPP_XPad_InitData*);
void FredEmmott_USBIP_VirtPP_XPad_Destroy(FredEmmott_USBIP_VirtPP_XPadHandle);
void* FredEmmott_USBIP_VirtPP_XPad_GetUserData(
  FredEmmott_USBIP_VirtPP_XPadHandle);

/* Update the state of an XPad in-place.
 *
 * userData does not need to match the userData from the initData
 *
 * You should try to only call this if you are definitely going to modify the
 * data.
 */
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_XPad_UpdateInPlace(
  FredEmmott_USBIP_VirtPP_XPadHandle,
  void* userData,
  void (*)(
    FredEmmott_USBIP_VirtPP_XPadHandle,
    void* userData,
    struct FredEmmott_USBIP_VirtPP_XPad_State*));

/** Update state, copying the data.
 *
 * This is a bit simpler to use, but less efficient.
 *
 * You should try to only call this if the data has actually changed.
 */
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_XPad_SetState(
  FredEmmott_USBIP_VirtPP_XPadHandle,
  const struct FredEmmott_USBIP_VirtPP_XPad_State*);

#ifdef __cplusplus
}// extern "C"
#endif