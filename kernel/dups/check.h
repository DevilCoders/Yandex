#pragma once

#include "fml.h"
#include "banned_info.h"

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/digest/fnv.h>
#include <util/memory/pool.h>
#include <library/cpp/regex/pcre/regexp.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/vector.h>

#include <array>
#include <util/string/split.h>

void SeparateHostPath(const TStringBuf& , TString& , TString&, bool normalize);

namespace NDups {
    enum class EDupType {
        BY_URL_MATCH,
        BY_URL_REDIRECT,
        BY_URL_MOBILE,
        BY_URL_BANNED,
        BY_URL_ORIGINAL,
        BY_HOST_MATCH,
        BY_ATTR_CLONE,
        BY_ATTR_CLONE_TEST,
        BY_ATTR_OFFLINE_DUPS,
        BY_ATTR_FAKE,
        BY_ATTR_UNKNOWN,
        BY_SIG_LONG_SIMHASH,
        BY_SIG_LONG_SIMHASH_EXACT,
        BY_SIG_STATIC,
        BY_SIG_STATIC_EXACT,
        BY_SIG_DYNAMIC,
        BY_SIG_DYNAMIC_EXACT,
        BY_SIG_SNIP,
        BY_SIG_SNIP_EXACT,
        BY_SIG_TITLE_EXACT,
        BY_FORMULA,
        BY_FORMULA_HALF_DUP,
        BY_URL_UNGLUE,
        BY_REL_CANONICAL,
        BY_MAIN_CONTENT_URL,
        BY_HREFLANG,
        NOT_DUP,
        Count
    };

    enum EDupMode {
        UrlOnly = 0,
        Signatures = 6,
    };

    /// events at group glueing
    enum class EEvent {
        DupRemoved,
        DupRearranged,
        DupFound
    };

    enum class ECheckedSourceType {
        NonChecked = 0,
        Web,
        WebMisspell,

        Count
    };

    class TCheckedSourceFrequencies {
    private:
        using TContainer = std::array<size_t, static_cast<size_t>(ECheckedSourceType::Count)>;

    private:
        TContainer PerSourceCount;

    public:
        TCheckedSourceFrequencies();

        void AddOne(ECheckedSourceType st);
        size_t GetMostFrequentCheckedSourceCount() const;

        template <typename TIterator, typename TGetValue>
            static TCheckedSourceFrequencies Count(const TIterator& begin, const TIterator& end, const TGetValue& value) {

            TCheckedSourceFrequencies res;
            for(auto it  = begin; it != end; ++it)
                res.AddOne(value(*it));

            return res;
        }
    };


    EDupMode GetDupModeFromInt(int mode);

    struct TCheckedDoc {
        ui32 DocPosition = 0;           // An index in docs list
        TStringBuf Url;         // URL

        TCheckedDoc(ui32 idx, TStringBuf url)
            : DocPosition(idx)
            , Url(url)
        {
        }

        TCheckedDoc()
        {
        }
    };

    struct TCheckResult {
        TCheckedDoc Doc;  // Position and url of the first (main) doc in group
        EDupType DupType;  // Check result

        TCheckResult(TCheckedDoc doc, EDupType dupType)
            : Doc(doc)
            , DupType(dupType)
        {
        }

        bool operator == (const TCheckResult& c) const {
            return Doc.DocPosition == c.Doc.DocPosition && DupType == c.DupType;
        }

        bool operator < (const TCheckResult& other) const {
            if (Doc.DocPosition == other.Doc.DocPosition) {
                return DupType < other.DupType;
            }
            return Doc.DocPosition < other.Doc.DocPosition;
        }
    };

    class TUrlRegionsInfo {
    private:
        struct THostGroup {
            struct TRegionalHost {
                TRegionalHost(const TString& hostName = "", const TString& regionName = "")
                    : HostName(hostName)
                    , RegionName(regionName)
                {
                }
                TString HostName;
                TString RegionName;
            };
            THostGroup()
                : GroupHash(0)
            {
            }
            bool Empty() const noexcept {
                return RegionalHosts.empty() && RegularExps.empty();
            }
            void Reset() {
                MainHost = "";
                RegionalHosts.clear();
                RegularExps.clear();
            }
            void AddRegional(const TRegionalHost& regionalHost) {
                RegionalHosts.push_back(regionalHost);
            }
            void AddRegExp(const TString& regExp) {
                RegularExps.push_back(TRegExMatch(regExp.data()));
            }
            bool HasRegExps() const noexcept {
                return !RegularExps.empty();
            }
            bool Satisfies(const TString& host) const {
                for (TVector<TRegExMatch>::const_iterator i = RegularExps.begin(), e = RegularExps.end();
                    i != e; ++i) {
                    if (i->Match(host.data())) {
                        return true;
                    }
                }
                return false;
            }
            TString MainHost;
            ui32 GroupHash; //Hash value of MainHost
            TVector<TRegionalHost> RegionalHosts;
            TVector<TRegExMatch> RegularExps; //regular expression for duplicate hosts
        };
        struct THostInfo {
            THostInfo(ui32 groupHash, ui32 regId)
                : GroupHash(groupHash)
                , RegId(regId)
            {
            }
            ui32 GroupHash;
            ui32 RegId;
        };
        typedef THostGroup::TRegionalHost TRegionalHost;
    public:
        static const ui32 UnknownRegion;

        enum EUrlSearch {
            EFoundRegional,
            EFound,
            ENotFound,
        };

    public:
        TUrlRegionsInfo(IInputStream& groupsFile)
        {
            LoadRegionalData(groupsFile);
            InitializeData();
        }
        TUrlRegionsInfo(const TString& regFileName)
        {
            TFileInput groupsFile(regFileName);
            LoadRegionalData(groupsFile);
            InitializeData();
        }

        ui32 GetRegion(const TString& regName) const {
            return RegionHashes.Value(regName, UnknownRegion);
        }

        EUrlSearch ModifyUrlKeyToMainKey(ui32& key, ui32 regionId) const {
            //key is some hash value based on host of the currently looked url
            //the key set of statically calculated urls are precalculated and
            //if this is one of them, this is replaced to the leader key of the
            //group, so that, for example, google.com add google.com.tr will have the same key
            THashMap<ui32, THostInfo>::const_iterator it = HostHashes.find(key);
            if (it != HostHashes.end()) {
                key = it->second.GroupHash;
                if (it->second.RegId == regionId)
                    return EFoundRegional;
                return EFound;
            }
            return ENotFound;
        }

        void SearchRegExps(const TString& host, ui32& key) const {
            for (size_t i = 0; i < RegularGroups.size(); ++i) {
                if (HostGroups[RegularGroups[i]].Satisfies(host)) {
                    key = HostGroups[RegularGroups[i]].GroupHash;
                    break;
                }
            }
        }
    private:
        void InitializeData() {
            //Initialize useful data,init host hashes, region hashes, etc.
            TString hostPart, pathPart;
            ui32 regionCount = 0;
            TVector<THostGroup>::iterator begIt = HostGroups.begin();
            for (TVector<THostGroup>::iterator it = begIt, endIt = HostGroups.end();
                                 it != endIt; ++it)
            {
                TStringBuf url(it->MainHost);
                SeparateHostPath(url, hostPart, pathPart, true);
                ui32 groupHash = FnvHash<ui32>(hostPart.data(), hostPart.size());
                it->GroupHash = groupHash;
                for (TVector<TRegionalHost>::const_iterator rit = it->RegionalHosts.begin();
                                      rit != it->RegionalHosts.end(); ++rit)
                {
                    url = rit->HostName;
                    SeparateHostPath(url, hostPart, pathPart, true);
                    ui32 hash = FnvHash<ui32>(hostPart.data(), hostPart.size());

                    ui32 regionId;
                    THashMap<TString, ui32>::iterator regIt = RegionHashes.find(rit->RegionName);
                    if (regIt == RegionHashes.end())
                        RegionHashes[rit->RegionName] = regionId = ++regionCount;
                    else
                        regionId = regIt->second;

                    HostHashes.insert(THashMap<ui32, THostInfo>::value_type(hash, THostInfo(groupHash, regionId)));
                }
                if (it->HasRegExps()) {
                    RegularGroups.push_back(it - begIt);
                }
            }
        }

        void LoadRegionalData(IInputStream& groupsFile) {
            //processing the file
            //and storing infos about the region of each url, global urls,
            //regular expressions for regioanal urls, etc
            TString hostLine;
            THostGroup hostGroup;

            while (groupsFile.ReadLine(hostLine))
            {
                TVector<TString> hostInfo;
                StringSplitter(hostLine).SplitBySet(" \t").SkipEmpty().Collect(&hostInfo);

                if (hostInfo.empty())
                    continue;

                const TString& hostType = hostInfo[0];
                if (hostType == "global") {
                    if (!hostGroup.Empty())
                        HostGroups.push_back(hostGroup);
                    hostGroup.Reset();
                    hostGroup.MainHost = hostInfo[1];
                }
                else if (hostType == "regional") {
                    if (hostGroup.MainHost.empty())
                        ythrow yexception() << "Main host in group should be saved first" << Endl;
                    else
                        hostGroup.AddRegional(TRegionalHost(hostInfo[1], hostInfo[2]));
                }
                else if (hostType == "regexp") {
                    hostGroup.AddRegExp(hostInfo[1]);
                }
            }
            if (!hostGroup.Empty())
                HostGroups.push_back(hostGroup);
        }
    private:
        TVector<THostGroup> HostGroups;
        THashMap<ui32, THostInfo> HostHashes;
        THashMap<TString, ui32> RegionHashes;
        TVector<size_t> RegularGroups;
    };

    class TUrlZoneManager {
    public:
        TUrlZoneManager(const TUrlRegionsInfo& regionsInfo, const TVector<TString>& regNames)
            : RegionsInfo(regionsInfo)
            , SeenRegionalUrl(false)
            , SeenNonRegionalUrl(false)
        {
            //ZoneManager gets information about regions of query
            //and interpretes regional and global hosts, so that only duplicate urls
            //from given regions will survive
            RegionIds.resize(regNames.size());
            for (size_t i=0; i<RegionIds.size(); ++i) {
                RegionIds[i] = RegionsInfo.GetRegion(regNames[i]);
            }
        }

        bool RedefineKey(const TStringBuf& url, ui32& key) {
            //key is some hash value based on host of the currently looked url
            //the key set of statically calculated urls are precalculated and
            //if this is one of them, this is replaced to the leader key of the
            //group, so that, for example, google.com add google.com.tr will have the same key
            bool nonRegionalFound = false;
            ui32 convertedKey = (ui32)-1;
            ui32 initialKey = key;

            for (size_t i=0; i<RegionIds.size(); ++i) {
                key = initialKey;
                TUrlRegionsInfo::EUrlSearch us = RegionsInfo.ModifyUrlKeyToMainKey(key, RegionIds[i]);
                if (us == TUrlRegionsInfo::EFoundRegional) {
                    SeenRegionalUrl = true;
                    RegionalUrls.insert(url);
                    return true;
                }
                else if (us == TUrlRegionsInfo::EFound) {
                    nonRegionalFound = true;
                    convertedKey = key;
                }
            }

            if (nonRegionalFound) {
                SeenNonRegionalUrl = true;
                NonRegionalUrls.insert(url);
                key = convertedKey;
                return true;
            }
            return false;
        }

        void LookForRegExps(const TString& host, ui32& key) {
            RegionsInfo.SearchRegExps(host, key);
        }

        bool IsRegionalUrl(const TStringBuf& url) {
            //true if the given url is regional url
            //tr-tr.facebook.com instead of facebook.com, etc
            return SeenRegionalUrl ? RegionalUrls.find(url) != RegionalUrls.end() : false;
        }

        bool IsNonRegionalUrl(const TStringBuf& url) {
            //true if the given url is nonregional url
            //for example, tr-tr.facebook.com is nonregional url if
            //query country is Russia
            return SeenNonRegionalUrl ? NonRegionalUrls.find(url) != NonRegionalUrls.end() : false;
        }
    private:
        const TUrlRegionsInfo& RegionsInfo;
        bool SeenRegionalUrl;
        bool SeenNonRegionalUrl;
        TVector<ui32> RegionIds;
        THashSet<TStringBuf> RegionalUrls;
        THashSet<TStringBuf> NonRegionalUrls;
    };

    class TAntidupWhiteList {
    public:
        TAntidupWhiteList(const TString& dirName, const TString& whiteLists, const TString& whiteUrlLists);
        bool IsDocInWhiteList(const TStringBuf& docUrl) const;

    private:
        THashSet<TString> Hosts;
        THashSet<TString> Urls;
    };

    class TGluePreventer {
    public:
        TGluePreventer(const THashSet<TStringBuf>& winnerUrls)
            : WinnerUrls(winnerUrls)
        {
        }
        bool DontGlue(TStringBuf url) const
        {
            url.Skip(GetHttpPrefixSize(url));
            TStringBuf hostname = CutWWWPrefix(GetHost(url));
            return WinnerUrls.find(hostname) != WinnerUrls.end();
        }
    private:
        const THashSet<TStringBuf>& WinnerUrls;
    };

    ui32 GetHashKeyForUrl(const TStringBuf& dataUrl, bool normalize, ui32* hostKey = nullptr);

}  // Namespace NDups
