// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include "logging.hpp"

#include <Windows.h>

#include <FredEmmott/USBIP-VirtPP/Core.h>

#include <algorithm>
#include <functional>
#include <source_location>

namespace FredEmmott::USBVirtPP {

namespace detail {

template <class TOut, class TIn>
TOut MapResult(TIn) = delete;

template <class T>
T MapResult(std::type_identity_t<T> v) {
  return v;
}

template <class TOut, class TIn>
  requires std::same_as<TOut, void>
void MapResult(TIn) {
}

}// namespace detail

template<logging_target TLogger, class TFn>
struct CInvoke {
  CInvoke() = delete;

  CInvoke(
    TLogger logger,
    TFn fn,
    const std::source_location caller = std::source_location::current()
    ): mLogger(logger), mFn(fn), mCaller(caller) {}

  template<class... TArgs, class TRet = std::invoke_result_t<TFn, TArgs...>>
  TRet operator()(TArgs&&... args) const {
    using detail::MapResult;
    try {
      return std::invoke(mFn, std::forward<TArgs>(args)...);
    } catch (const std::system_error& e) {
      LogError(
        mLogger,
        "[{}@{}:{}] System error: {} ({})",
        mCaller.function_name(),
        mCaller.file_name(),
        mCaller.line(),
        e.what(),
        e.code().value());
      return MapResult<TRet>(e.code().value());
    } catch (const std::exception& e) {
      LogError(
        mLogger,
        "[{}@{}:{}] C++ exception: {}",
        mCaller.function_name(),
        mCaller.file_name(),
        mCaller.line(),
        e.what());
      return MapResult<TRet>(HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION));
    } catch (...) {
      LogError(
        mLogger,
        "[{}@{}:{}] non-standard C++ exception",
        mCaller.function_name(),
        mCaller.file_name(),
        mCaller.line());
      __debugbreak();
      return MapResult<TRet>(HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION));
    }
  }
private:
  const TLogger mLogger;
  TFn mFn;
  const std::source_location mCaller;
};

}// namespace FredEmmott::USBVirtPP