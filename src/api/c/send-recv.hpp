// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once

#include <expected>

#include <winsock2.h>

namespace FredEmmott::USBVirtPP {

std::expected<void, HRESULT> SendAll(SOCKET sock, void const* buffer, std::size_t len);
std::expected<void, HRESULT> RecvAll(SOCKET sock, void* buffer, size_t len);

template <class T>
auto SendAll(const SOCKET sock, const T& what) {
  return SendAll(sock, &what, sizeof(T));
}

template <class T>
auto RecvAll(const SOCKET sock, T* what) {
  return RecvAll(sock, what, sizeof(std::decay_t<T>));
}


}