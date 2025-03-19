#pragma once

#include <kernel/hosts/extowner/ext_owner.h>

#include <util/ysaveload.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>

struct TOwnerId {
    ui64 Owner;

    TOwnerId(ui64 o = 0)
        : Owner(o)
    {
    }

    TOwnerId(const TOwnerId&) = default;

    TOwnerId& operator=(const TOwnerId&) = default;

    bool operator == (const TOwnerId& o) const {
        return Owner == o.Owner;
    }

    bool operator < (const TOwnerId& o) const {
        // Warning! Comparison is bytewise-lexicographic (as binary keys/subkeys in MR)
        return memcmp(&Owner, &o.Owner, sizeof(ui64)) < 0;
    }
};

struct TPathId {
    ui64 Path;

    TPathId(ui64 p = 0)
        : Path(p)
    {
    }

    TPathId(const TPathId&) = default;

    TPathId& operator=(const TPathId&) = default;

    bool operator == (const TPathId& p) const {
        return Path == p.Path;
    }
};

struct TComplexId: public TOwnerId, public TPathId {
    TComplexId(TOwnerId o = TOwnerId(), TPathId p = TPathId())
        : TOwnerId(o)
        , TPathId(p)
    {
    }

    TComplexId(const TComplexId&) = default;

    TComplexId& operator=(const TComplexId&) = default;

    bool operator == (const TComplexId& id) const {
        return Owner == id.Owner && Path == id.Path;
    }

    bool operator != (const TComplexId& id) const {
        return Owner != id.Owner || Path != id.Path;
    }

    bool operator < (const TComplexId& id) const {
        auto compareResult = memcmp(&Owner, &id.Owner, sizeof(ui64));
        return compareResult < 0 || (compareResult == 0 && memcmp(&Path, &id.Path, sizeof(ui64)) < 0);
    }
};

Y_DECLARE_PODTYPE(TComplexId);

class TComplexIdBuilder {
    const TOwnerExtractor Extractor;

public:
    // Multiple areas files can be passed as a colon-separated list
    TComplexIdBuilder(const TString& areasFilename)
        : Extractor(areasFilename)
    {
    }

    TComplexIdBuilder(const TVector<TString>& areas)
        : Extractor(areas)
    {
    }

    TComplexId Do(TStringBuf host, TStringBuf owner, TStringBuf path, TStringBuf ext = TStringBuf()) const;
    TComplexId Do(TStringBuf host, TStringBuf owner, TStringBuf port, TStringBuf ext, ui64 pathId) const;

    TComplexId FromParts(TStringBuf host, TStringBuf path) const {
        std::pair<TStringBuf, TStringBuf> owner(Extractor.GetOwnerFromParts(host, path));
        return Do(
            owner.first.empty() ? host : TStringBuf(host.begin(), owner.first.begin()),
            owner.first,
            owner.second.empty() ? path : TStringBuf(owner.second.end(), path.end()),
            owner.second
        );
    }

    std::pair<TComplexId, TStringBuf> WithOwner(TStringBuf url) const {
        TStringBuf owner(Extractor.GetOwner(url));
        return std::make_pair(
            Do(
                owner.empty() ? TStringBuf() : TStringBuf(url.begin(), owner.begin()),
                owner,
                owner.empty() ? url : TStringBuf(owner.end(), url.end())
            ),
            owner
        );
    }

    TComplexId Do(TStringBuf url) const {
        return WithOwner(url).first;
    }

    TComplexId Do(const char* begin, const char* end) const {
        return Do(TStringBuf(begin, end));
    }

    TComplexId Do(const TString& url) const {
        return Do(TStringBuf(url.begin(), url.end()));
    }

    TComplexId Do(const char* url) const {
        return Do(TStringBuf(url));
    }

    const TOwnerExtractor& GetExtractor() const {
        return Extractor;
    }
};

template <>
struct THash<TComplexId> {
    size_t operator() (TComplexId id) const {
        return id.Owner ^ id.Path;
    }
};

template <>
struct TEqualTo<TComplexId> {
    bool operator() (TComplexId a, TComplexId b) const {
        return a.Owner == b.Owner && a.Path == b.Path;
    }
};

TComplexId ParseComplexId(const TStringBuf& s);

ui64 HashOwner(const TString& s);
