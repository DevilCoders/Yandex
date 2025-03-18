#pragma once

#include <util/generic/fwd.h>
#include <util/generic/string.h>

#include <cstdint>
#include <ostream>

namespace NAntiAdBlock {

    struct TDecryptResult {
        TString url;
        TString seed;
        TString origin;
    };

    typedef std::pair<TString, TString> TPartsCryptUrl;

    const int SEED_LENGTH = 6;
    const int URL_LENGTH_PREFIX_LENGTH = 9;

    const TString GetKey(TStringBuf secret, TStringBuf data);
    const TString DecryptBase64(const TStringBuf data);
    const TString DecryptXor(const TStringBuf data, const TStringBuf key);
    bool IsCryptedUrl(const TStringBuf& crypted_url, const TStringBuf& crypt_prefix);
    const TPartsCryptUrl ResplitUsingLength(int length, const TStringBuf& cry_part, const TStringBuf& noncry_part);
    const TDecryptResult DecryptUrl(const TStringBuf& crypted_url, const TStringBuf& secret_key,
                                           const TStringBuf& crypt_prefix, int is_trailing_slash_enabled);
}
