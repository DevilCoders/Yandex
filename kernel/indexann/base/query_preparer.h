#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>


namespace NIndexAnn {

    typedef TUtf16String TKeyInvText;

    class TQueryPreparer {
    public:
        struct TConfig {
            size_t MinLength;
            size_t MaxLength;
            bool   Verbose;
            bool   Reject;

            TConfig(size_t minLength = 3, size_t maxLength = 120, bool verbose = false, bool reject = true)
                : MinLength(minLength)
                , MaxLength(maxLength)
                , Verbose(verbose)
                , Reject(reject)
            {
            }
        };

    private:
        bool EnsureSize(TKeyInvText& result) const;
    public:
        TQueryPreparer(const TConfig& config = TConfig());

        bool    TryConvert(const TStringBuf& queryUTF8, TKeyInvText& result) const;
        TKeyInvText Convert(const TStringBuf& queryUTF8) const;

    public:
        const TConfig Config;
    };

}
