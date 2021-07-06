#pragma once

#include "base/worker.h"

#include <algorithm>
#include <atomic>

#include "base/logging.h"
#include "base/worker_pool.h"

namespace tdf {
namespace base {

std::atomic<int32_t> global_worker_id{0};
thread_local int32_t local_worker_id;

Worker::Worker(const std::string& name)
    : Thread(name), name_(name), is_terminated_(false), is_stacking_mode_(false) {
  cv_ = std::make_shared<std::condition_variable>();
}

int32_t Worker::GetCurrentWorkerId() { return local_worker_id; }

void Worker::Balance() {
  // running_mutex_ has locked before balance
  std::lock_guard<std::mutex> lock(balance_mutex_);

  if (pending_groups_.empty()) {
    return;
  }

  TimeDelta time;  // default 0
  if (!running_groups_.empty()) {
    // Sort is executed before balance
    time = running_groups_.front().at(0)->GetTime();
  }
  for (auto it = pending_groups_.begin(); it != pending_groups_.end(); ++it) {
    auto group = *it;
    for (auto group_it = it->begin(); group_it != it->end(); ++group_it) {
      (*group_it)->SetTime(time);
    }
  }
  running_groups_.splice(running_groups_.end(), pending_groups_);
  // The first taskrunner still has the highest priority
}

void Worker::RunTask() {
  auto task = GetNextTask();
  if (task && !task->is_canceled_) {
    TimePoint begin = TimePoint::Now();
    task->Run();
    for (auto it = curr_group_.begin(); it != curr_group_.end(); ++it) {
      (*it)->AddTime(TimePoint::Now() - begin);
    }
  }
}

void Worker::Run() {
  local_worker_id = global_worker_id.fetch_add(1);
  while (!is_terminated_) {
    RunTask();
  }
}

void Worker::Sort() {
  if (!running_groups_.empty()) {
    running_groups_.sort([](const auto& lhs, const auto& rhs) {
      TDF_BASE_DCHECK(!lhs.empty() && !rhs.empty());
      int64_t left = lhs[0]->GetPriority() * lhs[0]->GetTime().ToNanoseconds();
      int64_t right = rhs[0]->GetPriority() * rhs[0]->GetTime().ToNanoseconds();
      if (left < right) {
        return true;
      }
      return false;
    });
  }
}

void Worker::Terminate() {
  is_terminated_ = true;
  cv_->notify_one();
  Join();
}

void Worker::BindGroup(int father_id, std::shared_ptr<TaskRunner> child) {
  std::lock_guard<std::mutex> lock(running_mutex_);
  std::list<std::vector<std::shared_ptr<TaskRunner>>>::iterator group_it;
  bool has_found = false;
  for (group_it = running_groups_.begin(); group_it != running_groups_.end(); ++group_it) {
    for (auto runner_it = group_it->begin(); runner_it != group_it->end(); ++runner_it) {
      if ((*runner_it)->GetId() == father_id) {
        has_found = true;
        break;
      }
    }
    if (has_found) {
      break;
    }
  }
  if (!has_found) {
    std::lock_guard<std::mutex> lock(balance_mutex_);
    for (group_it = pending_groups_.begin(); group_it != pending_groups_.end(); ++group_it) {
      for (auto runner_it = group_it->begin(); runner_it != group_it->end(); ++runner_it) {
        if ((*runner_it)->GetId() == father_id) {
          has_found = true;
          break;
        }
      }
      if (has_found) {
        break;
      }
    }
  }
  group_it->push_back(child);
}

void Worker::Bind(std::vector<std::shared_ptr<TaskRunner>> runner) {
  std::lock_guard<std::mutex> lock(balance_mutex_);

  std::vector<std::shared_ptr<TaskRunner>> group{runner};
  pending_groups_.insert(pending_groups_.end(), group);
  need_balance_ = true;
  cv_->notify_one();
}

void Worker::Bind(std::list<std::vector<std::shared_ptr<TaskRunner>>> list) {
  std::lock_guard<std::mutex> lock(balance_mutex_);
  pending_groups_.splice(pending_groups_.end(), list);
  need_balance_ = true;
  cv_->notify_one();
}

void Worker::UnBind(std::shared_ptr<TaskRunner> runner) {
  std::lock_guard<std::mutex> lock(running_mutex_);

  std::vector<std::shared_ptr<TaskRunner>> group;
  bool has_found = false;
  for (auto group_it = running_groups_.begin(); group_it != running_groups_.end(); ++group_it) {
    for (auto runner_it = group_it->begin(); runner_it != group_it->end(); ++runner_it) {
      if ((*runner_it)->GetId() == runner->GetId()) {
        group_it->erase(runner_it);
        has_found = true;
        break;
      }
    }
    if (has_found) {
      break;
    }
  }
  if (!has_found) {
    std::lock_guard<std::mutex> lock(balance_mutex_);
    for (auto group_it = pending_groups_.begin(); group_it != pending_groups_.end(); ++group_it) {
      for (auto runner_it = group_it->begin(); runner_it != group_it->end(); ++runner_it) {
        if ((*runner_it)->GetId() == runner->GetId()) {
          group_it->erase(runner_it);
          has_found = true;
          break;
        }
      }
      if (has_found) {
        break;
      }
    }
  }
}

std::list<std::vector<std::shared_ptr<TaskRunner>>> Worker::UnBind() {
  std::list<std::vector<std::shared_ptr<TaskRunner>>> ret;
  {
    std::lock_guard<std::mutex> lock(running_mutex_);
    ret.splice(ret.end(), running_groups_);
  }
  {
    std::lock_guard<std::mutex> lock(balance_mutex_);
    ret.splice(ret.end(), pending_groups_);
  }

  return ret;
}

std::list<std::vector<std::shared_ptr<TaskRunner>>> Worker::ReleasePending() {
  std::lock_guard<std::mutex> lock(balance_mutex_);

  std::list<std::vector<std::shared_ptr<TaskRunner>>> ret(std::move(pending_groups_));
  pending_groups_ = std::list<std::vector<std::shared_ptr<TaskRunner>>>{};
  return ret;
}

std::list<std::vector<std::shared_ptr<TaskRunner>>> Worker::RetainActive() {
  std::lock_guard<std::mutex> lock(running_mutex_);
  std::list<std::vector<std::shared_ptr<TaskRunner>>> ret;
  auto group_it = running_groups_.begin();
  if (group_it != running_groups_.end()) {
    ++group_it;
    ret.splice(ret.end(), running_groups_, group_it, running_groups_.end());
  }

  ret.splice(ret.end(), pending_groups_);
  return ret;
}

std::list<std::vector<std::shared_ptr<TaskRunner>>> Worker::Retain(
    std::shared_ptr<TaskRunner> runner) {
  std::lock_guard<std::mutex> lock(running_mutex_);

  std::vector<std::shared_ptr<TaskRunner>> group;
  for (auto group_it = running_groups_.begin(); group_it != running_groups_.end(); ++group_it) {
    for (auto runner_it = group_it->begin(); runner_it != group_it->end(); ++runner_it) {
      if ((*runner_it)->GetId() == runner->GetId()) {
        group = *group_it;
        running_groups_.erase(group_it);
        break;
      }
    }
  }

  std::list<std::vector<std::shared_ptr<TaskRunner>>> ret(std::move(running_groups_));
  running_groups_ = std::list<std::vector<std::shared_ptr<TaskRunner>>>{group};
  return ret;
}

std::unique_ptr<Task> Worker::GetNextTask() {
  if (is_terminated_) {
    return nullptr;
  }

  std::unique_lock<std::mutex> lock(running_mutex_);

  Sort();
  if (need_balance_) {
    Balance();
    need_balance_ = false;
  }

  TimeDelta min_time, time;
  TimePoint now;
  for (auto it = running_groups_.begin(); it != running_groups_.end(); ++it) {
    std::shared_ptr<TaskRunner> runner = it->back();
    auto task = runner->GetNext();
    if (task) {
      curr_group_ = *it;
      return task;
    } else {
      if (now == TimePoint()) {  // uninitialized
        now = TimePoint::Now();
        min_time = TimeDelta::Max();
        time = min_time;
      }
      time = it->front()->GetNextTimeDelta(now);
      if (min_time > time) {
        min_time = time;
      }
    }
  }

  if (min_time != TimeDelta::Max() && min_time != TimeDelta::Zero()) {
    cv_->wait_for(lock, std::chrono::nanoseconds(min_time.ToNanoseconds()));
  } else {
    cv_->wait(lock);
  }
  return nullptr;
}
}  // namespace base
}  // namespace tdf
