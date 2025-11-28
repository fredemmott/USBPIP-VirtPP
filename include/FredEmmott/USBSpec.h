// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#ifdef _WIN32
#include "USBSpec/win32.h"
#else
#error "To add support for Linux, use the definitions in usb/ch9.h. Alternatively, use the libusb struct definitions"
#endif