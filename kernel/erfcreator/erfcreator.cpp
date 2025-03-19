#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/datetime/cputimer.h>
#include <library/cpp/regex/pcre/regexp.h>

#include <library/cpp/microbdb/safeopen.h>
#include <library/cpp/deprecated/dater_old/structs.h>
#include <library/cpp/deprecated/dater_old/scanner/dater.h>
#include <library/cpp/on_disk/aho_corasick/writer.h>
#include <library/cpp/on_disk/aho_corasick/reader.h>

#include <kernel/search_types/search_types.h> //for TCateg

#include <ysite/yandex/erf_format/erf_format.h>
#include <ysite/yandex/erf_format/host_erf_format.h>
#include <ysite/yandex/dates/doctime.h>
#include <ysite/yandex/erf_format/erf_format.h>

#include <kernel/erfcreator/urllen/urllen.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/region2country/countries.h>
#include <kernel/remap/remaps.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <kernel/groupattrs/attrweightprop.h>// for NGroupingAttrs::TAttrWeightPropagator
#include <kernel/herf_hash/herf_hash.h>

#include <kernel/gsk_model/batch_gsk.h> // for TBatchRegexpCalcer

#include <yweb/robot/dbscheeme/baserecords.h>
#include <yweb/robot/dbscheeme/mergerecords.h>
#include <yweb/protos/hostfactors.pb.h>
#include <yweb/protos/docfactors.pb.h>
#include <yweb/protos/indexeddoc.pb.h>
#include <yweb/protos/robot.pb.h>
#include <google/protobuf/descriptor.h> // for descriptors

#include "tfattr.h"
#include "erfcreator.h"
#include "orangeattrs.h"

const char* isCommByKeyWordsCategory = "secta1"; // 36000000 catalog value family
const char* isCommByKeyWordsValue = "2";
const ui32 isCommByKeyWordsGroupValue = 36000002; // "secta1" group plus IsCommByKeywordsValue

class TAhoWrapper {
private:
    static TBlob Build(const char** data) {
        TDefaultAhoCorasickBuilder builder;
        while (*data) {
            builder.AddString(*data, 0);
            ++data;
        }
        return builder.Save();
    }

    TDefaultMappedAhoCorasick AhoCorasick;

public:
    TAhoWrapper(const char** data)
        : AhoCorasick(Build(data))
    {
    }

    ui32 Exists(const TString& s) const {
        return AhoCorasick.AhoContains(s) ? 1 : 0;
    }
};

#define WRAP(X) static const TAhoWrapper aho##X(X);

static const char* szNewsFrags[] = {
    "news", "News", "lenta.ru", "regnum.ru", "newsru.ru", "www.redtram.ru", "www.podrobnosti.ua", "www.gazeta.ru",
    "www.rian.ru", "www.24news.ru", "www.novoteka.ru", "ura-inform.com",
    nullptr
};
WRAP(szNewsFrags);

static const char* szCatalogFrags[] = {
    "catalog", "katalog", "www.isun.ru", "www.iskoni.ru", "www.6sotok.com", "/cat", "/link0",
    "www.exys.ru", "soft-driver.com", "/dir3/",
    nullptr
};
WRAP(szCatalogFrags);

static const char* szShopFrags[] = {
    "shop", "/offers/", "/showtov", "buy", "pay",
    "centermag.ru", "neobook.ru", "www.knizhniy.com", "www.knizhnaya.net", "www.muna.ru", "www.books-shop.info",
    "www.ozon.ru", "www.pricespb.ru", "www.moscowbooks.ru", "goods.axes.ru", "www.bearbooks.ru",
    nullptr
};
WRAP(szShopFrags);

static const char* szLivejournalFraqs[] = {
    "livejournal.com",
    nullptr
};
WRAP(szLivejournalFraqs);

static const char* szObsoleteFraqs[] = {
    "1996",
    "1997",
    "1998",
    "1999",
    "2001",
    "2002",
    "2003",
    "2004",
    "2005",
    "2006",
    "2007",
    "2008",
    nullptr
};
WRAP(szObsoleteFraqs);

static const char* szInternationalDomainZones[] = {
    "com", "org", "net", "edu", "gov", "info", "name", "biz",
    "to", "tv", "tk", "cc", "tc", "co", "me", "im", "ws", "eu",
    "news",
    nullptr
};

bool IsInternationalDomain(TString zone){
    const char** it = szInternationalDomainZones;
    while (*it && zone != *it)
        ++it;
    return *it && zone == *it;
}

void FillErfAggregatedValues(const TErfAttrs& attrs, SDocErfInfo3& erf, time_t currentTime) {
    NDater::TDaterStats stats;
    ReadDaterStats(stats, attrs);

    // fill Mtime MtimeFrom and MtimeAccuracy
    NDater::TTimeAndSource dt = MakeTimeAndSource(erf, stats, currentTime);
    dt.Apply(erf);

    if (!stats.Empty())
        FillErfFromDaterStats(stats, erf, currentTime);
}

void FillErfFromErfAttrs(const TErfAttrs& attrs, SDocErfInfo3& erf) {
    FillErfTextFeatures(attrs, erf);
    FillErfTimeCrawlRankVisitRate(attrs, erf);
    FillErfDaterFieldsFromErfAttrs(attrs, erf);
    FillErfQueryFactors(attrs, erf);
    FillErfTitleFeatures(attrs, erf);
}

void FillErfDaterFieldsFromErfAttrs(const TErfAttrs& attrs, SDocErfInfo3& erf) {
    TString daterDate;
    TErfAttrs::const_iterator i = attrs.find(DATER_DATE_ATTR);
    if (i != attrs.end()) {
        daterDate = i->second;
    }

    NDater::TDaterDate dater(NDater::TDaterDate::FromString(daterDate));
    WriteBestDateToErf(dater, erf);
}

void ReadDaterStats(NDater::TDaterStats& stats, const TErfAttrs& attrs) {
    TErfAttrs::const_iterator ity = attrs.find(DATER_STATS_ATTR);
    TErfAttrs::const_iterator itmy = attrs.find(DATER_STATS_MY_ATTR);
    TErfAttrs::const_iterator itdm = attrs.find(DATER_STATS_DM_ATTR);

    stats.Clear();

    if (ity != attrs.end())
        stats.FromString(ity->second);

    if (itmy != attrs.end())
        stats.FromString(itmy->second);

    if (itdm != attrs.end())
        stats.FromString(itdm->second);
}

NDater::TTimeAndSource MakeTimeAndSource(const SDocErfInfo3& erf, const NDater::TDaterStats& stats, time_t currtime) {
    NDater::TTimeAndSource docTime;

    if (!stats.Empty()) {
        docTime = NDater::GetDocTimeAndSource(erf, currtime, &stats);
    } else {
        docTime = NDater::GetDocTimeAndSource(erf, currtime, nullptr);
    }

    return docTime;
}

static inline ui32 GetFieldFromAttrs(const TErfAttrs& attrs, const TString name) {
    TErfAttrs::const_iterator i = attrs.find(name);
    ui32 value = 0;
    if (i != attrs.end() && ! i->second.empty())
        value = FromString<ui32>(i->second);

    return value;
}

void FillErfQueryFactors(const TErfAttrs& attrs, SDocErfInfo3& erf) {
    erf.IsPorno = GetFieldFromAttrs(attrs, "IsPorno");
    erf.IsSEO = GetFieldFromAttrs(attrs, "IsSEO");
    erf.IsComm = GetFieldFromAttrs(attrs, "IsComm");
    erf.HasPayments = GetFieldFromAttrs(attrs, "HasPayments");
}

void FillErfTitleFeatures(const TErfAttrs& attrs, SDocErfInfo3& erf) {
    TErfAttrs::const_iterator it = attrs.find("TitleComm");
    if (it != attrs.end() && ! it->second.empty()) {
        erf.TitleComm = ClampVal<ui32>((ui32)(FromString<float>(it->second)*255.0), 0, 255);
    }
    it = attrs.find("TitleBM25Ex");
    if (it != attrs.end() && ! it->second.empty()) {
        erf.TitleBM25Ex = ClampVal<ui32>((ui32)(FromString<float>(it->second)*255.0), 0, 255);
    }
}

void FillDocFactorsFromUrl(const TString& url0, NRealTime::TDocFactors& proto, const IHostCanonizer* mainPageCanonizer) {
    TString url = TString{CutHttpPrefix(url0)};
    // Calc UrlLen factor without end slash deleting. (like in big robot)
    proto.SetUrlLen(CalcUrlLen(url));

    if (url[url.size()-1] == '/')
        url.resize(url.size() - 1);

    TString hostAndPort = TString{GetHostAndPort(url)};

    for (size_t pos = 0; url[pos]; ++pos)
        if (isdigit(url[pos])){
            proto.SetUrlHasDigits(1);
            break;
        }

    proto.SetIsWikipedia(hostAndPort == "ru.wikipedia.org");

    // TODO set IsIndexPage,IsIndexPageSoft,IsMainMirrorUrl and MinSemiDuplicatesPathLen

    url.to_lower();

    FillUrlBasedFactors(url, proto);

    if (url.size() == hostAndPort.size()) {
        proto.SetnRoot(1);
        proto.SetnNews(0);

        if (mainPageCanonizer) {
            hostAndPort.to_lower();
            TString mainPageCanonizedHost = mainPageCanonizer->CanonizeHost(hostAndPort);
            if ((hostAndPort == mainPageCanonizedHost) || hostAndPort == TString::Join("www.", mainPageCanonizedHost)) {
                proto.SetIsMainPage(1);
            }
        }
    }
}

#define ExistsSubstringFromSet(szUrl, X) aho##X.Exists(szUrl)

template<class TConcretteFiller>
static void FillUrlBased(const TString url, TConcretteFiller& filler) {
    if (ExistsSubstringFromSet(url, szNewsFrags))
        filler.SetnNews(true);
    if (ExistsSubstringFromSet(url, szCatalogFrags))
        filler.SetnCatalog(true);
    filler.SetnShop(ExistsSubstringFromSet(url, szShopFrags));
    filler.SetIsLJ(ExistsSubstringFromSet(url, szLivejournalFraqs));
    filler.SetIsObsolete(ExistsSubstringFromSet(url, szObsoleteFraqs));

    ui32 numSlashes = 0;                        // may be differences in big robot, because slash on end of the url may be present or no.
    for(size_t i = 0; i < url.size(); ++i)
        if (url[i] == '/' && numSlashes < 15)
            ++numSlashes;

    filler.SetNumSlashes(numSlashes);
}

class TSDocErfInfo3Filler {
private:
    SDocErfInfo3& Erf;
    const SDocErfInfo3::TFieldMask* FieldMask;

public:
    TSDocErfInfo3Filler(SDocErfInfo3& erf, const SDocErfInfo3::TFieldMask* fieldMask)
        : Erf(erf)
        , FieldMask(fieldMask)
    {
    }

    void SetnNews (bool value) {
        Y_ASSERT(value);

        if (FieldMask && !FieldMask->nNews)
            return;

        Erf.nNews = 1;
    }

    void SetnCatalog (bool value) {
        Y_ASSERT(value);

        if (FieldMask && !FieldMask->nCatalog)
            return;

        Erf.nCatalog = 1;
    }

    void SetnShop (bool value) {
        if (FieldMask && !FieldMask->nShop)
            return;

        Erf.nShop = value;
    }

    void SetIsLJ (bool value) {
        if (FieldMask && !FieldMask->IsLJ)
            return;

        Erf.IsLJ = value;
    }

    void SetIsObsolete (bool value) {
        if (FieldMask && !FieldMask->IsObsolete)
            return;

        Erf.IsObsolete = value;
    }

    void SetNumSlashes (ui32 value) {
        if (FieldMask && !FieldMask->NumSlashes)
            return;

        Erf.NumSlashes = value;
    }
};

void FillUrlBasedFactors(const TString& szUrl, SDocErfInfo3& erf)
{
    TSDocErfInfo3Filler filler(erf, nullptr);
    FillUrlBased(szUrl, filler);
}


void FillUrlBasedFactors(const TString& szUrl, const SDocErfInfo3::TFieldMask& fieldMask, SDocErfInfo3& erf)
{
    TSDocErfInfo3Filler filler(erf, &fieldMask);
    FillUrlBased(szUrl, filler);
}

void FillUrlBasedFactors(const TString& szUrl, NRealTime::TDocFactors& proto)
{
    FillUrlBased(szUrl, proto);
}

#define ERF_FROM_DOC_FACTORS()\
    SET_FIELD(nNews);                                           \
    SET_FIELD(nCatalog);                                        \
    SET_FIELD(nShop);                                           \
    SET_FIELD(IsLJ);                                            \
    SET_FIELD(IsObsolete);                                      \
    SET_FIELD(ManualAdultness);                                 \
    SET_FIELD(UrlHasDigits);                                    \
    SET_FIELD(UrlLen);                                          \
    SET_FIELD(nRoot);                                           \
    SET_FIELD(IsUkr);                                           \
    SET_FIELD(Adultness);                                       \
                                                                \
    SET_FIELD(IsCommByKeywords);                                \
    SET_FIELD(IsMainPage);                                      \
    SET_FIELD(NumSlashes);                                      \
    SET_FIELD(UrlTrigrams);

#define ERF_FROM_ORANGE_DOC_FACTORS()\
    SET_FIELD(IsHTML);                                          \
    SET_FIELD(AddTime);                                         \
    SET_FIELD(AddTimeFull);                                     \
    SET_FIELD(Language);

static ui32 GetRobotHops(ui32 orangeHops) {
    // Map Orange hops to robot hops. If host is unknown it is marked as unreachable in robot.
    // Also Orange starts hops from 1 while robot does that starting from 0.
    return ConvertOrangeToRobotHops(orangeHops);
}

void FillErfFromDocFactorsProto(const NRealTime::TDocFactors& proto, SDocErfInfo3& erf)
{
#define SET_FIELD(F) erf.F = Max((ui32)proto.Get##F(), erf.F);
    ERF_FROM_DOC_FACTORS()
#undef SET_FIELD
    erf.ManualNonAdultness = Max((ui32)proto.GetManualNonAdultness(), erf.ManualNonAdultness);
    erf.HasPornoMenu = Max((ui32)proto.GetHasPornoMenu(), erf.HasPornoMenu);

    const NRealTime::TDocFactors_TOrangeDocFactors& orangeFactors = proto.GetOrangeDocFactors();
#define SET_FIELD(F) erf.F = orangeFactors.Get ##F ();
    ERF_FROM_ORANGE_DOC_FACTORS();
#undef SET_FIELD
    erf.PubTime = orangeFactors.GetPubTime();

    erf.Hops = GetRobotHops(orangeFactors.GetHops());
    erf.HopsFixed = erf.Hops;

    // Calculate it since RtIndexer doesn't provide value right now.
    erf.IsUnreachable = (erf.Hops == HOP_MAX);

    /*
    ticket ARC-993
    if (currentTime)
        erf.AddTimeMP = RemapTime(currentTime, orangeFactors.GetHostAddTime());
    else
        erf.AddTimeMP = RemapTimeFromCurrent(orangeFactors.GetHostAddTime());
    */
}

void FillOrangeDocFactorsFromDocInfo(const TDocInfoEx& docInfo, NRealTime::TDocFactors::TOrangeDocFactors& proto, size_t currentTime)
{
    if (docInfo.DocHeader != nullptr) {
        proto.SetLanguage(docInfo.DocHeader->Language);
        proto.SetIsHTML(docInfo.DocHeader->MimeType == MIME_HTML);
    }

    if (!!docInfo.AnchorData) {
        TMemoryInput input(docInfo.AnchorData, docInfo.AnchorSize);
        TAnchorText anchorText;
        anchorText.ParseFromArcadiaStream(&input);
        ui32 addTimeFull = anchorText.GetAddTime();
        ui32 lastExportTime = anchorText.GetExportTime();
        if (currentTime)
            proto.SetAddTime(RemapTime(currentTime, addTimeFull));
        else
            proto.SetAddTime(RemapTimeFromCurrent(addTimeFull));
        proto.SetAddTimeFull(addTimeFull);
        proto.SetHostSize(anchorText.GetHostSize());
        proto.SetHostAddTime(anchorText.GetHostAddTime());
        if (lastExportTime) {
            proto.SetLastExportTime(lastExportTime);
        }
    }

    proto.SetHops(docInfo.Hops);
}

void FillErfFromSpamFactors(const NRealTime::TSpamFactors& spamFactors, SDocErfInfo3* erfInfo) {
#define SET_FIELD(FIELD) erfInfo->FIELD = Max((ui32)spamFactors.Get##FIELD(), erfInfo->FIELD);

    SET_FIELD(IsForum);
    SET_FIELD(IsBlog);
    SET_FIELD(IsLongTitle);
    SET_FIELD(IsProgLang);
    SET_FIELD(HasLiRuCNT);
    SET_FIELD(DownloadVideo);
    SET_FIELD(Wap);
    SET_FIELD(IsGoodPrgForum);
    SET_FIELD(HasUserReviewL);
    SET_FIELD(NoUserReview);
    SET_FIELD(HasDownloadLinkOnFile);
    SET_FIELD(HasDownloadLinkOnFileHosting);
    SET_FIELD(HasHtml5VideoPlayer);
    SET_FIELD(IsReferat);
    SET_FIELD(HasVacancy);
    SET_FIELD(EmptyVacancy);
    SET_FIELD(IsChildish);
    SET_FIELD(IsNarco);
    SET_FIELD(IsSuicide);
    SET_FIELD(IsRepost);
    SET_FIELD(AntispamFlags);
    SET_FIELD(IsPopunder);
    SET_FIELD(IsClickunder);
    SET_FIELD(YellowAdv);
    SET_FIELD(TrashAdv);
#undef SET_FIELD

    erfInfo->NastyContent = Max((ui32)spamFactors.GetPornoWeight(), erfInfo->NastyContent);
}

void FillOrangeDocFactorsFromAnchorText(const TAnchorText& anchorText, NRealTime::TDocFactors_TOrangeDocFactors& proto) {
    if (anchorText.HasPubTime())
        proto.SetPubTime(anchorText.GetPubTime());

    if (anchorText.HasHostLanguage())
        proto.SetHostLanguage(LanguageByName(anchorText.GetHostLanguage().data()));

    for (size_t i = 0; i < anchorText.DocListInfoSize(); ++i) {
        proto.AddDocListInfo()->CopyFrom(anchorText.GetDocListInfo(int(i)));
    }
    for (size_t i = 0; i < anchorText.ExportStreamSize(); ++i) {
        proto.AddExportStream()->CopyFrom(anchorText.GetExportStream(int(i)));
    }
}

void FillErfAttrs(const NRealTime::TIndexedDoc& indexedDoc, SDocErfInfo3& erfInfo3, time_t currentTime) {
    TErfAttrs erfAttrs;
    for (size_t i = 0; i < indexedDoc.AttrsSize(); ++i) {
        const NRealTime::TIndexedDoc_TAttr &attr = indexedDoc.GetAttrs((int)i);
        erfAttrs[attr.GetName()] = attr.GetValue();
    }
    FillErfAttrs(indexedDoc, erfAttrs, erfInfo3, currentTime);
}

void FillErfAttrs(const NRealTime::TIndexedDoc& indexedDoc, const TErfAttrs& erfAttrs, SDocErfInfo3& erfInfo3, time_t currentTime) {
    FillErfAttrs(indexedDoc, erfAttrs, erfInfo3, currentTime, indexedDoc.GetUrl());
}

void FillErfAttrs(const NRealTime::TIndexedDoc& indexedDoc, const TErfAttrs& erfAttrs, SDocErfInfo3& erfInfo3, time_t currentTime, const TString& url4UrlHash) {
    Zero(erfInfo3);
    FillErfFromErfAttrs(erfAttrs, erfInfo3);
    //FillErfFromUrl(indexedDoc.GetUrl(), erfInfo3); it's duplicate of FillErfFromDocFactorsProto
    FillErfFromDocFactorsProto(indexedDoc.GetDocFactors(), erfInfo3);
    FillErfAggregatedValues(erfAttrs, erfInfo3, currentTime);
    erfInfo3.UpdateUrlHash(url4UrlHash);
}

void ProcessDocAttrs(NGroupingAttrs::TAttrWeightPropagator& propagator, const TFullDocAttrs& attrs, NRealTime::TDocFactors& docFactors) {
    for (TFullDocAttrs::TConstIterator i = attrs.Begin(); i != attrs.End(); ++i) {
        const TFullDocAttrs::TAttr& elem = *i;
        bool hasGeo = false;
        hasGeo |= elem.Name == "geoa";
        hasGeo |= elem.Name == "geo";
        hasGeo |= elem.Name == "geofor";
        hasGeo |= elem.Name == "geoabout";
        if (hasGeo) {
            TCateg geo = FromString<TCateg>(elem.Value);
            if (0.0f < propagator.Get(geo))
                docFactors.SetIsUkr(true);
        }

        // IsCommByKeyWords
        if (elem.Name == isCommByKeyWordsCategory && elem.Value == isCommByKeyWordsValue)
            docFactors.SetIsCommByKeywords(true);

    }
}

static const TString LibsHosts[] = {
    TString("russ.ru")
    , TString("aldebaran.ru")
    , TString("lib.ru")
    , TString("2lib.ru")
    , TString("fictionbook.ru")
    , TString("koob.ru")
    , TString("mexmat.ru")
    , TString("erlib.com")
    , TString("zipsites.ru")
    , TString("ladoshki.com")
    , TString("litera.ru")
    , TString("imwerden.de")
    , TString("litportal.ru")
    , TString("fenzin.org")
    , TString("stihi-rus.ru")
    , TString("feb-web.ru")
};

void FillHostFactorsFromHost(const TString& host, const TCanonizers& canonizers, NRealTime::THostFactors& proto) {
    const TString owner = canonizers.ReverseDomainCanonizer->CanonizeHost(canonizers.OwnerCanonizer->CanonizeHost(host));
    ::FillHostFactorsFromHost(host, owner, proto);
}

void FillHostFactorsFromHost(const TString& host, const TString& owner, NRealTime::THostFactors& proto) {
    // For even more space economy, skip zeroes in the final protobuf
#define SET_FIELD(FIELD, VALUE) { \
    const ui32 value = VALUE; \
    if (value != 0) { \
        proto.Set ## FIELD(value); \
    } \
}

    SET_FIELD(NationalDomainId, TRelevCountryInfo::GetNationalDomainId(host));
    SET_FIELD(IsInternationalDomain, IsInternationalDomain(TString(GetZone(host))));

    // All "ru" sites are russians by Romanenko
    if (TRelevCountryInfo::IsDomainNational(COUNTRY_RUSSIA, proto.GetNationalDomainId())) {
        SET_FIELD(HostLanguage, LANG_RUS);
    }

    SET_FIELD(IsCom, host.EndsWith(".com"));
    SET_FIELD(IsWikipediaHost, host == "wikipedia.org" || host.EndsWith(".wikipedia.org"));

    ui32 yearInHost = NDater::GetHostYear(host) - NDater::TDaterDate::ErfZeroYear;
    SET_FIELD(DaterStatsYearInHost, Min<ui32>(yearInHost, 31));

    static const TString wikipediaHost("ru.wikipedia.org");
    if (host == wikipediaHost)
        SET_FIELD(IsWikipedia, true);

    bool isLib = false;
    for (size_t i = 0; i < sizeof(LibsHosts) / sizeof(LibsHosts[0]); ++i) {
        const TString& lib = LibsHosts[i];
        if (host.EndsWith(lib))
            if (host.size() == lib.size() || host[host.size() - (lib.size()) - 1] == '.') {
                isLib = true;
                break;
            }
    }

    if (isLib)
        SET_FIELD(IsLib, isLib);

    SET_FIELD(HostHash, NHerfHash::HostHash(host));
    SET_FIELD(OwnerHash, NHerfHash::OwnerHash(owner));
#undef  SET_FIELD
}

void FillHerfFromHostFactorsProto(const NRealTime::THostFactors& proto, THostErfInfo& herf) {
#define SETFIELD(F) herf.F = proto.Get ## F ();
    SETFIELD(IsVendor);
    SETFIELD(IsCommercial);
    SETFIELD(HostCommercialRank);
    SETFIELD(IsInternational);
    SETFIELD(IsLib);
    SETFIELD(IsImportant);
    SETFIELD(YabarHostVisits);
    SETFIELD(CommLinksHostSEO);
    SETFIELD(YaBar);
    // dword border
    SETFIELD(LogCtrMean);
    //36 dword border
    SETFIELD(Bookmarks);
    SETFIELD(HostRank);
    SETFIELD(NewsCit);
    SETFIELD(Spam2);
    // dword border
    SETFIELD(Nevasca1);
    SETFIELD(Nevasca2);
    SETFIELD(Nevasca3);
    // dword border
    SETFIELD(LiruInternalTraffic1);
    SETFIELD(LiruInternalTraffic2);
    SETFIELD(LiruOutTraffic1);
    SETFIELD(LiruOutTraffic2);
    // dword border
    SETFIELD(LiruSearchTraffic1);
    SETFIELD(LiruSearchTraffic2);
    SETFIELD(YaBarCoreOwner);
    // dword border
    SETFIELD(IsCom); // for code unification (some time ago filled in FillHerfFromHost)
    SETFIELD(DaterStatsYearInHost);
    herf.IsHostCISLanguagesDominance = proto.GetIsHostCISLanguagesDominance() ? 1 : 0;
    // dword border
    SETFIELD(YaBarCoreHost);
    SETFIELD(YabarHostSearchTraffic);
    SETFIELD(YabarHostInternalTraffic);
    SETFIELD(YabarHostAvgTime);
    // dword border
    SETFIELD(YabarHostAvgTime2);
    SETFIELD(YabarHostAvgActions);
    SETFIELD(YabarHostBrowseRank);
    SETFIELD(OwnerClicksPCTR);
    // dword border
    SETFIELD(OwnerSDiffClickEntropy);
    SETFIELD(OwnerSDiffShowEntropy);
    SETFIELD(OwnerSDiffCSRatioEntropy);
    // dword border
    SETFIELD(OwnerSessNormDur);
    if (proto.HasNoSpam()) {
        herf.NoSpam = proto.GetNoSpam();
    } else {
        herf.NoSpam = 255;
    }
    SETFIELD(HostSize);
    SETFIELD(HostLanguage);
    // dword border
    herf.HostQuality = proto.GetHostQuality() / 16;
    SETFIELD(HasAdv);
    SETFIELD(HasYandexAdv);
    SETFIELD(IsWikipedia);
    // dword border
    SETFIELD(TrafHostRank);
    SETFIELD(NationalDomainId); // now here for code unification (some time ago was in filled in FillHerfFromHost)
    SETFIELD(IsInternationalDomain);
    // dword border
    SETFIELD(BadBookmarks);
    SETFIELD(BookmarkUsers);
    // dword border
    SETFIELD(OwnerReqsPopularity);
    SETFIELD(HostSpeed);
    SETFIELD(HostReliability);
    // following comments are not mistake, but result of BUKI-2314
    // dword border
    // dword border
    // dword border
    // dword border
    SETFIELD(HostAdultness);
    SETFIELD(AddTimeMP);
    SETFIELD(YabarHostVisitors);
    SETFIELD(OwnerNavQuota);
    // dword border
    SETFIELD(OwnerSatisfied4Rate);
    SETFIELD(OwnerEnoughClicked);

    if (proto.HasOwnerAuraFactors()) {
        const TOwnerAuraFactors& auraProto = proto.GetOwnerAuraFactors();
#define SET_AURA_FIELD(F) herf.Aura ## F = (ui32)auraProto.Get ## F();
        SET_AURA_FIELD(OwnerLogUnique);
        SET_AURA_FIELD(OwnerLogShared);
        SET_AURA_FIELD(OwnerLogAuthor);
        SET_AURA_FIELD(OwnerLogUnauth);
        SET_AURA_FIELD(OwnerLogOrigin);
        SET_AURA_FIELD(OwnerRelAuthor);
        SET_AURA_FIELD(OwnerMeanSharedSpread);
#undef SET_AURA_FIELD
    }

    // dword border
    SETFIELD(SeoInPayLinks);       // BUKI-1243

    SETFIELD(RankComGoodness);     // SEARCHSPAM-3204
    SETFIELD(RankComGoodnessBar);  // SEARCHSPAM-3204

    SETFIELD(RankOnlineShop);      // SEARCHSPAM-3660
    SETFIELD(RankBoostGoodness);   // SEARCHSPAM-3858
    SETFIELD(IsBoard);             // SEARCHSPAM-3862

    SETFIELD(QueriesCommRatioLe005);    // SEARCHSPAM-3979
    SETFIELD(QueriesRatioMorda);        // SEARCHSPAM-3979
    SETFIELD(QueriesAvgCM2);            // SEARCHSPAM-3979
    SETFIELD(NumUrls);                  // SEARCHSPAM-3979

    SETFIELD(IsEncyc); //ROBOT-2577

    SETFIELD(YabarHostSurfTrDpNdLeafLn);    // BUKI-1334
    SETFIELD(YabarHostSurfTrNdTmGrDsp);     // BUKI-1334
    SETFIELD(YabarHostSurfTrNdTmLeafLn90);  // BUKI-1334
    SETFIELD(HostDownloadProbability);      // FACTOR-44
    SETFIELD(TorrentQueryShare);            // GATEWAY-854
    SETFIELD(TorrentClickShare);            // GATEWAY-854
    SETFIELD(YabarHostSurfTrNdHgGr);        // FACTOR-112

    SETFIELD(IsCatalogueCard); // SECCONT-239

    SETFIELD(HostHash);  // ROBOT-2772
    SETFIELD(OwnerHash); // ROBOT-2772
    SETFIELD(IsWikipediaHost); // ROBOT-2805

    SETFIELD(AdvAspam); // BUKI-1688

    SETFIELD(RegionalClicks);   // SEARCHSPAM-4017
    SETFIELD(IsReferatHost);    // SEARCHSPAM-4661
    SETFIELD(TestAntispamFeatures); // SEARCHSPAM-5493
    SETFIELD(AdsMarker); // SEARCHSPAM-5493
    SETFIELD(RankCommGoodnessUA); // SEARCHSPAM-5493
    SETFIELD(OwnerMeanRelevMx); // SEARCHSPAM-6197
    SETFIELD(TrashAdv); // SEARCHSPAM-6967
    // SEARCHSPAM-7857 for Mascot?? herfs
    SETFIELD(NastyHost);

#undef SETFIELD
}

template<class TVectorErf>
void WriteErfImpl(TVectorErf& erfVec, const TString& fileName) {
    TTimeLogger logger("writing results");
    TFixedBufferFileOutput fDst(fileName);
    TArrayWithHeadWriter<typename TVectorErf::value_type> writer(&fDst);
    writer.Put(erfVec.data(), erfVec.size());
    logger.SetOK();
}

void WriteErf(const TErfCreateConfig& config, TErfsRemap& erfVec, const TString& erFilefName) {
    TString fileName = erFilefName.empty() ? (config.Output + config.ErfFileName).data() : erFilefName;
    TTimeLogger logger("writing results");
    TFixedBufferFileOutput fDst(fileName);
    typename TErfsRemap::value_type dummy;
    memset(&dummy, 0, sizeof(dummy));
    TArrayWithHeadWriter<typename TErfsRemap::value_type> writer(&fDst);
    for (ui32 i = 0; i < erfVec.Remap.size(); i++) {
        if (erfVec.Remap[i] == (ui32)-1)
            writer.Put(dummy);
        else
            writer.Put(erfVec[i]);
    }
    logger.SetOK();
//    WriteErfImpl(erfVec, erFilefName.Empty() ? ~(config.Output + config.ErfFileName) : erFilefName);
}

void WriteErf(const TErfCreateConfig& config, TErfs& erfVec, const TString& erFilefName) {
    WriteErfImpl(erfVec, erFilefName.empty() ? (config.Output + config.ErfFileName).data() : erFilefName);
}

void WriteErf2(const TErfCreateConfig& config, TVector<SDocErf2Info>& erfVec) {
    WriteErfImpl(erfVec, config.Output + "indexerf2");
}

template<class TVectorErf, class TErfFieldMask, class TMerger>
void PatchErfImpl(TVectorErf& erfVec, const TString& inputFile,
        const TString& outputFile, const TErfFieldMask& fieldMask, TMerger merger) {
    TTimeLogger logger("patching results");
    TFixedBufferFileOutput fDst(outputFile);
    TArrayWithHeadWriter<typename TVectorErf::value_type> writer(&fDst);
    TArrayWithHead<typename TVectorErf::value_type> inputErf;
    inputErf.Load(inputFile.data());
    TErfFieldMask emptyMask;
    for (ui32 doc = 0; doc < inputErf.GetSize(); ++doc) {
        typename TVectorErf::value_type entry = inputErf.GetAt(doc);
        merger(entry, emptyMask, erfVec[doc], fieldMask, true);
        writer.Put(entry);
    }
    logger.SetOK();
}

void PatchErf(const TErfCreateConfig& config, TErfsRemap& erfVec, const TString& erfFileName) {
    SDocErfInfo3::TFieldMask fieldMask = config.GetErfFieldMask();
    PatchErfImpl(erfVec, config.ErfInput, erfFileName.empty() ?
            (config.Output + config.ErfFileName).data() : erfFileName, fieldMask, SDocErfInfo3::DoMerge);
}

void PatchErf2(const TErfCreateConfig& config, TErfs2& erfVec) {
    SDocErf2Info::TFieldMask fieldMask = config.GetErf2FieldMask();
    PatchErfImpl(erfVec, config.ErfInput, config.Output + "indexerf2", fieldMask, SDocErf2Info::DoMerge);
}

template<class TVectorErf>
void ReadErfImpl(const TString& fileName, TVectorErf* erfVec, bool precharge) {
    TTimeLogger logger("reading erf");
    TArrayWithHead<typename TVectorErf::value_type> array;
    array.Load(fileName.data(), true, precharge);

    if (sizeof(typename TVectorErf::value_type) < array.GetRecordSize()) {
        ythrow yexception() << "Patching erf with smaller SDocErfInfo3 (or SDocErf2Info)";
    }

    erfVec->resize(array.GetSize());
    for (size_t i = 0; i < array.GetSize(); ++i) {
        typename TVectorErf::value_type* info = &(*erfVec)[i];
        memset(info, 0, sizeof(typename TVectorErf::value_type));
        memcpy(info, &array.GetAt(i), array.GetRecordSize());
    }

    logger.SetOK();
}

void ReadErf(const TErfCreateConfig& config, TErfs* erfVec) {
    ReadErfImpl(config.ErfInput.data(), erfVec, true);
}

void ReadErf(const TErfCreateConfig& config, TErfsRemap* erfVec) {
    ReadErfImpl(config.ErfInput.data(), erfVec, true);
}

void ReadErf2(const TErfCreateConfig& config, TVector<SDocErf2Info>* erfVec) {
    ReadErfImpl(config.ErfInput.data(), erfVec, true);
}

void ReadHerf(const TErfCreateConfig& config, TVector<THostErfInfo>* herfVec) {
    ReadErfImpl(config.HostErfInput.data(), herfVec, false);
}

ui32 ConvertOrangeToRobotHops(ui32 orangeHops) {
    if (orangeHops == 0) {
        return HOP_MAX;
    } else if (orangeHops < HOP_MAX) {
        return orangeHops - 1;
    } else {
        return HOP_MAX;
    }
}

typedef google::protobuf::Reflection TReflection;
typedef google::protobuf::FieldDescriptor TFieldDescriptor;

template<class TErfInfo>
bool FillErfCommon(const NRealTime::TIndexedDoc& indexedDoc, const TFieldDescriptor* fieldDesc, TErfInfo& erf) {
    const TReflection& reflection = *indexedDoc.GetReflection();

    if (fieldDesc && reflection.HasField(indexedDoc, fieldDesc)) {
        const TString& erfInfoData = reflection.GetString(indexedDoc, fieldDesc);
        memcpy(&erf, erfInfoData.data(), Min(sizeof(TErfInfo), erfInfoData.size()));
        return true;
    }

    return false;
}

bool FillErfFromIndexedDocBinary(const NRealTime::TIndexedDoc& indexedDoc, SDocErfInfo3& erf) {
    static const TFieldDescriptor* erfInfoDescriptor = NRealTime::TIndexedDoc::descriptor()->FindFieldByName("ErfInfo");
    return FillErfCommon<SDocErfInfo3>(indexedDoc, erfInfoDescriptor, erf);
}

bool FillErf2FromIndexedDocBinary(const NRealTime::TIndexedDoc& indexedDoc, SDocErf2Info& erf) {
    static const TFieldDescriptor* erf2InfoDescriptor = NRealTime::TIndexedDoc::descriptor()->FindFieldByName("Erf2Info");
    return FillErfCommon<SDocErf2Info>(indexedDoc, erf2InfoDescriptor, erf);
}

bool FillHostErfFromIndexedDocBinary(const NRealTime::TIndexedDoc& indexedDoc, THostErfInfo& erf) {
    static const TFieldDescriptor* hostErfInfoDescriptor = NRealTime::TIndexedDoc::descriptor()->FindFieldByName("HostErfInfo");
    return FillErfCommon<THostErfInfo>(indexedDoc, hostErfInfoDescriptor, erf);
}

double CalcUrlGskModel(const TString& url, const TBatchRegexpCalcer& urlClassifier) {
    TVector<int> str;
    size_t urlLen = (int)url.size();
    str.resize(urlLen);
    for (size_t i = 0; i < urlLen; ++i)
        str[i] = (ui8)url[i];
    float factors[1];
    factors[0] = (float)urlLen;
    return ComputeModel(urlClassifier, str, factors);
}

int CalcUrlGskModel4Erf(const TString& url, const TBatchRegexpCalcer& urlClassifier) {
    return ClampVal((int)((CalcUrlGskModel(url, urlClassifier) + 3) / 6 * 255), 0, 255);
}

bool IsAccessible(const TErfCreateConfig& config, const TString fileName) {
    return !config.Patch2 || NFs::Exists(fileName);
}
