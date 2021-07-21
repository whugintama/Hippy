#pragma once

#include <memory>

#include "task_runner.h"

namespace tdf {
namespace base {

class Scheduler {
 public:
  virtual void Resize(int size) = 0;
  virtual std::shared_ptr<TaskRunner> CreateTaskRunner(bool is_excl, int priority) = 0;
};
}  // namespace base
}  // namespace tdf
