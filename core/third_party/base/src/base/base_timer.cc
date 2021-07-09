#include "base/base_timer.h"

namespace tdf {
namespace base {

BaseTimer::BaseTimer(std::shared_ptr<TaskRunner> task_runner)
    : user_task_(nullptr), task_runner_(task_runner){};

BaseTimer::~BaseTimer(){};

void BaseTimer::Stop() {
  is_running_ = false;
  OnStop();
}

void BaseTimer::ScheduleNewTask(TimeDelta delay) {
  std::shared_ptr<TaskRunner> task_runner = task_runner_.lock();
  if (!task_runner) {
    return;
  }

  is_running_ = true;
  if (delay > TimeDelta::Zero()) {
    task_runner->PostDelayedTask(std::make_unique<Task>(user_task_), delay);
    scheduled_run_time_ = desired_run_time_ = TimePoint::Now() + delay;
  } else {
    task_runner->PostTask(std::make_unique<Task>(user_task_));
    scheduled_run_time_ = desired_run_time_ = TimePoint::Now();
  }
}

void BaseTimer::OnScheduledTaskInvoked() {
  if (!is_running_) {
    return;
  }

  if (desired_run_time_ > scheduled_run_time_) {
    TimePoint now = TimePoint::Now();
    if (desired_run_time_ > now) {
      ScheduleNewTask(desired_run_time_ - now);
      return;
    }
  }

  RunUserTask();
}

void BaseTimer::StartInternal(TimeDelta delay) {
  delay_ = delay;

  Reset();
}

void BaseTimer::Reset() {
  if (delay_ > TimeDelta::Zero()) {
    desired_run_time_ = TimePoint::Now() + delay_;
  } else {
    desired_run_time_ = TimePoint::Now();
  }

  if (desired_run_time_ >= scheduled_run_time_) {
    is_running_ = true;
    return;
  }

  ScheduleNewTask(delay_);
}

}  // namespace base
}  // namespace tdf
