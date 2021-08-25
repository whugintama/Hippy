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

std::function<void(UriLoader::SyncContext & , std::function<std::shared_ptr<Delegate>()>)>
    DebugDelegate::on_request_untrusted_content_sync_ = [](UriLoader::SyncContext &ctx,
                                                           std::function<std::shared_ptr<Delegate>()> next) {
  std::shared_ptr<Delegate> next_delegate = next();
  if (next_delegate) {
    next_delegate->RequestUntrustedContent(ctx, next);
  } else {
    ctx.ret_code = UriLoader::RetCode::SchemeNotRegister;
  }
};

std::function<void(UriLoader::ASyncContext & , std::function<std::shared_ptr<Delegate>()>)>
    DebugDelegate::on_request_untrusted_content_async_ = [](UriLoader::ASyncContext &ctx,
                                                            std::function<std::shared_ptr<Delegate>()> next) {
  std::shared_ptr<Delegate> next_delegate = next();
  if (next_delegate) {
    next_delegate->RequestUntrustedContent(ctx, next);
  } else {
    ctx.cb(UriLoader::RetCode::SchemeNotRegister, UriLoader::bytes());
  }
};

DebugDelegate::DebugDelegate(std::shared_ptr<JavaRef> bridge): bridge_(bridge) {
  RegisterDebugDelegate();
}

void DebugDelegate::RegisterDebugDelegate() {

}

void DebugDelegate::RequestUntrustedContent(
    UriLoader::SyncContext &ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  if (on_request_untrusted_content_sync_) {
    on_request_untrusted_content_sync_(ctx, next);
  }
}

void DebugDelegate::RequestUntrustedContent(
    UriLoader::ASyncContext &ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  if (on_request_untrusted_content_async_) {
    on_request_untrusted_content_async_(ctx, next);
  }
}
