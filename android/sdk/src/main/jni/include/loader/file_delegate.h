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

#pragma once

#include "core/core.h"

class FileDelegate : public hippy::base::UriLoader::Delegate {
 public:
  using unicode_string_view = tdf::base::unicode_string_view;
  using UriLoader = hippy::base::UriLoader;

  FileDelegate() = default;
  virtual ~FileDelegate() = default;

  inline void SetWorkerTaskRunner(std::weak_ptr<WorkerTaskRunner> runner) {
    runner_ = runner;
  }

  virtual void RequestUntrustedContent(
      UriLoader::SyncContext& ctx,
      std::function<void(UriLoader::SyncContext&)> next);
  virtual void RequestUntrustedContent(
      UriLoader::ASyncContext& ctx,
      std::function<void(UriLoader::ASyncContext&)> next);
 private:
  bool LoadByFile(const unicode_string_view& path,
                  std::function<void(UriLoader::RetCode, UriLoader::bytes)> cb);

  std::weak_ptr<WorkerTaskRunner> runner_;
};
