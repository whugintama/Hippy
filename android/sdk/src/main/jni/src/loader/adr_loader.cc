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

#include "loader/adr_loader.h"

#include "core/core.h"
#include "jni/uri.h"

using unicode_string_view = tdf::base::unicode_string_view;

ADRLoader::ADRLoader() {}

unicode_string_view ADRLoader::GetScheme(const unicode_string_view& uri) {
  std::shared_ptr<Uri> uri_obj = Uri::Create(uri);
  if (!uri_obj) {
    TDF_BASE_DLOG(ERROR) << "uri error, uri = " << uri;
    return unicode_string_view();
  }
  return uri_obj->GetScheme();
}

