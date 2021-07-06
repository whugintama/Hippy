#include "base/worker_pool.h"

#include "base/logging.h"

namespace tdf {
namespace base {

std::mutex WorkerPool::creation_mutex_;

std::shared_ptr<WorkerPool> WorkerPool::instance_ = nullptr;

WorkerPool::WorkerPool(int size) : size_(size), index_(0) { CreateWorker(size); }

WorkerPool::~WorkerPool() {}

void WorkerPool::Terminate() {
  for (auto it = workers_.begin(); it != workers_.end(); ++it) {
    (*it)->Terminate();
  }
};

std::shared_ptr<WorkerPool> WorkerPool::GetInstance(int size) {
  std::scoped_lock creation(creation_mutex_);
  if (!instance_) {
    instance_ = std::make_shared<WorkerPool>(size);
  }
  return instance_;
}

void WorkerPool::CreateWorker(int size, bool is_excl) {
  for (int i = 0; i < size; ++i) {
    if (is_excl) {
      excl_workers_.push_back(std::move(std::make_shared<Worker>("")));
    } else {
      workers_.push_back(std::move(std::make_shared<Worker>("")));
    }
  }
}

void WorkerPool::Resize(int size) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (size == size_) {
    return;
  } else if (size > size_) {  // increase the number of threads
    CreateWorker(size - size_);
    std::list<std::vector<std::shared_ptr<TaskRunner>>>
        groups;  // save all runners that need to be reallocated
    for (int i = 0; i < size_; ++i) {
      groups.splice(groups.end(), workers_[i]->RetainActive());
    }

    // 暂不排序，这里如果根据加权运行时间排序，后面worker还需要runner排序
    // runners.sort();

    index_ = size_;
    auto it = groups.begin();
    while (it != groups.end()) {
      // use rr
      workers_[index_]->Bind(*it);
      index_ == size - 1 ? 0 : ++index_;
      ++it;
    }
    size_ = size;
  } else {
    // reduce the number of threads
    // size < size_
    if (index_ > size - 1) {
      index_ = 0;  // make sure index_ is valid
    }

    std::list<std::vector<std::shared_ptr<TaskRunner>>> groups;
    for (int i = size_ - 1; i > size - 1; --i) {
      // handle running runner on thread
      groups.splice(groups.end(), workers_[i]->UnBind());
    }
    for (int i = 0; i < size; ++i) {
      groups.splice(groups.end(), workers_[i]->RetainActive());
    }

    if (index_ >= size) {
      index_ = 0;
    }

    auto it = groups.begin();
    while (it != groups.end()) {
      // use rr
      workers_[index_]->Bind(*it);
      index_ == size - 1 ? 0 : ++index_;
      ++it;
    }
    size_ = size;

    for (int i = size_ - 1; i > size - 1; --i) {
      // handle running runner on thread
      workers_[i]->Terminate();
      workers_.pop_back();
    }
  }
}

std::shared_ptr<TaskRunner> WorkerPool::CreateTaskRunner(bool is_excl, int priority) {
  auto ret = std::make_shared<TaskRunner>(is_excl, priority);
  instance_->AddTaskRunner(ret);
  return ret;
}

void WorkerPool::AddTaskRunner(std::shared_ptr<TaskRunner> runner) {
  std::vector<std::shared_ptr<TaskRunner>> group{runner};
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (runner->GetExclusive()) {
      CreateWorker(1, true);
      auto it = excl_workers_.rbegin();
      (*it)->Bind(group);
      BindWorker(*it, group);
      runner->SetCv((*it)->cv_);
    } else {
      workers_[index_]->Bind(group);
      BindWorker(workers_[index_], group);
      runner->SetCv(workers_[index_]->cv_);
      index_ == (size_ - 1) ? 0 : ++index_;
    }
  }
}

void WorkerPool::BindWorker(std::shared_ptr<Worker> worker,
                            std::vector<std::shared_ptr<TaskRunner>> group) {
  for (auto it = group.begin(); it != group.end(); ++it) {
    (*it)->worker_ = worker;
  }
}
}  // namespace base
}  // namespace tdf
