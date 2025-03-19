#include "qd_docitems.h"
#include "qd_cgi_utils.h"

#include <kernel/querydata/idl/scheme/querydata_request.sc.h>

#include <kernel/urlnorm/normalize.h>
#include <kernel/hosts/owner/owner.h>
#include <kernel/urlid/url2docid.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>

#include <util/generic/algorithm.h>
#include <util/string/builder.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/util.h>

namespace NQueryData {

    void TDocItemsRec::DoAddUrlRaw(TStringBufPairs& urls, TStringBuf url, TStringBuf categ) {
        if (url) {
            urls.emplace_back(Own(url), Own(categ));
        }
    }

    void TDocItemsRec::DoAddUrlWithNormalization(TStringBufPairs& urls, TStringBuf url) {
        if (url) {
            TStringBuf weakUrl = Pool.AppendCString(TStringBuf(DoWeakUrlNormalization(url)));
            TStringBuf owner = TOwnerCanonizer::WithSerpCategOwners().GetUrlOwner(CutSchemePrefix(weakUrl));
            urls.emplace_back(weakUrl, owner); // owner - подстрока weakUrl, которым мы уже владеем
            if (url != weakUrl) {
                urls.emplace_back(Own(url), owner);
            }
        }
    }

    void TDocItemsRec::DoAddUrls(TStringBufPairs& urls, const NSc::TValue& rawUrls) {
        if (rawUrls.IsArray()) {
            for (const auto& url : rawUrls.GetArray()) {
                DoAddUrlWithNormalization(urls, url);
            }
        } else if (rawUrls.IsDict()) {
            const auto& dict = rawUrls.GetDict();
            for (const auto& url : rawUrls.DictKeys()) {
                DoAddUrlRaw(urls, url, dict.at(url));
            }
        }
    }

    void TDocItemsRec::FromJson(const NSc::TValue& val) {
        TDocItemsConst<TSchemeTraits> items(&val);

        AddDocIdsAny(items.DocIds().GetRawValue()->DictKeys(), DIM_BANS);
        AddDocIdsAny(items.SnipDocIds().GetRawValue()->DictKeys(), DIM_SNIPS);

        AddCategsAny(items.Categs().GetRawValue()->DictKeys(), DIM_BANS);
        AddCategsAny(items.SnipCategs().GetRawValue()->DictKeys(), DIM_SNIPS);

        AddUrlsAny(*items.Urls().GetRawValue(), DIM_BANS);
        AddUrlsAny(*items.SnipUrls().GetRawValue(), DIM_SNIPS);
    }

    template <typename TDict>
    static void DoCopyToDict(TDict to, const TStringBufs& from) {
        for (const auto& v : from) {
            if (v) {
                to[v] = 1;
            }
        }
    }

    template <class TDict>
    static void DoCopyToDict(TDict to, const TStringBufPairs& from) {
        for (const auto& v : from) {
            if (v.first) {
                to[v.first] = v.second;
            }
        }
    }

    void TDocItemsRec::ToJson(NSc::TValue& val) const {
        TDocItems<TSchemeTraits> items(&val);

        if (DocIds()) {
            DoCopyToDict(items.DocIds(), DocIds());
        }

        if (SnipDocIds()) {
            DoCopyToDict(items.SnipDocIds(), SnipDocIds());
        }

        if (Categs()) {
            DoCopyToDict(items.Categs(), Categs());
        }

        if (SnipCategs()) {
            DoCopyToDict(items.SnipCategs(), SnipCategs());
        }

        if (Urls()) {
            DoCopyToDict(items.Urls(), Urls());
        }

        if (SnipUrls()) {
            DoCopyToDict(items.SnipUrls(), SnipUrls());
        }
    }

    NSc::TValue TDocItemsRec::ToJson() const {
        NSc::TValue res;
        ToJson(res);
        return res;
    }

    template <class TCollect>
    static size_t DoGetSize(const TCollect& bans, const TCollect& snips, EDocItemMode mode) {
        size_t res = 0;
        if (mode & DIM_BANS) {
            res += bans.size();
        }

        if (mode & DIM_SNIPS) {
            res += snips.size();
        }

        return res;
    }

    static void DoAllDocIds(TStringBufs& result, const TStringBufs& docIds, const TStringBufPairs& urls, TMemoryPool& pool, TString& buf) {
        for (auto docId : docIds) {
            if (Y_LIKELY(docId)) {
                result.emplace_back(docId);
            }
        }

        for (const auto& url : urls) {
            if (Y_LIKELY(url.first)) {
                result.push_back(pool.AppendString<char>(Url2DocId(url.first, buf)));
            }
        }
    }

    TStringBufs TDocItemsRec::AllDocIdsNoCopy(TMemoryPool& pool, EDocItemMode mode) const {
        TStringBufs docIds;
        docIds.reserve(DoGetSize(DocIds_, SnipDocIds_, mode) + DoGetSize(Urls_, SnipUrls_, mode));

        TString buf;

        if (mode & DIM_BANS) {
            DoAllDocIds(docIds, DocIds_, Urls_, pool, buf);
        }

        if (mode & DIM_SNIPS) {
            DoAllDocIds(docIds, SnipDocIds_, SnipUrls_, pool, buf);
        }

        return docIds;
    }

    static void DoAllCategUrls(TStringBufs& result, const TStringBufPairs& urls, TMemoryPool& pool, TString& buf) {
        for (const auto& url : urls) {
            if (Y_LIKELY(url.first)) {
                result.emplace_back(pool.AppendString<char>(buf.assign(url.second).append(' ').append(url.first)));
            }
        }
    }

    TStringBufs TDocItemsRec::AllCategUrlsNoCopy(TMemoryPool& pool, EDocItemMode mode) const {
        TStringBufs urls;
        urls.reserve(DoGetSize(Urls_, SnipUrls_, mode));

        TString buf;

        if (mode & DIM_BANS) {
            DoAllCategUrls(urls, Urls_, pool, buf);
        }

        if (mode & DIM_SNIPS) {
            DoAllCategUrls(urls, SnipUrls_, pool, buf);
        }

        return urls;
    }

    TStringBufs TDocItemsRec::AllUrlsNoCopy(EDocItemMode mode) const {
        TStringBufs result;
        result.reserve(DoGetSize(Urls_, SnipUrls_, mode));

        if (mode & DIM_BANS) {
            for (const auto& url : Urls_) {
                if (Y_LIKELY(url.first)) {
                    result.emplace_back(url.first);
                }
            }
        }

        if (mode & DIM_SNIPS) {
            for (const auto& url : SnipUrls_) {
                if (Y_LIKELY(url.first)) {
                    result.emplace_back(url.first);
                }
            }
        }

        return result;
    }

    static void DoAllCategs(TStringBufs& result, const TStringBufs& categs, const TStringBufPairs& urls) {
        for (auto categ : categs) {
            if (Y_LIKELY(categ)) {
                result.emplace_back(categ);
            }
        }

        for (const auto& url : urls) {
            if (Y_LIKELY(url.second)) {
                result.emplace_back(url.second);
            }
        }
    }

    TStringBufs TDocItemsRec::AllCategsNoCopy(EDocItemMode mode) const {
        TStringBufs categs;
        categs.reserve(DoGetSize(Categs_, SnipCategs_, mode) + DoGetSize(Urls_, SnipUrls_, mode));

        if (mode & DIM_BANS) {
            DoAllCategs(categs, Categs_, Urls_);
        }

        if (mode & DIM_SNIPS) {
            DoAllCategs(categs, SnipCategs_, SnipUrls_);
        }

        return categs;
    }

    TVector<TString> PairsToStrings(const TStringBufPairs& t) {
        TVector<TString> result;
        result.reserve(t.size());
        for (const auto& k : t) {
            result.push_back(NEscJ::EscapeJ<true>(k.first) + ":" + NEscJ::EscapeJ<true>(k.second));
        }
        return result;
    }

    TString SplitDictStrings(const TVector<TString>& dictItems, ui32 part, ui32 parts) {
        return (TStringBuilder() << "{" << VectorToStringSplit(dictItems, ',', part, parts) << "}");
    }
}
