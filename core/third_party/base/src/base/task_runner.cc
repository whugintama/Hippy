#include "base/task_runner.h"

#include <atomic>

#include "base/logging.h"
#include "base/worker.h"

namespace tdf {
namespace base {
std::atomic<int> global_task_runner_id{0};

TaskRunner::TaskRunner(bool is_excl, int priority)
    : is_terminated_(false),
      is_excl_(is_excl),
      priority_(priority),
      time_(TimeDelta::Zero()),
      cv_(nullptr) {
  id_ = global_task_runner_id.fetch_add(1);
}
TaskRunner::~TaskRunner() {}

void TaskRunner::Clear() {
  std::unique_lock<std::mutex> lock(mutex_);

  while (!task_queue_.empty()) {
    task_queue_.pop();
  }

  while (!delayed_task_queue_.empty()) {
    delayed_task_queue_.pop();
  }
}

void TaskRunner::AddSubTaskRunner(std::shared_ptr<TaskRunner> sub_runner, bool is_task_running) {
  std::shared_ptr<Worker> worker = worker_.lock();
  if (worker) {
    worker->BindGroup(id_, sub_runner);
    if (is_task_running) {
      worker->SetStackingMode(true);
      while (worker->GetStackingMode()) {
        worker->RunTask(); // todo task时间计算，被阻塞的task不应该计算到整体运行时间里面
      }
    }
  }
}

void TaskRunner::RemoveSubTaskRunner(std::shared_ptr<TaskRunner> sub_runner) {
  std::shared_ptr<Worker> worker = worker_.lock();
  if (worker) {
    worker->UnBind(sub_runner);
  }
  if (cv_) {  // 通知父Runner执行
    cv_->notify_one();
  }
}

void TaskRunner::Terminate() {
  is_terminated_ = true;

  Clear();
}

void TaskRunner::PostTask(std::unique_ptr<Task> task) {
  std::lock_guard<std::mutex> lock(mutex_);

  task_queue_.push(std::move(task));
  if (cv_) {  // cv未初始化时要等cv初始化后立刻处理
    cv_->notify_one();
  }
}

void TaskRunner::PostDelayedTask(std::unique_ptr<Task> task, TimeDelta delay) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (is_terminated_) {
    return;
  }

  TimePoint deadline = TimePoint::Now() + delay;
  delayed_task_queue_.push(std::make_pair(deadline, std::move(task)));

  cv_->notify_one();
}

std::unique_ptr<Task> TaskRunner::PopTask() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!task_queue_.empty()) {
    std::unique_ptr<Task> result = std::move(task_queue_.front());
    task_queue_.pop();
    return result;
  }

  return nullptr;
}

std::unique_ptr<Task> TaskRunner::GetTopDelayTask() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (task_queue_.empty() && !delayed_task_queue_.empty()) {
    std::unique_ptr<Task> result =
        std::move(const_cast<DelayedEntry&>(delayed_task_queue_.top()).second);
    return result;
  }
  return nullptr;
}

std::unique_ptr<Task> TaskRunner::GetNext() {
  std::unique_lock<std::mutex> lock(mutex_);

  if (is_terminated_) {
    return nullptr;
  }

  TimePoint now = TimePoint::Now();
  std::unique_ptr<Task> task = popTaskFromDelayedQueueNoLock(now);
  while (task) {
    task_queue_.push(std::move(task));
    task = popTaskFromDelayedQueueNoLock(now);
  }

  if (!task_queue_.empty()) {
    std::unique_ptr<Task> result = std::move(task_queue_.front());
    task_queue_.pop();
    return result;
  }
  return nullptr;
}

void TaskRunner::SetCv(std::shared_ptr<std::condition_variable> cv) { cv_ = cv; }

TimeDelta TaskRunner::GetNextTimeDelta(TimePoint now) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (task_queue_.empty() && !delayed_task_queue_.empty()) {
    const DelayedEntry& delayed_task = delayed_task_queue_.top();
    return delayed_task.first - now;
  } else {
    return TimeDelta::Max();
  }
}

std::unique_ptr<Task> TaskRunner::popTaskFromDelayedQueueNoLock(TimePoint now) {
  if (delayed_task_queue_.empty()) {
    return nullptr;
  }

  const DelayedEntry& deadline_and_task = delayed_task_queue_.top();
  if (deadline_and_task.first > now) {
    return nullptr;
  }

  std::unique_ptr<Task> result = std::move(const_cast<DelayedEntry&>(deadline_and_task).second);
  delayed_task_queue_.pop();
  return result;
}

}  // namespace base
}  // namespace tdf
