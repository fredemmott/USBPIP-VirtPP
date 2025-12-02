// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <FredEmmott/USBIP-VirtPP/Core.h>

#include <concepts>
#include <format>
#include <print>

namespace FredEmmott::USBVirtPP {

using Logger
  = decltype(FredEmmott_USBIP_VirtPP_Instance_Callbacks::OnLogMessage);

template <class T>
concept logging_target = requires(T v) {
  { GetLoggerCallback(v) } -> std::convertible_to<Logger>;
};

template <logging_target TTarget, class... Args>
void LogWithSeverity(
  TTarget&& target,
  const int severity,
  std::format_string<Args...> fmt,
  Args&&... args) {
  if (const auto callback = GetLoggerCallback(std::forward<TTarget>(target));
      callback) {
    const auto formatted = std::format(fmt, std::forward<Args>(args)...);
    callback(severity, formatted.c_str(), formatted.size());
  } else {
    std::println(
      (severity >= FredEmmott_USBIP_VirtPP_LogSeverity_Error) ? stderr : stdout,
      fmt,
      std::forward<Args>(args)...);
  }
}

template <logging_target TTarget, class... Args>
void LogError(
  TTarget&& target,
  std::format_string<Args...> fmt,
  Args&&... args) {
  LogWithSeverity(
    std::forward<TTarget>(target),
    FredEmmott_USBIP_VirtPP_LogSeverity_Error,
    fmt,
    std::forward<Args>(args)...);
}

template <logging_target TTarget, class... Args>
void Log(TTarget&& target, std::format_string<Args...> fmt, Args&&... args) {
  LogWithSeverity(
    std::forward<TTarget>(target),
    FredEmmott_USBIP_VirtPP_LogSeverity_Default,
    fmt,
    std::forward<Args>(args)...);
}
}// namespace FredEmmott::USBVirtPP

FredEmmott::USBVirtPP::Logger GetLoggerCallback(
  FredEmmott_USBIP_VirtPP_Instance const*);
