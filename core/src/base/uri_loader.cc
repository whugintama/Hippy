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

#include "core/base/uri_loader.h"

#include "base/logging.h"
#include "core/base/string_view_utils.h"

namespace hippy {
namespace base {

void UriLoader::RegisterUriDelegate(const unicode_string_view& scheme,
                                    std::shared_ptr<Delegate> delegate) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::u16string u16_scheme =
      StringViewUtils::Convert(scheme, unicode_string_view::Encoding::Utf16).utf16_value();

  auto it = router_.find(u16_scheme);
  if (it == router_.end()) {
    router_.insert({u16_scheme, std::list<std::shared_ptr<Delegate>>{delegate}});
  } else {
    it->second.push_back(delegate);
  }
}

void UriLoader::RegisterDebugDelegate(const unicode_string_view& scheme,
                                    std::shared_ptr<Delegate> delegate) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::u16string u16_scheme =
      StringViewUtils::Convert(scheme, unicode_string_view::Encoding::Utf16).utf16_value();

  auto it = router_.find(u16_scheme);
  if (it == router_.end()) {
    router_.insert({u16_scheme, std::list<std::shared_ptr<Delegate>>{delegate}});
  } else {
    it->second.push_front(delegate);
  }
}

UriLoader::RetCode UriLoader::RequestUntrustedContent(const unicode_string_view &uri,
                                                      bytes &content) {
  unicode_string_view scheme = GetScheme(uri);
  if (scheme.encoding() == unicode_string_view::Encoding::Unkown) {
    return RetCode::SchemeError;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  TDF_BASE_DCHECK(scheme.encoding() == unicode_string_view::Encoding::Utf16);
  const auto scheme_it = router_.find(scheme.utf16_value());
  if (scheme_it == router_.end()) {
    return RetCode::SchemeNotRegister;
  }
  SyncContext ctx{uri, RetCode::Success, content};
  auto cur_it = scheme_it->second.begin();
  auto end_it = scheme_it->second.end();
  std::function<std::shared_ptr<Delegate>()> next =
      [&cur_it, end_it]() -> std::shared_ptr<Delegate> {
        ++cur_it;
        if (cur_it != end_it) {
          return *cur_it;
        }
        return nullptr;
      };
  (*cur_it)->RequestUntrustedContent(ctx, next);
  return ctx.ret_code;
}

void UriLoader::RequestUntrustedContent(const unicode_string_view &uri,
                                        std::function<void(RetCode, bytes)> cb) {
  bytes content;
  unicode_string_view scheme = GetScheme(uri);
  if (scheme.encoding() == unicode_string_view::Encoding::Unkown) {
    cb(RetCode::SchemeError, content);
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  TDF_BASE_DCHECK(scheme.encoding() == unicode_string_view::Encoding::Utf16);
  const auto scheme_it = router_.find(scheme.utf16_value());
  if (scheme_it == router_.end()) {
    cb(RetCode::SchemeNotRegister, content);
    return;
  }
  ASyncContext ctx{uri, cb};
  auto cur_it = scheme_it->second.begin();
  auto end_it = scheme_it->second.end();
  std::function<std::shared_ptr<Delegate>()> next =
      [&cur_it, end_it]() -> std::shared_ptr<Delegate> {
        ++cur_it;
        if (cur_it != end_it) {
          return *cur_it;
        }
        return nullptr;
      };
  (*cur_it)->RequestUntrustedContent(ctx, next);
}

}
}
