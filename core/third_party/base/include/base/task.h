#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

namespace tdf {
namespace base {

class Task {
 public:
  Task();
  ~Task() = default;

  inline void Run() {
    if (cb_) {
      cb_();
    }
  };

  bool is_canceled_;
  std::function<void()> cb_;
  std::atomic<uint32_t> id_;
};

}  // namespace base
}  // namespace tdf
