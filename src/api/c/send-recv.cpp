// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#include "send-recv.hpp"

#include <print>

namespace FredEmmott::USBVirtPP {

std::expected<void, HRESULT> SendAll(SOCKET sock, const void* buffer, const size_t len) {
  const char* ptr = (const char*)buffer;
  int sent = 0;
  while (sent < len) {
    const int result = send(sock, ptr + sent, (int)(len - sent), 0);
    if (result == SOCKET_ERROR) {
      const auto err = WSAGetLastError();
      std::println(stderr, "Send failed: {}", err);
      return std::unexpected { HRESULT_FROM_WIN32(err) };
    }
    sent += result;
  }
  return {};
}

std::expected<void, HRESULT> RecvAll(SOCKET sock, void* buffer, size_t len) {
  char* ptr = (char*)buffer;
  int received = 0;
  while (received < len) {
    int result = recv(sock, ptr + received, (int)(len - received), 0);
    if (result == 0) {
      std::println(stderr, "Connection closed by peer.");
      return std::unexpected { HRESULT_FROM_WIN32( WSAECONNRESET) };
    }
    if (result == SOCKET_ERROR) {
      const auto err = WSAGetLastError();
      std::println(stderr, "Recv failed: {}", err);
      return std::unexpected { HRESULT_FROM_WIN32(err) };
    }
    received += result;
  }
  return {};
}

}