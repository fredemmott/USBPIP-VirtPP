// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <FredEmmott/USBIP-VirtPP/Request.h>

#include <memory>

namespace FredEmmott::USBVirtPP {

using unique_request = std::unique_ptr<
  FredEmmott_USBIP_VirtPP_Request,
  decltype([](const auto h) {
    FredEmmott_USBIP_VirtPP_Request_Destroy(h);
  })>;

}// namespace FredEmmott::USBVirtPP