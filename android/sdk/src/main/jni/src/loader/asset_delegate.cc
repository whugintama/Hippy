#include "loader/asset_delegate.h"

#include "jni/uri.h"

using UriLoader = hippy::base::UriLoader;
using HippyFile = hippy::base::HippyFile;

template <typename CharType>
bool ReadAsset(const tdf::base::unicode_string_view& path,
               AAssetManager* aasset_manager,
               std::basic_string<CharType>& bytes,
               bool is_auto_fill) {
  tdf::base::unicode_string_view owner(""_u8s);
  const char* asset_path = hippy::base::StringViewUtils::ToConstCharPointer(path, owner);
  std::string file_path = std::string(asset_path);
  if (file_path.length() > 0 && file_path[0] == '/') {
    file_path = file_path.substr(1);
    asset_path = file_path.c_str();
  }
  TDF_BASE_DLOG(INFO) << "asset_path = " << asset_path;

  auto asset =
      AAssetManager_open(aasset_manager, asset_path, AASSET_MODE_STREAMING);
  if (asset) {
    int size = AAsset_getLength(asset);
    if (is_auto_fill) {
      size += 1;
    }
    bytes.resize(size);
    int offset = 0;
    int readbytes;
    while ((readbytes = AAsset_read(asset, &bytes[0] + offset,
                                    bytes.size() - offset)) > 0) {
      offset += readbytes;
    }
    if (is_auto_fill) {
      bytes.back() = '\0';
    }
    AAsset_close(asset);
    TDF_BASE_DLOG(INFO) << "path = " << path << ", len = " << bytes.length()
                        << ", file_data = "
                        << reinterpret_cast<const char*>(bytes.c_str());
    return true;
  }
  TDF_BASE_DLOG(INFO) << "ReadFile fail, file_path = " << file_path;
  return false;
}

void AssetDelegate::RequestUntrustedContent(
    UriLoader::SyncContext& ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  unicode_string_view uri = ctx.uri;
  std::shared_ptr<Uri> uri_obj = Uri::Create(uri);
  unicode_string_view path = uri_obj->GetPath();
  if (path.encoding() == unicode_string_view::Encoding::Unkown) {
    ctx.ret_code = UriLoader::RetCode::PathError;
    return;
  }
  bool ret = ReadAsset(path, aasset_manager_, ctx.content, false);
  if (ret) {
    ctx.ret_code = UriLoader::RetCode::Success;
  } else {
    ctx.ret_code = UriLoader::RetCode::Failed;
  }
}

void AssetDelegate::RequestUntrustedContent(
    UriLoader::ASyncContext& ctx,
    std::function<std::shared_ptr<Delegate>()> next) {
  unicode_string_view uri = ctx.uri;
  std::shared_ptr<Uri> uri_obj = Uri::Create(uri);
  unicode_string_view path = uri_obj->GetPath();
  if (path.encoding() == unicode_string_view::Encoding::Unkown) {
    ctx.cb(UriLoader::RetCode::PathError, UriLoader::bytes());
    return;
  }
  LoadByAsset(path, ctx.cb);
}

bool AssetDelegate::LoadByAsset(const unicode_string_view& path,
                            std::function<void(UriLoader::RetCode, UriLoader::bytes)> cb,
                            bool is_auto_fill) {
  TDF_BASE_DLOG(INFO) << "ReadAssetFile file_path = " << path;
  std::shared_ptr<WorkerTaskRunner> runner = runner_.lock();
  if (!runner) {
    return false;
  }
  std::unique_ptr<CommonTask> task = std::make_unique<CommonTask>();
  task->func_ = [path, aasset_manager = aasset_manager_, is_auto_fill, cb] {
    UriLoader::bytes content;
    bool ret = ReadAsset(path, aasset_manager, content, is_auto_fill);
    if (ret) {
      cb(UriLoader::RetCode::Success, std::move(content));
    } else {
      cb(UriLoader::RetCode::Failed, std::move(content));
    }
  };
  runner->PostTask(std::move(task));
  return true;
}
