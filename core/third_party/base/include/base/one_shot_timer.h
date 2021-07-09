#pragma once

#include "base/base_timer.h"

namespace tdf::base {

class OneShotTimer : public BaseTimer {
 public:
  OneShotTimer() = default;
  OneShotTimer(std::shared_ptr<TaskRunner> task_runner);
  virtual ~OneShotTimer();

  virtual void Start(std::unique_ptr<Task> user_task, TimeDelta delay = TimeDelta::Zero());

  void FireNow();

 private:
  void OnStop() final;
  void RunUserTask() final;

  std::unique_ptr<Task> user_task_;

  TDF_BASE_DISALLOW_COPY_AND_ASSIGN(OneShotTimer);
};

}  // namespace tdf::base
