#include "exception.h"
#include "token.h"

#include <kernel/ugc/security/lib/internal/crypto.h>
#include <kernel/ugc/security/lib/internal/random.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/guid.h>
#include <util/stream/file.h>
#include <util/system/unaligned_mem.h>

#include <google/protobuf/text_format.h>

namespace NUgc {
    namespace NSecurity {
        TString EncryptToken(
            const TSecretManager& secretManager,
            const Message& data,
            TTokenAad aad)
        {
            if (aad.GetCreationTimeMs() == 0) {
                aad.SetCreationTimeMs(TInstant::Now().MilliSeconds());
            }

            char nonce[NInternal::GCM_NONCE_LEN];
            if (size_t(NInternal::GCM_NONCE_LEN) != NInternal::RandomBytes(nonce, NInternal::GCM_NONCE_LEN)) {
                ythrow TInternalException() << "Failed to generage random nonce.";
            }

            TCryptoBundle bundle;
            bundle.SetPlaintext(data.SerializeAsString());
            bundle.SetAad(aad.SerializeAsString());
            bundle.SetNonce(nonce, NInternal::GCM_NONCE_LEN);

            const TString& key = secretManager.GetKeyByDate(TInstant::MilliSeconds(aad.GetCreationTimeMs()));
            NInternal::Aes256GcmEncrypt(key, bundle);

            TString serialized = bundle.SerializeAsString();
            return Base64Encode(serialized); // base64 encode for usage in json
        }

        bool DecryptToken(
            const TSecretManager& secretManager,
            const TString& token,
            Message* data,
            TTokenAad* aad,
            TDuration defaultTtl)
        {
            TCryptoBundle bundle;
            try {
                TString decoded = Base64Decode(token);
                if (!bundle.ParseFromString(decoded)) {
                    return false;
                }
            } catch (const yexception&) {
                // bad base64 encoding
                return false;
            }

            TTokenAad localAad;
            if (!localAad.ParseFromString(bundle.GetAad())) {
                return false;
            }

            TDuration ttl = TDuration::MilliSeconds(localAad.GetTtlMs());
            if (ttl == TDuration::Zero()) {
                ttl = defaultTtl;
            }
            if (TInstant::Now() - TInstant::MilliSeconds(localAad.GetCreationTimeMs()) > ttl) {
                return false;
            }

            if (aad != nullptr) {
                *aad = localAad;
            }

            try {
                const TString& key = secretManager.GetKeyByDate(TInstant::MilliSeconds(localAad.GetCreationTimeMs()));
                if (!NInternal::Aes256GcmDecrypt(key, bundle)) {
                    return false;
                }
            } catch (const TApplicationException&) {
                // key not found -> bad date
                return false;
            }

            return data->ParseFromString(bundle.GetPlaintext());
        }

    } // namespace NSecurity
} // namespace NUgc
