#pragma once

#include <functional>
#include <memory>

#include "base/task_runner.h"
#include "base/time_delta.h"
#include "base/time_point.h"

namespace tdf {
namespace base {

class BaseTimer {
 public:
  BaseTimer() = default;
  BaseTimer(std::shared_ptr<TaskRunner> task_runner);
  virtual ~BaseTimer();

  void Stop();
  void Reset();
  inline void BindTaskRunner(std::shared_ptr<TaskRunner> task_runner) {
    task_runner_ = task_runner;
  }
  inline bool IsRunning() { return is_running_; }

 protected:
  virtual void RunUserTask() = 0;
  virtual void OnStop() = 0;
  void ScheduleNewTask(TimeDelta delay);
  void StartInternal(TimeDelta delay);

  std::weak_ptr<TaskRunner> task_runner_;
  std::unique_ptr<Task> user_task_;
  TimeDelta delay_;

 private:
  void OnScheduledTaskInvoked();

  bool is_running_;
  TimePoint desired_run_time_;
  TimePoint scheduled_run_time_;

  TDF_BASE_DISALLOW_COPY_AND_ASSIGN(BaseTimer);
};

}  // namespace base
}  // namespace tdf
