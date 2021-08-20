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

#include <future>

#include "bridge/runtime.h"
#include "core/base/string_view_utils.h"
#include "core/core.h"
#include "jni/jni_env.h"
#include "jni/jni_register.h"
#include "jni/jni_utils.h"
#include "jni/uri.h"

using unicode_string_view = tdf::base::unicode_string_view;
using StringViewUtils = hippy::base::StringViewUtils;
using UriLoader = hippy::base::UriLoader;
using u8string = unicode_string_view::u8string;
using char8_t_ = unicode_string_view::char8_t_;

static std::atomic<int64_t> global_request_id{0};

ADRLoader::ADRLoader() {}

unicode_string_view ADRLoader::GetScheme(const unicode_string_view& uri) {
  std::shared_ptr<Uri> uri_obj = Uri::Create(uri);
  if (!uri_obj) {
    TDF_BASE_DLOG(ERROR) << "uri error, uri = " << uri;
    return unicode_string_view();
  }
  return uri_obj->GetScheme();
}

void ADRLoader::GetContent(const unicode_string_view& uri,
                        std::function<void(RetCode, bytes)> cb) {
  LoadByJNI(uri, cb);
}

UriLoader::RetCode ADRLoader::GetContent(
    const unicode_string_view& uri,
    bytes& content) {
  std::shared_ptr<JNIEnvironment> instance = JNIEnvironment::GetInstance();
  JNIEnv* j_env = instance->AttachCurrentThread();

  if (instance->GetMethods().j_fetch_resource_sync_method_id) {
    jstring j_relative_path = JniUtils::StrViewToJString(j_env, uri);
    jobject ret = j_env->CallObjectMethod(bridge_->GetObj(),
                          instance->GetMethods().j_fetch_resource_sync_method_id,
                          j_relative_path);
    j_env->DeleteLocalRef(j_relative_path);
    jclass j_cls = j_env->GetObjectClass(ret);
    jfieldID ret_code_field = j_env->GetFieldID(j_cls, "retCode", "I");
    jint j_ret_code = j_env->GetIntField(ret, ret_code_field);
    jfieldID bytes_field = j_env->GetFieldID(j_cls, "content", "[B");
    jbyteArray j_bytes = (jbyteArray)j_env->GetObjectField(j_cls, bytes_field);
    content = JniUtils::AppendJavaByteArrayToBytes(j_env, j_bytes);
    return static_cast<RetCode>(j_ret_code);
  }
  return RetCode::Failed;
}

bool ADRLoader::LoadByJNI(const unicode_string_view& uri,
                          std::function<void(RetCode, bytes)> cb) {
  std::shared_ptr<JNIEnvironment> instance = JNIEnvironment::GetInstance();
  JNIEnv* j_env = instance->AttachCurrentThread();

  if (instance->GetMethods().j_fetch_resource_async_method_id) {
    int64_t id = SetRequestCB(cb);
    jstring j_relative_path = JniUtils::StrViewToJString(j_env, uri);
    j_env->CallVoidMethod(bridge_->GetObj(),
                          instance->GetMethods().j_fetch_resource_async_method_id,
                          j_relative_path, id);
    j_env->DeleteLocalRef(j_relative_path);
    return true;
  }

  TDF_BASE_DLOG(ERROR) << "jni fetch_resource_method_id error";
  return false;
}

void OnResourceReady(JNIEnv* j_env,
                     jobject j_object,
                     jobject j_byte_buffer,
                     jlong j_runtime_id,
                     jlong j_request_id) {
  TDF_BASE_DLOG(INFO) << "HippyBridgeImpl onResourceReady j_runtime_id = "
                      << j_runtime_id;
  std::shared_ptr<Runtime> runtime = Runtime::Find(j_runtime_id);
  if (!runtime) {
    TDF_BASE_DLOG(WARNING)
        << "HippyBridgeImpl onResourceReady, j_runtime_id invalid";
    return;
  }
  std::shared_ptr<Scope> scope = runtime->GetScope();
  if (!scope) {
    TDF_BASE_DLOG(WARNING) << "HippyBridgeImpl onResourceReady, scope invalid";
    return;
  }

  std::shared_ptr<ADRLoader> loader =
      std::static_pointer_cast<ADRLoader>(scope->GetUriLoader());
  int64_t request_id = j_request_id;
  TDF_BASE_DLOG(INFO) << "request_id = " << request_id;
  auto cb = loader->GetRequestCB(request_id);
  if (!cb) {
    TDF_BASE_DLOG(WARNING) << "cb not found" << request_id;
    return;
  }
  if (!j_byte_buffer) {
    TDF_BASE_DLOG(INFO) << "HippyBridgeImpl onResourceReady, buff null";
    cb(UriLoader::RetCode::Success, UriLoader::bytes());
    return;
  }
  int64_t len = (j_env)->GetDirectBufferCapacity(j_byte_buffer);
  TDF_BASE_DLOG(INFO) << "len = " << len;
  if (len == -1) {
    TDF_BASE_DLOG(ERROR)
        << "HippyBridgeImpl onResourceReady, BufferCapacity error";
    cb(UriLoader::RetCode::Failed, UriLoader::bytes());
    return;
  }
  void* buff = (j_env)->GetDirectBufferAddress(j_byte_buffer);
  if (!buff) {
    TDF_BASE_DLOG(INFO) << "HippyBridgeImpl onResourceReady, buff null";
    cb(UriLoader::RetCode::Failed, UriLoader::bytes());
    return;
  }

  UriLoader::bytes str(reinterpret_cast<const char*>(buff), len);
  cb(UriLoader::RetCode::Success, std::move(str));
}

REGISTER_JNI("com/tencent/mtt/hippy/bridge/HippyBridgeImpl",
             "onResourceReady",
             "(Ljava/nio/ByteBuffer;JJ)V",
             OnResourceReady)

std::function<void(UriLoader::RetCode,
                   UriLoader::bytes)> ADRLoader::GetRequestCB(int64_t request_id) {
  auto it = request_map_.find(request_id);
  return it != request_map_.end() ? it->second : nullptr;
}

int64_t ADRLoader::SetRequestCB(std::function<void(RetCode, bytes)> cb) {
  int64_t id = global_request_id.fetch_add(1);
  request_map_.insert({id, cb});
  return id;
}
