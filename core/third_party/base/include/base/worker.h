#pragma once

#include "task.h"
#include "thread.h"

#include <list>
#include <string>
#include <vector>

namespace tdf {
namespace base {
class WorkerPool;
class TaskRunner;
class Worker : public Thread {
 public:
  Worker(const std::string& name = "");
  ~Worker() = default;
  void Run();
  void Terminate();
  void BindGroup(int father_id, std::shared_ptr<TaskRunner> child);
  void Bind(std::vector<std::shared_ptr<TaskRunner>> runner);
  void Bind(std::list<std::vector<std::shared_ptr<TaskRunner>>> list);
  void UnBind(std::shared_ptr<TaskRunner> runner);
  std::list<std::vector<std::shared_ptr<TaskRunner>>> UnBind();
  std::list<std::vector<std::shared_ptr<TaskRunner>>> ReleasePending();
  std::list<std::vector<std::shared_ptr<TaskRunner>>> RetainActive();
  std::list<std::vector<std::shared_ptr<TaskRunner>>> Retain(std::shared_ptr<TaskRunner> runner);
  inline bool GetStackingMode() { return is_stacking_mode_; }
  inline void SetStackingMode(bool is_stacking_mode) { is_stacking_mode_ = is_stacking_mode; }

  static int32_t GetCurrentWorkerId();

 private:
  friend class WorkerPool;
  friend class TaskRunner;

  void Balance();
  void Sort();

  void RunTask();
  std::unique_ptr<Task> GetNextTask();

  std::vector<std::shared_ptr<TaskRunner>> curr_group_;
  std::list<std::vector<std::shared_ptr<TaskRunner>>> running_groups_;
  std::list<std::vector<std::shared_ptr<TaskRunner>>> pending_groups_;
  std::mutex running_mutex_;
  std::mutex balance_mutex_;
  std::shared_ptr<std::condition_variable> cv_;
  bool need_balance_;
  bool is_terminated_;
  bool is_stacking_mode_;

  // debug
  std::string name_;
};
}  // namespace base
}  // namespace tdf
