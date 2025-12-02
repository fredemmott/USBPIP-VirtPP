// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include "Core.h"

#ifdef __cplusplus
#include <cinttypes>
extern "C" {
#else
#include <inttypes.h>
#endif

struct FredEmmott_USBIP_VirtPP_Mouse;
typedef FredEmmott_USBIP_VirtPP_Mouse* FredEmmott_USBIP_VirtPP_MouseHandle;

struct FredEmmott_USBIP_VirtPP_Mouse_InitData {
  void* mUserData;
  bool mAutoAttach;
};

struct FredEmmott_USBIP_VirtPP_Mouse_State {
  /**
   * bit 0 = left button
   * bit 1 = right button
   * bit 2 = middle button
   */
  uint8_t bmButtons;
  int8_t bDX;
  int8_t bDY;
  int8_t bDWheel;
};
static_assert(sizeof(FredEmmott_USBIP_VirtPP_Mouse_State) == 4, "Unexpected packing for Mouse state");

FredEmmott_USBIP_VirtPP_MouseHandle FredEmmott_USBIP_VirtPP_Mouse_Create(
  FredEmmott_USBIP_VirtPP_InstanceHandle,
  const FredEmmott_USBIP_VirtPP_Mouse_InitData*);
void FredEmmott_USBIP_VirtPP_Mouse_Destroy(
  FredEmmott_USBIP_VirtPP_MouseHandle);
void* FredEmmott_USBIP_VirtPP_Mouse_GetUserData(
  FredEmmott_USBIP_VirtPP_MouseHandle);

/* Update the state of a Mouse in-place. */
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Mouse_UpdateInPlace(
  FredEmmott_USBIP_VirtPP_MouseHandle,
  void* userData,
  void (*)(
    FredEmmott_USBIP_VirtPP_MouseHandle,
    void* userData,
    FredEmmott_USBIP_VirtPP_Mouse_State*));

/* Update state, copying the data. */
FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Mouse_SetState(
  FredEmmott_USBIP_VirtPP_MouseHandle,
  const FredEmmott_USBIP_VirtPP_Mouse_State*);

FredEmmott_USBIP_VirtPP_Result FredEmmott_USBIP_VirtPP_Mouse_Move(
  FredEmmott_USBIP_VirtPP_MouseHandle,
  int8_t dx,
  int8_t dy);


#ifdef __cplusplus
}// extern "C"
#endif
