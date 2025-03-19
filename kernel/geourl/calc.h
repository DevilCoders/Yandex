#pragma once

#include <util/generic/noncopyable.h>
#include <util/memory/blob.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/split.h>
#include <util/string/printf.h>

#include <library/cpp/on_disk/aho_corasick/helpers.h>
#include <library/cpp/on_disk/chunks/chunked_helpers.h>
#include <library/cpp/containers/comptrie/chunked_helpers_trie.h>

template<bool Writer>
class TGeoUrlCalcer : TNonCopyable {
private:
    TBlob Data;
    typedef typename TTrieMapG<i32, Writer>::T TRegMap;
    TRegMap RegTlds;
    TRegMap RegDomains;
    TRegMap RegCodes;
    typename TDefaultAhoCorasickG<Writer>::T Searcher;
    typename TYVectorG<i32, Writer>::T Geos;
    typename TYVectorG<i32, Writer>::T Flags;
    typename TStringsVectorG<Writer>::T Substrs;
    TRegMap GeoTrie;

public:
    struct TResult {
        i32 Region;
        TString Rule;
        TString SubStr;

        TResult(i32 region, const TString& rule, const TString& substr)
            : Region(region)
            , Rule(rule)
            , SubStr(substr)
        {
        }

        TString ToString() const {
            return Sprintf("%d\t%s\t%s", Region, Rule.data(), SubStr.data());
        }
    };

    TGeoUrlCalcer() {
    }

    TGeoUrlCalcer(TBlob blob)
        : Data(blob)
        , RegTlds(GetBlock(blob, 1))
        , RegDomains(GetBlock(blob, 2))
        , RegCodes(GetBlock(blob, 3))
        , Searcher(GetBlock(blob, 4))
        , Geos(GetBlock(blob, 5))
        , Flags(GetBlock(blob, 6))
        , Substrs(GetBlock(blob, 7))
        , GeoTrie(GetBlock(blob, 8))
    {
        const ui32 version = TSingleValue<ui32>(GetBlock(blob, 0)).Get();
        if (1 != version)
            ythrow yexception() << "Wrong version: " << version;
    }

    TResult GetGeo(const TString& s) const {
        TString url = TString{CutHttpPrefix(s)};
        url.to_lower();
        const TString host = TString{GetHost(url)};
        const TString domain = TString{GetDomain(host)};
        const TString tld = TString{GetZone(host)};

        i32 region;
        if (RegTlds.Get(tld.data(), &region))
            return TResult(region, "tld", tld);

        if (RegDomains.Get(domain.data(), &region))
            return TResult(region, "regdomain", domain);

        TVector<TString> hostParts;
        Split(host, ".", hostParts);

        {
            if (hostParts.size() >= 3) {
                const TString& thirdLevel = hostParts[hostParts.size() - 3];
                if (RegCodes.Get(thirdLevel.data(), &region))
                    return TResult(region, "thirdlevel-code", thirdLevel);
                i32 index;
                if (GeoTrie.Get(thirdLevel.data(), &index))
                    if (Flags.At(index) & 8)
                        return TResult(Geos.At(index), "thirdlevel-substr", thirdLevel);
            }
        }

        {
            TVector<TString> parts;
            Split(url, "/", parts);
            if (parts.size() >= 2) {
                i32 index;
                if (GeoTrie.Get(parts[1].data(), &index))
                    if (Flags.At(index) & 4)
                        return TResult(Geos.At(index), "secondslash", parts[1]);
            }
        }

        static const ui32 INVALID_VALUE = (ui32)-1;
        {
            TDefaultMappedAhoCorasick::TSearchResult res;
            if (hostParts.size() >= 2) {
                const TString domain2 = hostParts[hostParts.size() - 2];
                Searcher.AhoSearch(domain2, &res);
                ui32 best = INVALID_VALUE;
                size_t bestLen = 0;
                TString bestSubstr;
                for (size_t i = 0; i < res.size(); ++i) {
                    const i32 index = res[i].second;
                    const size_t len = Substrs.GetLength(index) - 1;
                    Y_ASSERT(res[i].first + 1 >= len);
                    const size_t begin = res[i].first + 1 - len;
                    // Cerr << begin << "\t" << domain2 << "\t" << domain2.substr(begin, len) << "\t" << Substrs.Get(region) << Endl;
                    if ((begin == 0) || (begin + len == domain2.size())) {
                        if (len > bestLen) {
                            best = index;
                            bestLen = len;
                            bestSubstr = Substrs.Get(index);
                        }
                    }
                }
                if (INVALID_VALUE != best) {
                    if (Flags.At(best) & 2)
                        return TResult(Geos.At(best), "prefixsuffix", bestSubstr);
                }
            }
        }

        TDefaultMappedAhoCorasick::TSearchResult res;
        Searcher.AhoSearch(s, &res);
        ui32 best = INVALID_VALUE;
        size_t bestLen = 0;
        TString bestSubstr;
        for (size_t i = 0; i < res.size(); ++i) {
            const i32 index = res[i].second;
            const size_t len = Substrs.GetLength(index);
            if ((len > bestLen) && (Flags.At(index) & 1)) {
                best = index;
                bestLen = len;
                bestSubstr = Substrs.Get(index);
            }
        }

        if (INVALID_VALUE == best)
            return TResult(-1, "", "");
        else
            return TResult(Geos.At(best), "substr", bestSubstr);
    }
};
