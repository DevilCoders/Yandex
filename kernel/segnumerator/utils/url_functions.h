#pragma once

#include <kernel/hosts/owner/owner.h>
#include <kernel/urlnorm/urlnorm.h>

#include <util/charset/utf8.h>
#include <library/cpp/charset/wide.h>
#include <util/digest/murmur.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/digest/fnv.h>
#include <util/system/maxlen.h>
#include <util/string/cast.h>

#include <contrib/libs/libidn/lib/idna.h>

namespace NSegm {

enum ELinkType {
    LT_EXTERNAL_LINK = 0,
    LT_LOCAL_LINK = 1,
    LT_FRAGMENT_LINK = 2 | LT_LOCAL_LINK,
    LT_SELF_LINK = 4 | LT_FRAGMENT_LINK,
};

struct TUrlInfo {
    struct TParseRes {
        THttpURL Url;
        TString Owner;

        ui64 UrlId = -1;
        THttpURL::TParsedState State = THttpURL::ParsedEmpty;
    } ParseRes;

    TString RawUrl;

    TUtf16String Host;
    TUtf16String Path;
    TUtf16String Dirs;
    TUtf16String File;
    TUtf16String Query;

    const TOwnerCanonizer* Canonizer;

    TUrlInfo(const TOwnerCanonizer* c = nullptr)
        : Canonizer(c)
    {}

    TUrlInfo(TStringBuf rawurl, const TOwnerCanonizer* c = nullptr)
        : Canonizer(c)
    {
        SetUrl(rawurl);
    }

    void SetCanonizer(const TOwnerCanonizer* c) {
        Canonizer = c;
    }

    void SetUrl(TStringBuf url) {
        Clear();
        RawUrl.assign(url.data(), url.size());

        ParseRes = ProcessUrl(url);

        {
            TTempBuf tmp(URL_MAX + 1);

            ParseRes.Url.Print(tmp.Data(), tmp.Size(), THttpURL::FlagHost);
            DecodeHost(Host, tmp.Data());

            ParseRes.Url.Print(tmp.Data(), tmp.Size(), THttpURL::FlagPath);
            DecodeUrl(Path, tmp.Data());

            {
                TStringBuf p(tmp.Data(), strlen(tmp.Data()));

                while (p.EndsWith('/'))
                    p.Chop(1);

                TStringBuf dirs, file;
                p.SplitAt(p.rfind('/'), dirs, file);
                file.Skip(1);
                *(char*)(dirs.data() + dirs.size()) = 0; // it's safe
                DecodeUrl(Dirs, dirs.data());
                DecodeUrl(File, file.data());
            }

            ParseRes.Url.Print(tmp.Data(), tmp.Size(), THttpURL::FlagQuery);
            DecodeUrl(Query, tmp.Data());
        }
    }

    void Clear() {
        ParseRes = TParseRes();
        RawUrl.clear();
        Host.clear();
        Path.clear();
        Dirs.clear();
        File.clear();
        Query.clear();
    }

    ELinkType CheckLink(const TString& rawurl) const {
        ui32 hash;
        return CheckLink(rawurl, hash);
    }

    ELinkType CheckLink(const TString& rawurl, ui32& hosthash) const {
        TParseRes res;
        res = ProcessUrl(rawurl);

        if (THttpURL::ParsedOK == res.State || THttpURL::ParsedOpaque == res.State) {
            if (!ParseRes.Owner.EndsWith(res.Owner)) {
                hosthash = MurmurHash<ui32>(res.Owner.data(), res.Owner.size());
                return LT_EXTERNAL_LINK;
            }

            if (ParseRes.UrlId != res.UrlId)
                return LT_LOCAL_LINK;

            return rawurl == RawUrl ? LT_SELF_LINK : LT_FRAGMENT_LINK;
        } else {
            return LT_LOCAL_LINK;
        }
    }

    TStringBuf GetOwner(const char* host) const {
        return Canonizer ? Canonizer->GetUrlOwner(host)
                        : GetHostOwner(THashSet<const char*>(), host);
    }

    static void DecodeUrl(TUtf16String& res, const char* url, ECharset doccs = CODES_ASCII) {
        TTempBuf tmp(URL_MAX + 1);
        size_t len = CGIUnescape(tmp.Data(), url) - tmp.Data();
        if (IsUtf(tmp.Data(), len))
            UTF8ToWide(TStringBuf(tmp.Data(), len), res);
        else
            CharToWide(TStringBuf(tmp.Data(), len), res, doccs);
    }

    static void DecodeHost(TUtf16String& res, const char* host) {
        char* out = nullptr;
        if (IDNA_SUCCESS == idna_to_unicode_8z8z(host, &out, 0)) {
            UTF8ToWide<true>(out, res);
            free(out);
        } else {
            if (out)
                free(out);
            UTF8ToWide<true>(host, res);
        }
    }

private:
    TParseRes ProcessUrl(TStringBuf rawurl) const {
        TParseRes res;

        res.State = res.Url.ParseUri(rawurl, THttpURL::FeaturesAll);

        if (THttpURL::ParsedOK == res.State || THttpURL::ParsedOpaque == res.State) {
            TTempBuf tmp(URL_MAX + 1);

            res.Url.Print(tmp.Data(), tmp.Size(), THttpURL::FlagHost);
            res.Owner = ToString(GetOwner(tmp.Data()));

            res.Url.Print(tmp.Data(), tmp.Size(), THttpURL::FlagPath | THttpURL::FlagQuery);

            if (THttpURL::ParsedOK != res.State
                            || tmp.Data()[0] != '/'
                                            || UrlHashVal(res.UrlId, tmp.Data(), false) != 0) {
                res.UrlId = FnvHash<ui64>(tmp.Data(), strlen(tmp.Data()));
            }

            res.Url.Print(tmp.Data(), tmp.Size(), THttpURL::FlagScheme | THttpURL::FlagHostPort);
            res.UrlId = CombineHashes(FnvHash<ui64>(tmp.Data(), strlen(tmp.Data())), res.UrlId);
        }

        return res;
    }

};

}
