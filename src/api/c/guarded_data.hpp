// Copyright 2025 Fred Emmott <fred@fredemmott.com>
// SPDX-License-Identifier: MIT
#pragma once
#include <mutex>

/* Totally not a Rust mutex.
 *
 * Hides the data so it can only be accessed via a lock guard.
 */
template <class T>
struct guarded_data {
  struct unique_lock {
    unique_lock(std::unique_lock<std::mutex> lock, T* data)
      : mLock(std::move(lock)), mData(data) {
    }

    T* operator->() const {
      return mData;
    }

  private:
    unique_lock() = delete;

    std::unique_lock<std::mutex> mLock;
    T* mData;
  };

  unique_lock lock() {
    return guarded_data::unique_lock(std::unique_lock {mMutex}, &mData);
  }

private:
  std::mutex mMutex {};
  T mData {};
};
