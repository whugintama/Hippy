#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <string>
#include <thread>

#include "macros.h"

namespace tdf {
namespace base {

class Thread {
 public:
  explicit Thread(const std::string& name = "");

  ~Thread();

  void Join();

  static void SetCurrentThreadName(const std::string& name);

  virtual void Run() = 0;

 private:
  std::unique_ptr<std::thread> thread_;

  TDF_BASE_DISALLOW_COPY_AND_ASSIGN(Thread);
};

}  // namespace base
}  // namespace tdf
