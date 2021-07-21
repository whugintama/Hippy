#pragma once

#include "base/base_timer.h"

namespace tdf {
namespace base {

class RepeatingTimer : public BaseTimer {
 public:
  RepeatingTimer() = default;
  RepeatingTimer(std::shared_ptr<TaskRunner> task_runner);
  virtual ~RepeatingTimer();

  virtual void Start(std::unique_ptr<Task> user_task, TimeDelta delay);

 private:
  void OnStop() final;
  void RunUserTask() override;

  TDF_BASE_DISALLOW_COPY_AND_ASSIGN(RepeatingTimer);
};

}
}
