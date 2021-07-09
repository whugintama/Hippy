#pragma once

#include "base/repeating_timer.h"

#include "base/logging.h"

namespace tdf::base {

RepeatingTimer::~RepeatingTimer() = default;

RepeatingTimer::RepeatingTimer(std::shared_ptr<TaskRunner> task_runner) : BaseTimer(task_runner) {}

void RepeatingTimer::Start(std::unique_ptr<Task> user_task, TimeDelta delay) {
  user_task_ = std::move(user_task);
  StartInternal(delay);
}

void RepeatingTimer::OnStop() {}
void RepeatingTimer::RunUserTask() {
  std::unique_ptr<Task>& task = user_task_;
  ScheduleNewTask(delay_);
  task->Run();
}

}  // namespace tdf::base
