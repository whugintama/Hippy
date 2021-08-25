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

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <mutex>

#include "base/unicode_string_view.h"


namespace hippy {
namespace base {

class UriLoader {
 public:
  enum class SourceType { Core, Java, OC};
  enum class RetCode { Success, Failed, DelegateError, UriError, SchemeError, SchemeNotRegister,
    PathNotMatch, PathError, ResourceNotFound, Timeout };
  using unicode_string_view = tdf::base::unicode_string_view;
  using bytes = std::string;

  struct SyncContext {
    const unicode_string_view& uri;
    RetCode ret_code;
    bytes& content;
  };

  struct ASyncContext {
    const unicode_string_view& uri;
    std::function<void(RetCode, bytes)> cb;
  };

  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;
    virtual void RequestUntrustedContent(
        SyncContext& ctx,
        std::function<std::shared_ptr<Delegate>()> next) = 0;
    virtual void RequestUntrustedContent(
        ASyncContext& ctx,
        std::function<std::shared_ptr<Delegate>()> next) = 0;
  };

  UriLoader() {}
  ~UriLoader() {}

  virtual void RegisterUriDelegate(const unicode_string_view &scheme,
                                   std::shared_ptr<Delegate> delegate);
  virtual void RegisterDebugDelegate(const unicode_string_view &scheme,
                                     std::shared_ptr<Delegate> delegate);

  virtual void RequestUntrustedContent(
      const unicode_string_view& uri,
      std::function<void(RetCode, bytes)> cb);

  virtual RetCode RequestUntrustedContent(
      const unicode_string_view& uri,
      bytes& content);

  virtual unicode_string_view GetScheme(const unicode_string_view& uri) = 0;
 private:
  std::map<std::u16string, std::list<std::shared_ptr<Delegate>>> router_;
  std::mutex mutex_;
};

}  // namespace base
}  // namespace hippy
