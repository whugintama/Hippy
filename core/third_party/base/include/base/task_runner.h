#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "macros.h"
#include "task.h"
#include "worker.h"
#include "time_delta.h"
#include "time_point.h"

namespace tdf {
namespace base {
class TaskRunner {
 public:
  TaskRunner(bool is_excl = false, int priority = 1);
  ~TaskRunner();

  void Clear();
  void Terminate();
  void AddSubTaskRunner(std::shared_ptr<TaskRunner> sub_runner, bool is_task_running = false);
  void RemoveSubTaskRunner(std::shared_ptr<TaskRunner> sub_runner);
  void PostTask(std::unique_ptr<Task> task);
  void PostDelayedTask(std::unique_ptr<Task> task, TimeDelta delay);
  void CancelTask(std::unique_ptr<Task> task);
  TimeDelta GetNextTimeDelta(TimePoint now);

  inline bool GetExclusive() { return is_excl_; }
  inline int GetPriority() { return priority_; }
  inline int GetId() { return id_; }
  inline TimeDelta GetTime() { return time_; }
  inline TimeDelta AddTime(TimeDelta time) {
    time_ = time_ + time;
    return time_;
  }

  inline void SetTime(TimeDelta time) { time_ = time; }

 private:
  friend class Worker;
  friend class Scheduler;
  friend class WorkerPool;

  // TaskRunner(bool is_excl = false);

  std::unique_ptr<Task> popTaskFromDelayedQueueNoLock(TimePoint now);

  std::unique_ptr<Task> PopTask();
  std::unique_ptr<Task> GetTopDelayTask();
  std::unique_ptr<Task> GetNext();

  void SetCv(std::shared_ptr<std::condition_variable> cv);

  std::queue<std::unique_ptr<Task>> task_queue_;
  using DelayedEntry = std::pair<TimePoint, std::unique_ptr<Task>>;
  struct DelayedEntryCompare {
    bool operator()(const DelayedEntry& left, const DelayedEntry& right) const {
      return left.first > right.first;
    }
  };
  std::priority_queue<DelayedEntry, std::vector<DelayedEntry>, DelayedEntryCompare>
      delayed_task_queue_;
  std::mutex mutex_;
  std::shared_ptr<std::condition_variable> cv_;
  std::weak_ptr<Worker> worker_;
  bool is_terminated_;
  bool is_excl_;
  int priority_;
  int id_;
  TimeDelta time_;

  TDF_BASE_DISALLOW_COPY_AND_ASSIGN(TaskRunner);
};

}  // namespace base
}  // namespace tdf
