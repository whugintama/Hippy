/*
 *
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "core/modules/timer_module.h"

#include <memory>

#include "base/logging.h"
#include "base/task.h"
#include "base/task_runner.h"
#include "core/base/common.h"
#include "core/base/string_view_utils.h"
#include "core/modules/module_register.h"
#include "core/napi/callback_info.h"

REGISTER_MODULE(TimerModule, SetTimeout)
REGISTER_MODULE(TimerModule, ClearTimeout)
REGISTER_MODULE(TimerModule, SetInterval)
REGISTER_MODULE(TimerModule, ClearInterval)

namespace napi = ::hippy::napi;

using unicode_string_view = tdf::base::unicode_string_view;
using Task = tdf::base::Task;
using TaskRunner = tdf::base::TaskRunner;
using TimeDelta = tdf::base::TimeDelta;
using Ctx = hippy::napi::Ctx;
using CtxValue = hippy::napi::CtxValue;
using RegisterFunction = hippy::base::RegisterFunction;
using RegisterMap = hippy::base::RegisterMap;

TimerModule::TimerModule() {}

TimerModule::~TimerModule() {}

void TimerModule::SetTimeout(const napi::CallbackInfo& info) {
  info.GetReturnValue()->Set(Start(info, false));
}

void TimerModule::ClearTimeout(const napi::CallbackInfo& info) {
  ClearInterval(info);
}

void TimerModule::SetInterval(const napi::CallbackInfo& info) {
  info.GetReturnValue()->Set(Start(info, true));
}

void TimerModule::ClearInterval(const napi::CallbackInfo& info) {
  std::shared_ptr<Scope> scope = info.GetScope();
  std::shared_ptr<Ctx> context = scope->GetContext();
  TDF_BASE_CHECK(context);

  int32_t argument1 = 0;
  if (!context->GetValueNumber(info[0], &argument1)) {
    info.GetExceptionValue()->Set(context, "The first argument must be int32.");
    return;
  }

  uint32_t task_id = argument1;
  Cancel(task_id, scope);
  info.GetReturnValue()->Set(context->CreateNumber(task_id));
}

std::shared_ptr<hippy::napi::CtxValue> TimerModule::Start(
    const napi::CallbackInfo& info,
    bool repeat) {
  std::shared_ptr<Scope> scope = info.GetScope();
  std::shared_ptr<Ctx> context = scope->GetContext();
  TDF_BASE_CHECK(context);

  std::shared_ptr<CtxValue> function = info[0];
  if (!context->IsFunction(function)) {
    info.GetExceptionValue()->Set(context,
                                  "The first argument must be function.");
    return nullptr;
  }

  double number = 0;
  context->GetValueNumber(info[1], &number);
  TimeDelta delta = TimeDelta::FromSecondsF(std::max(.0, number));
  int64_t interval = delta.ToMilliseconds();

  std::unique_ptr<Task> task = std::make_unique<Task>();
  std::weak_ptr<Scope> weak_scope = scope;
  std::weak_ptr<CtxValue> weak_function = function;
  std::unique_ptr<Task>& quotes_task = task;
  uint32_t task_id = task->id_;

  //to do cb复制问题
  std::function<void()> cb = [this, weak_scope, weak_function, cb, quotes_task,
                              repeat, delta] {
    std::shared_ptr<Scope> scope = weak_scope.lock();
    if (!scope) {
      return;
    }
    std::shared_ptr<CtxValue> function = weak_function.lock();
    if (function) {
      std::shared_ptr<hippy::napi::Ctx> context = scope->GetContext();
      context->CallFunction(function);
    }

    std::unique_ptr<RegisterMap>& map = scope->GetRegisterMap();
    if (map) {
      RegisterMap::const_iterator it = map->find(hippy::base::kAsyncTaskEndKey);
      if (it != map->end()) {
        RegisterFunction f = it->second;
        if (f) {
          f(nullptr);
        }
      }
    }

    if (repeat) {
      std::shared_ptr<TaskRunner> runner = scope->GetTaskRunner();
      std::unique_ptr<Task> repeat_task = std::make_unique<Task>();
      repeat_task->cb_ = cb;
      if (runner) {
        runner->PostDelayedTask(std::move(repeat_task), delta);
      }
    } else {
      RemoveTask(task_id);
    }
  };

  std::shared_ptr<TaskRunner> runner = scope->GetTaskRunner();
  if (runner) {
    runner->PostDelayedTask(std::move(task), delta);
  }

  task_map_.insert({task_id, std::make_shared<TaskEntry>(context, function)});
  return context->CreateNumber(task_id);
}

void TimerModule::RemoveTask(uint32_t task_id) {
  task_map_.erase(task_id);
}

void TimerModule::Cancel(uint32_t task_id, std::shared_ptr<Scope> scope) {
  auto item = task_map_.find(task_id);
  if (item != task_map_.end()) {
    std::shared_ptr<TaskRunner> runner = scope->GetTaskRunner();
    if (runner) {
      runner->CancelTask(task_id);
    }
    task_map_.erase(item->first);
  }
}
