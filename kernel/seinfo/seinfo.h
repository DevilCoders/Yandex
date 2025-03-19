#pragma once

/// @file seinfo.h Contains functions to detect search engines and queries from urls.
/// @author Victor Ploshikhin aka vvp@
/// @author Mikhail Sharkray aka smikler@

#include "enums.h"

#include <util/system/guard.h>
#include <util/string/printf.h>
#include <util/string/vector.h>
#include <util/generic/string.h>
#include <util/generic/maybe.h>
#include <utility>
#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/containers/dictionary/dictionary.h>

namespace NSe
{
    /**
     * Search engine names.
     * @see definitions at the bottom of the file: SE_YANDEX, SE_GOOGLE, ...
     */
    enum ESearchEngine;
    const TStringIdDictionary<ESearchEngine>& GetSearchEngineDict();

    /**
     * Search engine types.
     * @see definitions at the bottom of the file: ST_WEB, ST_IMAGES, ST_VIDEO, ...
     */
    enum ESearchType;
    const TStringIdDictionary<ESearchType>& GetSearchTypeDict();

    /* Note: on update - don't forget to update SF_TOTAL_FLAGS_COUNT field too!
     *
     * SF_MOBILE   - search from mobile systems
     * SF_LOGINED  - user should be logined to gain this url
     * SF_LOCAL    - site search or url search
     * SF_PEOPLE   - searching people
     * SF_REDIRECT - i.e. click url wrapper, like google.com/url
     *
     * SEARCH_FLAGS_MAP macro definition is needed to ToCString function which used in
     * seinfo python bindings
     *
     * @see definitions at the bottom of the file: SF_MOBILE, SF_LOGINED, ...
     */
    enum ESearchFlags;
    const TStringSparseIdDictionary<ESearchFlags>& GetSearchFlagsDict();

    /*
     * @see definitions at the bottom of the file: P_ANDROID, P_IPHONE, P_IPAD, ...
     */
    enum EPlatform;
    const TStringIdDictionary<EPlatform>& GetPlatformsDict();

    /**
     * Checks that url belongs to known search engines.
     *
     * @param[in] url any URL.
     * @return true if url belongs to known search engines, otherwise - false.
     */
    bool IsSe(const TString& url);
    bool AtomicIsSe(const TString& url); // thread safe version

    /**
     * Contains information about url.
     */
    class TInfo {
    public:
        ESearchEngine Name;
        ESearchType   Type;
        TString        Query;
        ESearchFlags  Flags;
        EPlatform     Platform; // For yandex.mobile requests.
        bool          BadQuery;
    private:
        // NOTE: Parsing only for SE_YANDEX & SE_GOOGLE.
        TMaybe<ui32>  PageSizeRaw;  // number of results, if defined.
        TMaybe<ui32>  PageNumRaw;   // page number, if defined.
        TMaybe<ui32>  PageStartRaw; // page offset (in elements), if defined.
    public:
        TInfo(ESearchEngine = SE_UNKNOWN, ESearchType = ST_UNKNOWN, const TString& query = "", ESearchFlags = SF_NO_FLAG, EPlatform = P_UNKNOWN, bool badQuery = false);

        // Search.
        bool IsSearch() const;

        // Email
        bool IsEmail() const;

        // Social networks.
        bool IsSocial() const;

        // Advertisement.
        bool IsSerpAdv() const;
        bool IsWebAdv() const;
        bool IsAdvert() const;

        bool IsWeb() const;
        bool IsImage() const;
        bool IsVideo() const;
        bool IsLocal() const;

        // NOTE: works only for SE_YANDEX & SE_GOOGLE.
        TMaybe<ui32> GetPageSize() const;
        TMaybe<ui32> GetPageNum() const;
        TMaybe<ui32> GetPageStart() const;

        void SetPageSizeRaw(const TMaybe<ui32>& val);
        void SetPageNumRaw(const TMaybe<ui32>& val);
        void SetPageStartRaw(const TMaybe<ui32>& val);

        void Clear();

        static TInfo Empty;
    };

    bool operator==(const TInfo& x, const TInfo& y);

    inline
    bool IsEmpty(const TInfo& info)
    {
        return (info == TInfo::Empty);
    }

    /**
     * Keys to decrypt query in yandex referers
     *
     * @see https://wiki.yandex-team.ru/serp/projects/Shifrovanietekstazaprosa/
     */
    class TEtextKeys {
    public:
        struct TKey {
            TString SigKey;
            TString TextKey;
            SAVELOAD(SigKey, TextKey);
            Y_SAVELOAD_DEFINE(SigKey, TextKey);
        };

    private:
        THashMap<ui32, TKey> Keys;

    public:
        TEtextKeys() {}
        /**
         * Accepts either keys.json or keys.xml file
         * Use latest CLICKDAEMON_KEYS sandbox resource or
         * svn export svn+ssh://arcadia.yandex.ru/robots/trunk/clickdaemon-keys/keys.xml
         */
        explicit TEtextKeys(const TString& keysPath);
        SAVELOAD(Keys);
        Y_SAVELOAD_DEFINE(Keys);

        const TKey& GetKey(ui32 keyNo) const;
        void AddKey(ui32 keyNo, const TKey& key);
    };

    /**
     * Returns info about URL.
     *
     * @param[in] url any URL.
     * @return TInfo structure describe belonging URL to search engines.
     */
    TInfo GetSeInfo(const TString& url, bool getQuery = true, bool toLower = false, bool needQuery = true, const TEtextKeys& keys = TEtextKeys());
    void GetSeInfo(const TString& url, TInfo& info, bool getQuery = true, bool toLower = false, bool needQuery = true, const TEtextKeys& keys = TEtextKeys());
    // thread safe version
    TInfo AtomicGetSeInfo(const TString& url, bool getQuery = true, bool toLower = false, bool needQuery = true, const TEtextKeys& keys = TEtextKeys());
    void AtomicGetSeInfo(const TString& url, TInfo& info, bool getQuery = true, bool toLower = false, bool needQuery = true, const TEtextKeys& keys = TEtextKeys());
}; // namespace NSe
