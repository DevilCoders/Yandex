#include "seinfo.h"
#include "enums_impl.h"
#include "regexps_impl.h"
#include "regexps.h"

#include <kernel/search_query/search_query.h>

#include <library/cpp/charset/recyr.hh>
#include <library/cpp/containers/dictionary/dictionary.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/openssl/holders/evp.h>
#include <library/cpp/regex/libregex/regexstr.h>
#include <library/cpp/regex/pire/pire.h>
#include <library/cpp/regex/pire/pcre2pire.h>
#include <library/cpp/regex/pcre/regexp.h>
#include <library/cpp/xml/document/xml-document.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <contrib/libs/openssl/include/openssl/evp.h>
#include <contrib/libs/openssl/include/openssl/hmac.h>

#include <util/charset/utf8.h>
#include <util/generic/singleton.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/hex.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/system/mutex.h>
#include <util/string/split.h>


// NOTE: Please don't laugh. Refactoring is in progress.
bool USE_NEW_WAYS = false;

namespace NSe
{
    //==== Dictionaries ===========================================================================

    const TStringIdDictionary<ESearchEngine> SearchEngineDict(GetSearchEngineNames());
    const TStringIdDictionary<ESearchType> SearchTypeDict(GetSearchTypeNames());
    const TStringSparseIdDictionary<ESearchFlags> SearchFlagsDict(GetSearchFlagNames());
    const TStringIdDictionary<EPlatform> PlatformsDict(GetPlatformNames());

    const TStringIdDictionary<ESearchEngine>& GetSearchEngineDict()
    {
        return SearchEngineDict;
    }

    const TStringIdDictionary<ESearchType>& GetSearchTypeDict()
    {
        return SearchTypeDict;
    }

    const TStringSparseIdDictionary<ESearchFlags>& GetSearchFlagsDict()
    {
        return SearchFlagsDict;
    }

    const TStringIdDictionary<EPlatform>& GetPlatformsDict()
    {
        return PlatformsDict;
    }

    //==== Regexp building ========================================================================

    static void UniqNamedParams(TString& what, const TString& delim)
    {
        TVector<TString> tmp;
        StringSplitter(what.data()).SplitByString(Sprintf("<%s>", delim.data()).data()).Collect(&tmp);
        what = tmp[0];
        ui32 count = 0;
        for (int i = 1; i < tmp.ysize(); ++i) {
            what += "<f" + ToString(++count) + "_" + delim + ">" + tmp[i];
        }
    }

    static TString BuildCommonSearchRegexp(const TSquareRegexRegion& sqdata)
    {
        TString fullRegexp = "^(?:https?://)?(?:";
        bool first = true;
        for (const TArrayRef<const TSESimpleRegexp>& data: sqdata)
            for (ui32 i = 0; i < data.size(); ++i) {
                if (first) {
                    first = false;
                } else {
                    fullRegexp += "|";
                }
                fullRegexp += "(?:" + TString{data[i].Regexp};
                {
                    if (!data[i].CgiParam.empty()) {
                        const TString cgiRe = TString{data[i].CgiParam} + "\\=(?P<query>[^#&]*)";
                        if (data[i].CgiMagic.empty()) {
                            fullRegexp += "(?:.*[?;&#]" + cgiRe + ")?";
                        } else {
                            fullRegexp += "[^?#&;]*(?:[?#&;](?:(?:" + cgiRe + ")|(?:" + TString{data[i].CgiMagic} + ")|[^#&]*))*$";
                        }
                    }
                }
                fullRegexp += ")";
            }
        fullRegexp += ")";

        // TODO optimize here with aho corasick
        UniqNamedParams(fullRegexp, "engine");
        UniqNamedParams(fullRegexp, "query");
        UniqNamedParams(fullRegexp, "low_query");
        UniqNamedParams(fullRegexp, "platform");
        UniqNamedParams(fullRegexp, "pnum");
        UniqNamedParams(fullRegexp, "pstart");
        UniqNamedParams(fullRegexp, "psize");
        UniqNamedParams(fullRegexp, "encrypted_query");
        for (size_t i = 0; i < SearchTypeDict.Size(); ++i) {
            UniqNamedParams(
                fullRegexp,
                TString{SearchTypeDict.Id2Name(ESearchType(i))}
            );
        }
        for (size_t i = 0; i < SearchFlagsDict.Size(); ++i) {
            UniqNamedParams(
                fullRegexp,
                TString{SearchFlagsDict.Id2Name(ESearchFlags(1 << i))}
            );
        }

        return fullRegexp;
    }

    static TString BuildCommonSearchFastRegexp(const TSquareRegexRegion& sqdata)
    {
        TString fullRegexp = "^(?:https?://)?(?:";
        bool first = true;
        for (const TArrayRef<const TSESimpleRegexp>& data: sqdata)
            for (ui32 i = 0; i < data.size(); ++i) {
                if (first) {
                    first = false;
                } else {
                    fullRegexp += "|";
                }
                fullRegexp += "(?:" + TString{data[i].Regexp} + ")";
            }
        fullRegexp += ")";

        return Pcre2Pire(fullRegexp);
    }

    //==== ReMatch Callback =======================================================================

    inline void DecodeQuery(TString& query, bool toLower = false)
    {
        if (!query.empty()) {
            for (size_t i = 0; i < 2; ++i) {
                if (CheckAndFixQueryStringUTF8(query, query, toLower, true)) {
                    return;
                }
                try {
                    query = Recode(CODES_WIN, CODES_UTF8, query);
                } catch (yexception& /*e*/) {
                    break;
                }
            }
            ythrow yexception() << "Failed to decode query: " <<  query.data();
        }
        return;
    }

    TString DecryptQueryFromEtext(const TStringBuf& etext, const TEtextKeys& keys)
    {
        TStringBuf sig = etext;
        TStringBuf keyNo = sig.NextTok('.');
        TString encQuery = TString(sig.NextTok('.'));
        Y_ENSURE(sig && keyNo && encQuery, "malformed etext");

        TEtextKeys::TKey key = keys.GetKey(FromString<ui32>(keyNo));
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digestLen;

        Y_ENSURE(nullptr != HMAC(EVP_sha1(), key.SigKey.data(), key.SigKey.size(), (const unsigned char*)encQuery.data(), encQuery.size(), digest, &digestLen));
        Y_ENSURE(ToLowerUTF8(HexEncode(digest, digestLen)) == sig, "etext signature validation failed");

        // base64 padding is missing in encQuery
        while (encQuery.size() % 4 != 0) {
            encQuery.append('=');
        }

        TString buf = Base64Decode(encQuery);
        size_t encryptedLen = buf.size();
        Y_ENSURE(encryptedLen >= 16);

        encryptedLen -= 16; // last 16 bytes are iv

        TString res;
        int decryptedLen;
        int tailLen;

        NOpenSSL::TEvpCipherCtx ctx;

        if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, (unsigned char*)key.TextKey.data(), (unsigned char*)buf.data() + encryptedLen)) {
            ythrow yexception() << "etext decryption failed in EVP_DecryptInit_ex";
        }

        res.resize(encryptedLen + EVP_CIPHER_CTX_block_size(ctx));

        if (1 != EVP_DecryptUpdate(ctx, (unsigned char*)res.data(), &decryptedLen, (unsigned char*)buf.data(), encryptedLen)) {
            ythrow yexception() << "etext decryption failed in EVP_DecryptUpdate";
        }

        if (1 != EVP_DecryptFinal_ex(ctx, (unsigned char*)res.data() + decryptedLen, &tailLen)) {
            ythrow yexception() << "etext decryption failed in EVP_DecryptFinal_ex";
        }

        decryptedLen += tailLen;

        Y_ENSURE(decryptedLen > 0, "decrypted etext is empty");
        res.resize(decryptedLen);

        Y_ENSURE(res.StartsWith("text="), "malformed decrypted etext");

        TCgiParameters params(res);
        return params.Get("text");
    }

    class TSeInfoCallback : public TNonCopyable {
    private:
        NSe::TInfo& Info;
        bool GetQuery;
        bool ToLower;
        bool QueryFound;
        TEtextKeys Keys;
    public:
        TSeInfoCallback(NSe::TInfo& info, bool getQuery, bool toLower, const TEtextKeys& keys)
            : Info(info)
            , GetQuery(getQuery)
            , ToLower(toLower)
            , QueryFound(false)
            , Keys(keys)
        {
            info.Clear();
        }

        bool HasQuery() const
        {
            return QueryFound;
        }

        void operator()(const char* fieldRaw, const char* str, ui32 len)
        {
            if (Y_UNLIKELY(!fieldRaw)) {
                return;
            }
            TStringBuf field(fieldRaw);
            const size_t pos = field.find('_');
            if (Y_UNLIKELY(pos == TString::npos)) {
                ythrow yexception() << "Unknown kernel/seinfo regexp field: " << field;
            }
            field.Skip(pos + 1);

            if (TStringBuf("engine") == field) {
                // parsing search engine
                TString value = TString(str, 0, len);
                value.to_lower();
                Info.Name = SearchEngineDict[value];

                return;
            }

            if (TStringBuf("platform") == field) {
                TString value = TString(str, 0, len);
                value.to_lower();
                Info.Platform = PlatformsDict[value];
                return;
            }

            if (TStringBuf("query") == field) {
                // parsing search query
                if (!Info.BadQuery) {
                    QueryFound = true;
                    if (GetQuery) {
                        Info.Query.assign(str, 0, len);
                        try {
                            CGIUnescape(Info.Query);
                            DecodeQuery(Info.Query, ToLower);
                        } catch (...) {
                            Info.Query = "";
                            Info.BadQuery = true;
                        }
                    }
                }
                return;
            }

            if (TStringBuf("low_query") == field) {
                // parsing search query
                if (!Info.BadQuery && Info.Query.empty()) {
                    QueryFound = true;
                    if (GetQuery && Info.Query.empty()) {
                        Info.Query.assign(str, 0, len);
                        try {
                            CGIUnescape(Info.Query);
                            DecodeQuery(Info.Query, ToLower);
                        } catch (...) {
                            Info.Query = "";
                        }
                    }
                }
                return;
            }

            if (TStringBuf("encrypted_query") == field) {
                if (!Info.BadQuery && Info.Query.empty()) {
                    const TString etext(str, len);
                    TString decryptedQuery;
                    try {
                        decryptedQuery = DecryptQueryFromEtext(etext, Keys);
                    } catch (...) {}

                    if (decryptedQuery) {
                        QueryFound = true;
                        Info.Query = decryptedQuery;
                        try {
                            DecodeQuery(Info.Query, ToLower);
                        } catch (...) {
                            Info.Query = "";
                        }
                    }
                }
                return;
            }

            // search type
            {
                const bool stOk = SearchTypeDict.TryName2Id(field.data(), Info.Type);
                if (stOk) {
                    return;
                }
            }

            // search flags
            {
                NSe::ESearchFlags newFlags;

                const bool fOk = SearchFlagsDict.TryName2Id(field.data(), newFlags);
                if (fOk) {
                    Info.Flags = (NSe::ESearchFlags)(Info.Flags | newFlags);
                    return;
                }
            }

            if (TStringBuf("pstart") == field) {
                if (len != 0) {
                    ui32 val;
                    if (TryFromString<ui32>(str, len, val)) {
                        Info.SetPageStartRaw(val);
                    } else {
                        Info.SetPageStartRaw(TMaybe<ui32>());
                    }
                }
                return;
            }

            if (TStringBuf("psize") == field) {
                if (len != 0) {
                    ui32 val;
                    if (TryFromString<ui32>(str, len, val)) {
                        Info.SetPageSizeRaw(val);
                    } else {
                        Info.SetPageSizeRaw(TMaybe<ui32>());
                    }
                }
                return;
            }

            if (TStringBuf("pnum") == field) {
                ui32 val;
                if (TryFromString<ui32>(str, len, val)) {
                    Info.SetPageNumRaw(val);
                } else {
                    Info.SetPageNumRaw(TMaybe<ui32>());
                }
                return;
            }

            ythrow yexception() << "Unexpected seinfo-regex field: '" << field << "'!" << Endl;
        }
    };

    //==== TEtextKeys implementations =============================================================
    TEtextKeys::TEtextKeys(const TString& keysPath)
    {
        if (keysPath.EndsWith(".json")) {
            NJson::TJsonValue json;
            TFileInput fileInput(keysPath);
            if(!NJson::ReadJsonTree(&fileInput, &json)) {
                ythrow yexception() << "Failed parsing keys json file";
            }

            for (const auto& key : json.GetArray()) {
                const auto& keyMap = key.GetMap();
                AddKey(FromString<ui32>(keyMap.at("id").GetString()), { keyMap.at("sig").GetString(), HexDecode(keyMap.at("val").GetString())});
            }
        } else {
            NXml::TDocument xmlDoc(keysPath);
            NXml::TConstNode root(xmlDoc.Root());

            NXml::TConstNodes keyNodes = root.Nodes("/keys/key");
            for (const auto& keyNode : keyNodes) {
                AddKey(keyNode.Attr<ui32>("id"), { keyNode.Attr<TString>("sig"), HexDecode(keyNode.Value<TString>()) });
            }
        }
    }

    const TEtextKeys::TKey& TEtextKeys::GetKey(ui32 keyNo) const
    {
        return Keys.at(keyNo);
    }

    void TEtextKeys::AddKey(ui32 keyNo, const TEtextKeys::TKey& key)
    {
        Keys[keyNo] = key;
    }

    //==== TInfo implementations ==================================================================

    bool TInfo::IsWeb() const
    {
        return Type == ST_WEB;
    }

    bool TInfo::IsImage() const
    {
        return Type == ST_IMAGES;
    }

    bool TInfo::IsVideo() const
    {
        return Type == ST_VIDEO;
    }

    bool TInfo::IsLocal() const
    {
        return (Name != SE_UNKNOWN) && (Flags & SF_LOCAL);
    }

    bool TInfo::IsSearch() const
    {
        return (Name != SE_UNKNOWN) && (Flags & SF_SEARCH) && !(Flags & SF_FAKE_SEARCH);
    }

    bool TInfo::IsEmail() const
    {
        return (Name != SE_UNKNOWN) && (Flags & SF_MAIL);
    }

    bool TInfo::IsSocial() const
    {
        return (Name != SE_UNKNOWN) && (Flags & SF_SOCIAL);
    }

    bool TInfo::IsSerpAdv() const
    {
        return (Name != SE_UNKNOWN) && (Type == ST_ADV_SERP);
    }

    bool TInfo::IsWebAdv() const
    {
        return (Name != SE_UNKNOWN) && (Type == ST_ADV_WEB);
    }

    bool TInfo::IsAdvert() const
    {
        return (IsSerpAdv() || IsWebAdv());
    }

    TMaybe<ui32> TInfo::GetPageSize() const
    {
        if (PageSizeRaw.Defined()) {
            return PageSizeRaw;
        } else if (Name == SE_YANDEX || Name == SE_GOOGLE) {
            return 10;
        } else {
            return TMaybe<ui32>();
        }
    }

    TMaybe<ui32> TInfo::GetPageNum() const
    {
        if (PageNumRaw.Defined()) {
            return PageNumRaw;
        } else if (PageStartRaw.Defined()) {
            if (PageSizeRaw.Defined() && PageSizeRaw.GetRef() > 0) {
                return PageStartRaw.GetRef() / PageSizeRaw.GetRef();
            } else if (Name == SE_YANDEX || Name == SE_GOOGLE) {
                return PageStartRaw.GetRef() / 10;
            }
        } else if (Name == SE_YANDEX || Name == SE_GOOGLE) {
            return 0;
        }

        return TMaybe<ui32>();
    }

    TMaybe<ui32> TInfo::GetPageStart() const
    {
        if (PageStartRaw.Defined()) {
            return PageStartRaw;
        } else {
            const TMaybe<ui32> rPageNum  = GetPageNum();
            const TMaybe<ui32> rPageSize = GetPageSize();
            if (rPageNum.Defined() && rPageSize.Defined()) {
                return rPageSize.GetRef() * rPageSize.GetRef();
            }
        }

        return TMaybe<ui32>();
    }

    void TInfo::SetPageSizeRaw(const TMaybe<ui32>& val)
    {
        PageSizeRaw = val;
    }

    void TInfo::SetPageNumRaw(const TMaybe<ui32>& val)
    {
        PageNumRaw = val;
    }

    void TInfo::SetPageStartRaw(const TMaybe<ui32>& val)
    {
        PageStartRaw = val;
    }

    TInfo::TInfo(ESearchEngine name, ESearchType type, const TString& query, ESearchFlags flags, EPlatform platform, bool badQuery)
        : Name(name)
        , Type(type)
        , Query(query)
        , Flags(flags)
        , Platform(platform)
        , BadQuery(badQuery)
    {
    }

    void TInfo::Clear()
    {
        *this = TInfo();
    }

    //==== TSeInfoExtractior ======================================================================

    class TSeInfoExtractor {
    private:
        TMutex Mutex;
        TRegexNamedParser<TRegexStringParser> Parser;
        NPire::TSimpleScanner FastScanner;

    public:
        static constexpr size_t MAX_URL_LENGTH = 4096;
        static constexpr size_t MAX_RECURSION = 256;

        TSeInfoExtractor(const TString& regexpStr, const TString& fastRegexpStr)
            : Parser(regexpStr.c_str(), PCRE_EXTENDED | PCRE_CASELESS)
        {
            Parser.SetRecursionLimit(MAX_RECURSION);
            FastScanner = NPire::TLexer(fastRegexpStr)
                .AddFeature(NPire::NFeatures::CaseInsensitive())
                .Parse()
                .Surround()
                .Compile<NPire::TSimpleScanner>();
        }

        bool ParseUrl(const TString& url, TInfo& info, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys);

        bool AtomicParseUrl(const TString& url, TInfo& info, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys);

    private:
        bool ParseUrlRaw(const TString& url, TInfo& info, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys);

        inline bool FastMatchHost(const TString& url) const
        {
            return NPire::Runner(FastScanner).Begin().Run(url.begin(), url.end()).End();
        }

        bool TryNewWays(const TString& url, TInfo& info, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys) const
        {
            info.Clear();

            if (Y_UNLIKELY(url.size() > MAX_URL_LENGTH)) {
                return false;
            }

            TRegexpResult r;
            bool ret = SERecognizeUrl(r, url);
            if (!ret)
                return false;

            info.Flags = r.Flags;
            info.Type = r.Type;
            if (info.Type == ST_UNKNOWN)
                info.Type = ST_WEB;

            TStringBuf s;
            bool queryFound = false;
            bool mainQueryFound = false;
            s = r.GetStringBetween(TRegexpResult::QUERY_BEGIN, TRegexpResult::QUERY_END);
            if (s.IsInited()) {
                mainQueryFound = true;
            } else {
                s = r.GetStringBetween(TRegexpResult::LOW_QUERY_BEGIN, TRegexpResult::LOW_QUERY_END);
            }

            if (s.IsInited()) {
                queryFound = true;
                if (getQuery) {
                    info.Query.assign(s);
                    try {
                        CGIUnescape(info.Query);
                        DecodeQuery(info.Query, toLower);
                    } catch (...) {
                        info.Query.clear();
                        if (mainQueryFound) {
                            info.BadQuery = true;
                        }
                    }
                }

            }

            TStringBuf etext = r.GetStringBetween(TRegexpResult::ENCRYPTED_QUERY_BEGIN, TRegexpResult::ENCRYPTED_QUERY_END);
            if (etext.IsInited() && !info.BadQuery && info.Query.empty()) {
                TString decryptedQuery;
                try {
                    decryptedQuery = DecryptQueryFromEtext(etext, keys);
                } catch (...) {}
                if (decryptedQuery) {
                    queryFound = true;
                    info.Query = decryptedQuery;
                    try {
                        DecodeQuery(info.Query, toLower);
                    } catch (...) {
                        info.Query.clear();
                        info.BadQuery = true;
                    }
                }
            }


            if (needQuery && !info.BadQuery && !queryFound) {
                if (!(info.Flags & SF_REDIRECT) && (info.Flags & SF_SEARCH)) {
                    // no query => no search
                    info.Clear();
                    return false;
                } else {
                    // redirects and nosearch urls may have no query
                }
            }

            s = r.GetStringBetween(TRegexpResult::ENGINE_BEGIN, TRegexpResult::ENGINE_END);
            if (s) {
                TString value = TString{s};
                value.to_lower();
                info.Name = SearchEngineDict[value];
            }

            s = r.GetStringBetween(TRegexpResult::PLATFORM_BEGIN, TRegexpResult::PLATFORM_END);
            if (s) {
                TString value = TString{s};
                value.to_lower();
                info.Platform = PlatformsDict[value];
            }

            s = r.GetStringBetween(TRegexpResult::PSTART_BEGIN, TRegexpResult::PSTART_END);
            if (s) {
                ui32 val;
                if (TryFromString<ui32>(s, val)) {
                    info.SetPageStartRaw(val);
                } else {
                    info.SetPageStartRaw(TMaybe<ui32>());
                }
            }

            s = r.GetStringBetween(TRegexpResult::PSIZE_BEGIN, TRegexpResult::PSIZE_END);
            if (s) {
                ui32 val;
                if (TryFromString<ui32>(s, val)) {
                    info.SetPageSizeRaw(val);
                } else {
                    info.SetPageSizeRaw(TMaybe<ui32>());
                }
            }

            s = r.GetStringBetween(TRegexpResult::PNUM_BEGIN, TRegexpResult::PNUM_END);
            if (s) {
                ui32 val;
                if (TryFromString<ui32>(s, val)) {
                    info.SetPageNumRaw(val);
                } else {
                    info.SetPageNumRaw(TMaybe<ui32>());
                }
            }

            return true;
        }

    };

    bool TSeInfoExtractor::ParseUrl(const TString& url, TInfo& info, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys) {
        try {
            return ParseUrlRaw(url, info, getQuery, toLower, needQuery, keys);
        } catch (...) {
            info = {}; // reset, because result may be inconsistent
            return false;
        }
    }

    bool TSeInfoExtractor::ParseUrlRaw(const TString& url, TInfo& info, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys)
    {
        if (USE_NEW_WAYS) {
            if (TryNewWays(url, info, getQuery, toLower, needQuery, keys))
                return true;
        }

        info.Clear();

        if (Y_UNLIKELY(url.size() > MAX_URL_LENGTH)) {
            return false;
        }

        if (!FastMatchHost(url)) {
            return false;
        }

        TSeInfoCallback callback(info, getQuery, toLower, keys);
        const bool isSe = Parser.Generate(callback, url.data(), url.size());
        if (!isSe) {
            info.Clear();
            return false;
        }

        if (needQuery && !info.BadQuery && !callback.HasQuery()) {
            if (!(info.Flags & SF_REDIRECT) && (info.Flags & SF_SEARCH)) {
                // no query => no search
                info.Clear();
                return false;
            } else {
                // redirects and nosearch urls may have no query
            }
        }
        if (isSe && info.Type == ST_UNKNOWN) {
            info.Type = ST_WEB;
        }
        return true;
    }

    bool TSeInfoExtractor::AtomicParseUrl(const TString& url, TInfo& info, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys)
    {
        TGuard<TMutex> lock(Mutex);
        Y_UNUSED(lock);
        return ParseUrl(url, info, getQuery, toLower, needQuery, keys);
    }


    //==== TDefaultSeInfoExtractor ======================================================================

    class TDefaultSeInfoExtractor {
    public:
        TSeInfoExtractor Extractor;
        TDefaultSeInfoExtractor()
            : Extractor(
                    BuildCommonSearchRegexp(
                        USE_NEW_WAYS
                            ? GetNotRagelizedRegexps()
                            : GetDefaultSearchRegexps()
                    ),
                    BuildCommonSearchFastRegexp(
                        USE_NEW_WAYS
                            ? GetNotRagelizedRegexps()
                            : GetDefaultSearchRegexps()
                    )
                )
        {}
    };

    //==== Global methods realizations ============================================================

    template<class TSeInfoParams>
    static inline TSeInfoExtractor& GetExtractor() {
        return Singleton<TSeInfoParams>()->Extractor;
    }

    TInfo TInfo::Empty;

    bool operator==(const TInfo& x, const TInfo& y)
    {
#define SE_CHECK_FIELDS_EQUAL(Field) \
        if (x.Field != y.Field) { \
            return false; \
        }

        SE_CHECK_FIELDS_EQUAL(Name);
        SE_CHECK_FIELDS_EQUAL(Type);
        SE_CHECK_FIELDS_EQUAL(Query);
        SE_CHECK_FIELDS_EQUAL(Flags);
        SE_CHECK_FIELDS_EQUAL(Platform);
        SE_CHECK_FIELDS_EQUAL(GetPageSize());
        SE_CHECK_FIELDS_EQUAL(GetPageNum());
        SE_CHECK_FIELDS_EQUAL(GetPageStart());
        SE_CHECK_FIELDS_EQUAL(BadQuery);

        return true;

#undef SE_CHECK_FIELDS_EQUAL
    }

    void GetSeInfo(const TString& url, TInfo& info, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys)
    {
        GetExtractor<TDefaultSeInfoExtractor>().ParseUrl(url, info, getQuery, toLower, needQuery, keys);
        // Ugly hack for 'doubleclick.net' to be Google.
        if (info.Name == SE_IMPL_DOUBLECLICK) {
            info.Name = SE_GOOGLE;
        }
    }

    TInfo GetSeInfo(const TString& url, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys)
    {
        TInfo result;
        GetSeInfo(url, result, getQuery, toLower, needQuery, keys);
        return result;
    }

    void AtomicGetSeInfo(const TString& url, TInfo& info, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys)
    {
        GetExtractor<TDefaultSeInfoExtractor>().AtomicParseUrl(url, info, getQuery, toLower, needQuery, keys);
    }

    TInfo AtomicGetSeInfo(const TString& url, bool getQuery, bool toLower, bool needQuery, const TEtextKeys& keys)
    {
        TInfo result;
        AtomicGetSeInfo(url, result, getQuery, toLower, needQuery, keys);
        return result;
    }

    bool IsSe(const TString& url)
    {
        return GetSeInfo(url, false, false, true).IsSearch();
    }

    bool AtomicIsSe(const TString& url)
    {
        return AtomicGetSeInfo(url, false, false, true).IsSearch();
    }
}; // namespace NSe
