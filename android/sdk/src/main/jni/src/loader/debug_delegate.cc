#include "loader/debug_delegate.h"

#include "loader/asset_delegate.h"

#include "bridge/runtime.h"
#include "jni/uri.h"
#include "jni/jni_register.h"
#include "jni/jni_utils.h"

using UriLoader = hippy::base::UriLoader;
using Delegate = UriLoader::Delegate;
using HippyFile = hippy::base::HippyFile;
using unicode_string_view = tdf::base::unicode_string_view;

DebugDelegate::DebugDelegate(std::shared_ptr<JavaRef> bridge): bridge_(bridge) {
  RegisterDebugDelegate();
}

void DebugDelegate::RegisterDebugDelegate() {

}

void DebugDelegate::RequestUntrustedContent(
    UriLoader::SyncContext &ctx,
    std::function<void(UriLoader::SyncContext&)> next) {
  if (next) {
    next(ctx);
  } else {
    ctx.ret_code = UriLoader::RetCode::SchemeNotRegister;
  }
}

void DebugDelegate::RequestUntrustedContent(
    UriLoader::ASyncContext &ctx,
    std::function<void(UriLoader::ASyncContext&)> next) {
  if (next) {
    next(ctx);
  } else {
    ctx.cb(UriLoader::RetCode::SchemeNotRegister, UriLoader::bytes());
  }
}
