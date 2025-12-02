// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <expected>

/* Client for v0.9.7.* of:
* - https://github.com/vadimgrn/usbip-win2
* - https://github.com/OSSign/vadimgrn--usbip-win2/
*
* This uses the IOCTL directly, not the provided SDK.
*
* I decided to directly use the IOCTL primarily because of the use of C++
* STL containers in the libusbip API definition; lesser concerns are the
* use of the C++ ABI more broadly as opposed to C API/ABI, and support for
* driver-only installs (e.g. some OSSign builds).
*
* I'm a big fan of the modern C++ features, but not for ABIs.
*
* I don't want to tie myself to the C++ language version used by the libusbip
* headers, and unless I ignored their DLL and built it myself, I'd still be
* tied to the C++ API/ABI.
*
* If there is a BC break, hopefully the GUIDs used to find usbip-win2 or
* the IOCTL number will also change. Otherwise apps using current versions of
* the project's SDK dlls will also be making incorrectly structured requests,
* which would also affect the "use the official SDK but build it myself" option.
*
* As I only care about one relatively-straightforward IOCTL, reimplementing it
* feels like the best balance of concerns for my needs.
*/
namespace FredEmmott::USBIP::Win2Client {
struct AttachError {
  enum class Stage {
    ArgumentValidation,
    Discovery,
    Open,
    IOControl,
    ResponseValidation,
  };
  Stage stage {Stage::ArgumentValidation};
  HRESULT hr {~0};
};

/* Attach the local USB-IP client (driver) to a server reachable on
 * INADDR_LOOPBACK.
 *
 * Returns the *USB* port number (starting at 1)
 */
[[nodiscard]]
std::expected<uint16_t, AttachError> Attach(
  uint16_t tcpPortNumber,
  char const* busID,
  std::size_t busIDLength);
}// namespace FredEmmott::USBIP::Win2Client