#pragma once
#include <kernel/common_server/library/kv/abstract/storage.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <util/system/mutex.h>

#include <aws/s3/S3Client.h>

namespace NCS {
    namespace NKVStorage {
        class TS3Storage: public IStorage {
        private:
            using TBase = IStorage;
            CSA_DEFAULT(TS3Storage, TString, Bucket);
            CSA_DEFAULT(TS3Storage, TString, DefaultFolder);
            CS_HOLDER(Aws::S3::S3Client, Client);
        protected:
            virtual bool DoPutData(const TString& path, const TBlob& data, const TWriteOptions& options) const override;
            virtual bool DoGetData(const TString& path, TMaybe<TBlob>& data, const TReadOptions& options) const override;
            virtual TString DoGetPathByHash(const TBlob& data, const TReadOptions& options) const override;
            virtual bool DoPrepareEnvironment(const TBaseOptions& /*options*/) const override {
                return true;
            }
        public:
            TS3Storage(const TString& storageName, const ui32 threadsCount, const TString& bucket, const TString& defaultFolder, THolder<Aws::S3::S3Client>&& client)
                : TBase(storageName, threadsCount)
                , Bucket(bucket)
                , DefaultFolder(defaultFolder)
                , Client(std::move(client))
            {
                CHECK_WITH_LOG(Client);
            }
        };
    }
}
