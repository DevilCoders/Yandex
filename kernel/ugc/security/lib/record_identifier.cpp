#include "record_identifier.h"

#include "exception.h"

#include <kernel/ugc/security/lib/internal/crypto.h>

#include <library/cpp/string_utils/base64/base64.h>

namespace NUgc {
    namespace NSecurity {
        namespace {
            // TODO: move this to secure storage as a real secret. For now, let's assume people with
            // access to Arcadia also have access to UGCDB.
            const TString HASH_KEY = "2017-06-21 UGC hash key.";

            const int MIN_NAMESPACE_LENGTH = 1;
            const int MIN_USERID_LENGTH = 1;

            // Minimum number of base64 characters in output hash. 30 characters is 180 bits.
            const int MIN_IDENTIFIER_LENGTH = 30;

            // Maximum number of characters. The purpose of variable-length identifiers is to
            // prevent people from hardcoding assumptions about identifier length.
            const int MAX_IDENTIFIER_LENGTH = 34;
        }

        TString GenerateRecordIdentifierByHash(const TRecordIdentifierBundle& bundle) {
            // Check that the bundle is set up correctly.
            if (bundle.GetNamespace().size() < MIN_NAMESPACE_LENGTH) {
                ythrow TApplicationException()
                    << "Namespace [" << bundle.GetNamespace() << "] is too short.";
            }

            if (bundle.GetUserId().size() < MIN_USERID_LENGTH) {
                ythrow TApplicationException()
                    << "UserId [" << bundle.GetUserId() << "] is too short.";
            }

            // Proto serialization has a few nice properties: it is relatively stable and every
            // field is prefixed with its tag, type and (if needed) length. This prevents extension
            // attacks.
            TString message = bundle.SerializeAsString();

            // HMAC is a bit of overkill, but it will make security reviewers feel warm and fuzzy.
            // Ultimately, it's a salted hash of a salted hash. SHA-512 is also overkill, but it's
            // faster than SHA-256 on a 64-bit machine, so why not.
            TString hash = NInternal::Hmac(HASH_KEY, message);

            // We're not going to use all these bits. Save a few nanoseconds in base64.
            hash.remove(30);

            // Prepare the output string.
            TString base64 = Base64EncodeUrl(hash);

            // Drop even more bits. Also, drop any base64 padding.
            int hashLength = MIN_IDENTIFIER_LENGTH;

            // The last 8 bits in the hash will not survive, but can drive the variable length.
            unsigned int random = hash.back();
            hashLength += random % (MAX_IDENTIFIER_LENGTH - MIN_IDENTIFIER_LENGTH);

            base64.remove(hashLength);

            return base64;
        }

        TString TRecordIdentifierGenerator::Build() {
            return GenerateRecordIdentifierByHash(bundle);
        }
    } // namespace NSecurity
} // namespace NUgc
