#pragma once

#include <mutex>

#include "scheduler.h"
#include "task_runner.h"
#include "thread.h"
#include "worker.h"

namespace tdf {
namespace base {
class WorkerPool : Scheduler {
 public:
  static std::shared_ptr<WorkerPool> GetInstance(int size);
  WorkerPool(int size);
  ~WorkerPool();
  void Terminate();
  void Resize(int size);
  std::shared_ptr<TaskRunner> CreateTaskRunner(bool is_excl = false, int priority = 1);
  void AddTaskRunner(std::shared_ptr<TaskRunner> runner);

 private:
  friend class Profile;

  // WorkerPool(int size);
  void CreateWorker(int size, bool is_excl = false);
  void BindWorker(std::shared_ptr<Worker> worker,
                              std::vector<std::shared_ptr<TaskRunner>> group);

  static std::mutex creation_mutex_;
  static std::shared_ptr<WorkerPool> instance_;

  std::vector<std::shared_ptr<Worker>> excl_workers_;
  std::vector<std::shared_ptr<Worker>> workers_;
  std::vector<std::shared_ptr<TaskRunner>> runners_;

  int index_;
  int size_;
  std::mutex mutex_;

  TDF_BASE_DISALLOW_COPY_AND_ASSIGN(WorkerPool);
};

}  // namespace base
}  // namespace tdf
