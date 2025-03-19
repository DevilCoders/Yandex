#pragma once

// @author: smikler

// @see: http://publicsuffix.org/
// @see: http://mxr.mozilla.org/mozilla-central/source/netwerk/dns/effective_tld_names.dat?raw=1

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/memory/segmented_string_pool.h>
#include <util/stream/input.h>

class TPublicSuffixStorage
{
public:
    enum ESuffixState
    {
        SS_UNKNOWN = 0,
        SS_OK      = 1,
        SS_EXTEND  = 2,
        SS_EXCEPT  = 4,
    };
private:
    THashMap<TStringBuf,ESuffixState> States;
    segmented_string_pool Names;

    void AddState(const TStringBuf& name, ESuffixState state);
public:
    TPublicSuffixStorage(const size_t size = 1024 * 1024)
        : Names(size)
    {
    }

    ESuffixState GetState(const TStringBuf& name) const;

    void Clear();

    // Note: expects utf8 input. Recodes to punycode automatically if needed.
    void FillFromStream(IInputStream& input);
    void FillFromFile(const TString& fileName);
};

// Note: host should be punycoded (you may use quality/util/url_utils.h for it for example).
TStringBuf GetHostOwner(const TStringBuf& host, const TPublicSuffixStorage& storage);
