#pragma once

#include <kernel/ugc/security/lib/secret_manager.h>
#include <kernel/ugc/security/proto/token.pb.h>

#include <util/datetime/base.h>

#include <google/protobuf/message.h>

using google::protobuf::Message;

namespace NUgc {
    namespace NSecurity {
        TString EncryptToken(
            const TSecretManager& secretManager,
            const Message& data,
            TTokenAad aad = TTokenAad());

        bool DecryptToken(
            const TSecretManager& secretManager,
            const TString& token,
            Message* data,
            TTokenAad* aad = nullptr,
            TDuration defaultTtl = TDuration::Max());
    } // namespace NSecurity
} // namespace NUgc
