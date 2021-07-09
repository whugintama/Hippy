#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

namespace tdf {
namespace base {

class Task {
 public:
  Task();
  explicit Task(std::function<void()> exec_unit);
  ~Task() = default;

  inline uint32_t GetId() { return id_; }
  inline void SetExecUnit(std::function<void()> exec_unit) { exec_unit_ = exec_unit; }
  inline void Run() {
    if (exec_unit_) {
      exec_unit_();
    }
  };

 private:
  std::atomic<uint32_t> id_;
  std::function<void()> exec_unit_;  // A unit of work to be processed
};

}  // namespace base
}  // namespace tdf
