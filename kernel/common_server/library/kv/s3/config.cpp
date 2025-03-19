#include "config.h"
#include <kernel/common_server/abstract/frontend.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

namespace NCS {
    namespace NKVStorage {
        TS3Config::TFactory::TRegistrator<TS3Config> TS3Config::Registrator("s3");

        IStorage::TPtr TS3Config::DoBuildStorage() const {
            Aws::Client::ClientConfiguration conf;
            conf.region = Region;
            conf.scheme = Aws::Http::Scheme::HTTPS;
            conf.caPath = CaPath;
            conf.endpointOverride = Endpoint;
            conf.verifySSL = true;

            std::shared_ptr<Aws::Auth::AWSCredentialsProvider> authProvider;
            authProvider = std::make_shared<Aws::Auth::SimpleAWSCredentialsProvider>(KeyId, SecretKey);
            return new TS3Storage(GetStorageName(), GetThreadsCount(), Bucket, DefaultFolder, MakeHolder<Aws::S3::S3Client>(authProvider, conf));
        }

    }
}
