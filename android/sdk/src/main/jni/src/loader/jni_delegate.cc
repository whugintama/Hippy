#include "loader/jni_delegate.h"
#include "jni/jni_register.h"
#include "bridge/runtime.h"
#include "loader/adr_loader.h"

using unicode_string_view = tdf::base::unicode_string_view;
using UriLoader = hippy::base::UriLoader;

static std::atomic<int64_t> global_request_id{0};
std::unordered_map<int64_t, std::function<void(UriLoader::RetCode, UriLoader::bytes)>>
    JniDelegate::request_map_ =
        std::unordered_map<int64_t, std::function<void(UriLoader::RetCode, UriLoader::bytes)>>{};

void JniDelegate::RequestUntrustedContent(
    UriLoader::SyncContext& ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  std::shared_ptr<JNIEnvironment> instance = JNIEnvironment::GetInstance();
  JNIEnv* j_env = instance->AttachCurrentThread();
  if (instance->GetMethods().j_fetch_resource_sync_method_id) {
    jstring j_relative_path = JniUtils::StrViewToJString(j_env, ctx.uri);
    jobject ret = j_env->CallObjectMethod(bridge_->GetObj(),
                                          instance->GetMethods().j_fetch_resource_sync_method_id,
                                          j_relative_path);
    j_env->DeleteLocalRef(j_relative_path);
    jclass j_cls = j_env->GetObjectClass(ret);
    jfieldID ret_code_field = j_env->GetFieldID(j_cls, "retCode", "I");
    jint j_ret_code = j_env->GetIntField(ret, ret_code_field);
    auto ret_code = static_cast<UriLoader::RetCode>(j_ret_code);
    if (ret_code != UriLoader::RetCode::PathNotMatch) {
      jfieldID bytes_field = j_env->GetFieldID(j_cls, "content", "Ljava/nio/ByteBuffer;");
      jobject j_byte_buffer = j_env->GetObjectField(j_cls, bytes_field);
      if (!j_byte_buffer) {
        TDF_BASE_DLOG(INFO) << "RequestUntrustedContent sync, buff null";
        ctx.ret_code = UriLoader::RetCode::Success;
        ctx.content = UriLoader::bytes();
        return;
      }
      int64_t len = (j_env)->GetDirectBufferCapacity(j_byte_buffer);
      if (len == -1) {
        TDF_BASE_DLOG(ERROR) << "RequestUntrustedContent sync, BufferCapacity error";
        ctx.ret_code = UriLoader::RetCode::DelegateError;
        return;
      }
      void* buff = (j_env)->GetDirectBufferAddress(j_byte_buffer);
      if (!buff) {
        TDF_BASE_DLOG(INFO) << "RequestUntrustedContent sync, buff null";
        ctx.ret_code = UriLoader::RetCode::DelegateError;
        return;
      }
      UriLoader::bytes str(reinterpret_cast<const char*>(buff), len);
      ctx.ret_code = UriLoader::RetCode::Success;
      ctx.content = std::move(str);
    } else {
      auto next_delegate = next();
      if (next_delegate) {
        next_delegate->RequestUntrustedContent(ctx, next);
      }
    }
  }
  ctx.ret_code = UriLoader::RetCode::DelegateError;
}
void JniDelegate::RequestUntrustedContent(
    UriLoader::ASyncContext& ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  auto new_cb = [&ctx, next](UriLoader::RetCode ret_code, UriLoader::bytes content) {
    if (ret_code != UriLoader::RetCode::PathNotMatch) {
      ctx.cb(ret_code, content);
    } else {
      auto next_delegate = next();
      if (next_delegate) {
        next_delegate->RequestUntrustedContent(ctx, next);
      }
    }
  };
  std::shared_ptr<JNIEnvironment> instance = JNIEnvironment::GetInstance();
  JNIEnv* j_env = instance->AttachCurrentThread();
  if (instance->GetMethods().j_fetch_resource_async_method_id) {
    int64_t id = SetRequestCB(new_cb);
    jstring j_relative_path = JniUtils::StrViewToJString(j_env, ctx.uri);
    j_env->CallVoidMethod(bridge_->GetObj(),
                          instance->GetMethods().j_fetch_resource_async_method_id,
                          j_relative_path, id);
    j_env->DeleteLocalRef(j_relative_path);
    return;
  }
  ctx.cb(UriLoader::RetCode::DelegateError, UriLoader::bytes());
}

void OnResourceReady(JNIEnv* j_env,
                     jobject j_object,
                     jobject j_byte_buffer,
                     jlong j_runtime_id,
                     jlong j_request_id) {
  TDF_BASE_DLOG(INFO) << "onResourceReady j_runtime_id = "
                      << j_runtime_id;
  std::shared_ptr<Runtime> runtime = Runtime::Find(j_runtime_id);
  if (!runtime) {
    TDF_BASE_DLOG(WARNING) << "onResourceReady, j_runtime_id invalid";
    return;
  }
  std::shared_ptr<Scope> scope = runtime->GetScope();
  if (!scope) {
    TDF_BASE_DLOG(WARNING) << "onResourceReady, scope invalid";
    return;
  }

  int64_t request_id = j_request_id;
  TDF_BASE_DLOG(INFO) << "request_id = " << request_id;
  auto cb = JniDelegate::GetRequestCB(request_id);
  if (!cb) {
    TDF_BASE_DLOG(WARNING) << "cb not found" << request_id;
    return;
  }
  if (!j_byte_buffer) {
    TDF_BASE_DLOG(INFO) << "onResourceReady, buff null";
    cb(UriLoader::RetCode::Success, UriLoader::bytes());
    return;
  }
  int64_t len = (j_env)->GetDirectBufferCapacity(j_byte_buffer);
  TDF_BASE_DLOG(INFO) << "len = " << len;
  if (len == -1) {
    TDF_BASE_DLOG(ERROR) << "onResourceReady, BufferCapacity error";
    cb(UriLoader::RetCode::DelegateError, UriLoader::bytes());
    return;
  }
  void* buff = (j_env)->GetDirectBufferAddress(j_byte_buffer);
  if (!buff) {
    TDF_BASE_DLOG(INFO) << "onResourceReady, buff null";
    cb(UriLoader::RetCode::DelegateError, UriLoader::bytes());
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
                   UriLoader::bytes)> JniDelegate::GetRequestCB(int64_t request_id) {
  auto it = request_map_.find(request_id);
  return it != request_map_.end() ? it->second : nullptr;
}

int64_t JniDelegate::SetRequestCB(std::function<void(UriLoader::RetCode, UriLoader::bytes)> cb) {
  int64_t id = global_request_id.fetch_add(1);
  request_map_.insert({id, cb});
  return id;
}
