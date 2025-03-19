#include "storage.h"
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/tvm_services/abstract/request/abstract.h>

#include <aws/core/Aws.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

namespace NCS {
    namespace NKVStorage {
        bool TS3Storage::DoPutData(const TString& path, const TBlob& data, const TWriteOptions& options) const {
            const TString folder = options.GetVisibilityScope() ? options.GetVisibilityScope() : DefaultFolder;
            auto req = Aws::S3::Model::PutObjectRequest{}.WithBucket(Bucket).WithKey(folder + "/" + path);
            auto inputData = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream", std::stringstream::in | std::stringstream::out | std::stringstream::binary);
            inputData->write((const char*)data.Data(), data.Size());
            req.SetBody(inputData);
            auto resp = Client->PutObject(req);
            if (!resp.IsSuccess()) {
                TFLEventLog::Error("cannot put data")("error", resp.GetError().GetMessage());
                return false;
            }
            return true;
        }

        bool TS3Storage::DoGetData(const TString& path, TMaybe<TBlob>& data, const TReadOptions& options) const {
            const TString folder = options.GetVisibilityScope() ? options.GetVisibilityScope() : DefaultFolder;
            auto req = Aws::S3::Model::GetObjectRequest{}.WithBucket(Bucket).WithKey(folder + "/" + path);
            auto resp = Client->GetObject(req);
            if (!resp.IsSuccess()) {
                TFLEventLog::Error("cannot get data")("error", resp.GetError().GetMessage());
                return false;
            }

            std::stringstream ss;
            ss << resp.GetResult().GetBody().rdbuf();
            data = TBlob::FromString(ss.str());
            return true;
        }

        TString TS3Storage::DoGetPathByHash(const TBlob& /*data*/, const TReadOptions& /*options*/) const {
            TFLEventLog::Error("unimplemented");
            return "";
        }

    }
}
