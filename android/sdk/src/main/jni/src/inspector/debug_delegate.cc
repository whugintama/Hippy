#include "inspector/debug_delegate.h"

#include "loader/asset_delegate.h"

#include "jni/uri.h"

using UriLoader = hippy::base::UriLoader;
using HippyFile = hippy::base::HippyFile;

void DebugDelegate::RequestUntrustedContent(
    UriLoader::SyncContext& ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  std::shared_ptr<Delegate> next_delegate = next();
  if (next_delegate) {
    next_delegate->RequestUntrustedContent(ctx, next);
  } else {
    ctx.ret_code = UriLoader::RetCode::SchemeNotRegister;
  }
}

void DebugDelegate::RequestUntrustedContent(
    UriLoader::ASyncContext& ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  std::shared_ptr<Delegate> next_delegate = next();
  if (next_delegate) {
    next_delegate->RequestUntrustedContent(ctx, next);
  } else {
    ctx.cb(UriLoader::RetCode::SchemeNotRegister, UriLoader::bytes());
  }
}
