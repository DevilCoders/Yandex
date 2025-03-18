#include "git_hash.h"

#include <util/string/builder.h>
#include <util/string/cast.h>

#include <contrib/libs/openssl/include/openssl/sha.h>

namespace NAapi {

TString GitLikeHash(const TStringBuf data) {
    const TString tail = TStringBuilder{} << '\0' << ToString(data.size());

    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data.data(), data.size());
    SHA1_Update(&ctx, tail.data(), tail.size());

    unsigned char sha1[SHA_DIGEST_LENGTH];
    SHA1_Final(sha1, &ctx);

    return TString(reinterpret_cast<const char*>(sha1), SHA_DIGEST_LENGTH);
}

} // namespace NAapi
