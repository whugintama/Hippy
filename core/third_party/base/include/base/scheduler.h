#ifndef TDF_BASE_SCHEDULER_H_
#define TDF_BASE_SCHEDULER_H_

#include <memory>

#include "include/task_runner.h"

namespace tdf {
namespace base {

class Scheduler {
 public:
  virtual void Resize(int size) = 0;
  virtual std::shared_ptr<TaskRunner> CreateTaskRunner(bool is_excl, int priority) = 0;
};
}  // namespace base
}  // namespace tdf

#endif  // TDF_BASE_SCHEDULER_H_