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

#include <android/asset_manager.h>

#include "core/core.h"
#include "jni/scoped_java_ref.h"

class DebugDelegate : public hippy::base::UriLoader::Delegate {
 public:
  using unicode_string_view = tdf::base::unicode_string_view;
  using UriLoader = hippy::base::UriLoader;

  DebugDelegate(std::shared_ptr<JavaRef> bridge);
  virtual ~DebugDelegate() = default;

  virtual void RequestUntrustedContent(
      UriLoader::SyncContext &ctx,
      std::function<std::shared_ptr<Delegate>()> next);
  virtual void RequestUntrustedContent(
      UriLoader::ASyncContext &ctx,
      std::function<std::shared_ptr<Delegate>()> next);

  static void SetSyncDelegate(std::function<void(UriLoader::SyncContext&,
      std::function<std::shared_ptr<Delegate>()>)> delegate) {
    on_request_untrusted_content_sync_ = delegate;
  }

  static void SetASyncDelegate(std::function<void(UriLoader::ASyncContext&,
      std::function<std::shared_ptr<Delegate>()>)> delegate) {
    on_request_untrusted_content_async_ = delegate;
  }

 private:
  static std::function<void(UriLoader::SyncContext & , std::function<std::shared_ptr<Delegate>()>)>
      on_request_untrusted_content_sync_;
  static std::function<void(UriLoader::ASyncContext & , std::function<std::shared_ptr<Delegate>()>)>
      on_request_untrusted_content_async_;

  void RegisterDebugDelegate();

  std::shared_ptr<JavaRef> bridge_;
};
