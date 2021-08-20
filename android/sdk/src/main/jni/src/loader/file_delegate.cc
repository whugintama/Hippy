#include "loader/file_delegate.h"
#include "jni/uri.h"

using UriLoader = hippy::base::UriLoader;
using HippyFile = hippy::base::HippyFile;

void FileDelegate::RequestUntrustedContent(
    UriLoader::SyncContext& ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  unicode_string_view uri = ctx.uri;
  std::shared_ptr<Uri> uri_obj = Uri::Create(uri);
  unicode_string_view path = uri_obj->GetPath();
  if (path.encoding() == unicode_string_view::Encoding::Unkown) {
    ctx.ret_code = UriLoader::RetCode::PathError;
    return;
  }
  bool ret = HippyFile::ReadFile(path, ctx.content, false);
  if (ret) {
    ctx.ret_code = UriLoader::RetCode::Success;
  } else {
    ctx.ret_code = UriLoader::RetCode::Failed;
  }
}

void FileDelegate::RequestUntrustedContent(
    UriLoader::ASyncContext& ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  unicode_string_view uri = ctx.uri;
  std::shared_ptr<Uri> uri_obj = Uri::Create(uri);
  unicode_string_view path = uri_obj->GetPath();
  if (path.encoding() == unicode_string_view::Encoding::Unkown) {
    ctx.cb(UriLoader::RetCode::PathError, UriLoader::bytes());
    return;
  }
  LoadByFile(path, ctx.cb);
}

bool FileDelegate::LoadByFile(const unicode_string_view& path,
                           std::function<void(UriLoader::RetCode, UriLoader::bytes)> cb) {
  std::shared_ptr<WorkerTaskRunner> runner = runner_.lock();
  if (!runner) {
    return false;
  }
  std::unique_ptr<CommonTask> task = std::make_unique<CommonTask>();
  task->func_ = [path, cb] {
    UriLoader::bytes content;
    bool ret = HippyFile::ReadFile(path, content, false);
    if (ret) {
      cb(UriLoader::RetCode::Success, std::move(content));
    } else {
      cb(UriLoader::RetCode::Failed, std::move(content));
    }
  };
  runner->PostTask(std::move(task));
  return true;
}
