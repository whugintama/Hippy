#pragma once

#include "base/one_shot_timer.h"

#include "base/logging.h"

namespace tdf::base {

OneShotTimer::OneShotTimer(std::shared_ptr<TaskRunner> task_runner) : BaseTimer(task_runner) {}

OneShotTimer::~OneShotTimer() = default;

void OneShotTimer::Start(std::unique_ptr<Task> user_task, TimeDelta delay) {
  user_task_ = std::move(user_task);
  StartInternal(delay);
}

void OneShotTimer::FireNow() {
  TDF_BASE_DCHECK(IsRunning());

  RunUserTask();
}

void OneShotTimer::OnStop() { user_task_.reset(); }

void OneShotTimer::RunUserTask() {
  std::unique_ptr<Task> task = std::move(user_task_);
  Stop();
  TDF_BASE_DCHECK(task);
  std::move(task)->Run();
}
}  // namespace tdf::base
