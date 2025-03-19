#include "url_based.h"

#include "url_based_features.h"

#include <yweb/config/hostconfig.h>
#include <kernel/ngrams/ngrams_processor.h>
#include <yweb/robot/dbscheeme/baserecords.h>
#include <yweb/robot/dbscheeme/mergecfg.h>
#include <yweb/robot/urlgeo_ml/url_regnav_lin_classifier.h>
#include <library/cpp/microbdb/safeopen.h>
#include <library/cpp/regex/pire/pire.h>
#include "erfcreator.h"

#include <util/datetime/cputimer.h>
#include <library/cpp/regex/pcre/regexp.h>

class IUrlHandler {
public:
    virtual ~IUrlHandler() {
    }

    virtual void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* urlRec, const TUrlRec::TExtInfo* extInfo, const char* url) const = 0;
};

class TUrlHandlerBase : public IUrlHandler {
protected:
    const TErfCreateConfig& Config;
public:
    explicit TUrlHandlerBase(const TErfCreateConfig& config)
        : Config(config)
    {}

    ~TUrlHandlerBase() override {
    }
};

class TNgramProcessorHandler : public TUrlHandlerBase {
private:
    TNgramsProcessor NgramProcessor;
public:
    explicit TNgramProcessorHandler(const TErfCreateConfig& config)
        : TUrlHandlerBase(config)
        , NgramProcessor((config.ErfDataDir + "/ngrams_clusters.txt").data()
        , (config.ErfDataDir + "/ngrams_model.info").data())
    {}

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* /*urlRec*/, const TUrlRec::TExtInfo* /*extInfo*/, const char* url) const override {
        const float val = TNgramsProcessor::Remap(NgramProcessor.Match(url));
        static const ui32 maxValue = std::numeric_limits<ui8>::max();
        erf.UrlNGramsModel = ClampVal<ui32>(ui32(val * maxValue), 0, maxValue);
    }
};

class TLastAccessProcessorHandler : public TUrlHandlerBase {
public:
    explicit TLastAccessProcessorHandler(const TErfCreateConfig& config)
        : TUrlHandlerBase(config)
    {}

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* /*urlRec*/, const TUrlRec::TExtInfo* extInfo, const char* /*url*/) const override {
        if (extInfo != nullptr && extInfo->HasLastAccessTrue())
            erf.LastAccess = extInfo->GetLastAccessTrue();
        else
            erf.LastAccess = 0;
    }
};

class TUrlLength2Handler : public IUrlHandler {
public:
    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* /*urlRec*/, const TUrlRec::TExtInfo* /*extInfo*/, const char* url) const override {
        static const ui32 maxValue = std::numeric_limits<ui8>::max();
        erf.UrlLen2 = ClampVal<ui32>((ui32)strlen(url), 0, maxValue);
    }
};

class TIsIndexPageHandler : public IUrlHandler {
private:
    TRegExMatch IsIndexPageRE;
    TRegExMatch IsIndexPageSoftRE;
public:

    explicit TIsIndexPageHandler()
        : IsIndexPageRE("^/(?:index\\.(?:html?|php|shtml|xml|aspx?|jsp))?$")
        , IsIndexPageSoftRE("^/(?:index\\.(?:html?|php|shtml|xml|aspx?|jsp))?(?:$|\\?)")
    {}

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* /*urlRec*/, const TUrlRec::TExtInfo* /*extInfo*/, const char* url) const override {
        const TString path = TString{GetPathAndQuery(url)};

        if (IsIndexPageRE.Match(path.data()))
            erf.IsIndexPage = 1;

        if (IsIndexPageSoftRE.Match(path.data()))
            erf.IsIndexPageSoft = 1;
    }
};

class TIsNotCgiHandler : public IUrlHandler {
public:
    TIsNotCgiHandler()
    {}

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* /*urlRec*/, const TUrlRec::TExtInfo* /*extInfo*/, const char* url) const override {
        if (NErfCreator::IsNotCgi(url))
            erf.IsNotCgi = 1;
    }
};

class TRegionFromUrlHandler : public IUrlHandler {
private:
    NErfCreator::TUrlRegionFeaturesCalculator UrlRegionFeaturesCalculator;

public:
    TRegionFromUrlHandler(const TErfCreateConfig& config)
        : UrlRegionFeaturesCalculator(config.ErfDataDir + "/rns.models.tsv.gz")
    {}

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* /*urlRec*/, const TUrlRec::TExtInfo* /*extInfo*/, const char* url) const override {
        auto result = UrlRegionFeaturesCalculator.GetUrlRegionFeatures(url);

        erf.RegionFromUrl = result.RegionFromUrl;
        erf.RegionFromUrlProbability = result.RegionFromUrlProbability;
    }
};

// for HasMultimedia
class TUrlScanner {
public:
    TUrlScanner()
        : RegexpCount(0)
    {
    }

    void Scan(const TStringBuf& str, TVector<bool>& results) const {
        NPire::TScanner::State finalState =
            NPire::Runner(Scanner)
            .Run(str.begin(), str.end())
            .State();

        std::pair<const size_t*, const size_t*> matches = Scanner.AcceptedRegexps(finalState);

        results.assign(RegexpCount, false);
        for (const size_t* match = matches.first; match < matches.second; ++match) {
            results[*match] = true;
        }
    }

    void AddScanner(const TString& regexp) {
        NPire::TScanner scanner =
            NPire::TLexer(regexp)
            .AddFeature(Pire::Features::CaseInsensitive())
            .Parse()
            .Surrounded()
            .Compile<NPire::TScanner>();

        if (RegexpCount == 0) {
            Scanner = scanner;
        } else {
            NPire::TScanner glueScanner = NPire::TScanner::Glue(Scanner, scanner);
            if (!glueScanner.Empty()) {
                Scanner = glueScanner;
            } else {
                ythrow yexception() << "Glue of scanners failed.";
            }
        }
        ++RegexpCount;
    }

private:
    NPire::TScanner Scanner;
    size_t RegexpCount;
};

enum EHasMultimediaRegexNames {
    NHM_SOFT = 0,
    NHM_IMAGE,
    NHM_NOTIMAGE,
    NHM_FILM,
    NHM_NOTFILM,
    NHM_MUSIC,
    NHM_NOTMUSIC,
    NHM_GAME,
    NHM_NOTGAME,

    NHM_TOTALCOUNT
};

class THasMultimediaHandler : public IUrlHandler { // FACTOR-80
private:
    TUrlScanner ScannerArray;
public:

    explicit THasMultimediaHandler() {
        // IsSoftRE
        ScannerArray.AddScanner("(soft|apps|app?lication|download|torr?ent|inn?dir|program|file|skachat|referat)");
        // IsImageRE
        ScannerArray.AddScanner("(image|photo|foto|fotki|kartink[ai]|thumbnail|zastavk[ai]|w(all?)?papers|avatar|"
                                "izobrazheni[ijy]?a|kartinki|picture|gallery|resim|goruntu|demotivation|demotivator)");
        // IsNotImageRE
        ScannerArray.AddScanner("(wikipedia|otvet\\.mail\\.ru)");
        // IsFilmRE
        ScannerArray.AddScanner("(kaliteli|movie|smotret|film|kino|serial|season|video|watch|multfilmy|multiki|"
                                "sinema|film|filmleri|seyret|izle|serisi|bolum|sezon)");
        // IsNotFilmRE
        ScannerArray.AddScanner("(livejournal|wikipedia|odnoklassniki|liveinternet|otvet\\.mail\\.ru)");
        // IsMusicRE
        ScannerArray.AddScanner("(mu[sz]ic|pesni|pesnya|myzika|mp3|listen|melodiler|sarki|melodi|dinle|radyo|"
                                "song|mptri|muzon|muzic|muzik|audio|pesen|pesn([jy]a|i)|soundtrack)");
        // IsNotMusicRE
        ScannerArray.AddScanner("(livejournal|wikipedia|odnoklassniki|liveinternet|otvet\\.mail\\.ru)");
        // IsGameRE
        ScannerArray.AddScanner("(game|[-0-9./]igrat?|igr[aiy](forboys|forgirls|forkids|[-0-9./])|shooter|"
                                "strel[jy]alki|odievalki|(flash.*online|online.*flash)|oyna|oynamak|oyun|play)");
        // IsNotGameRE
        ScannerArray.AddScanner("(livejournal|vk\\.com|wikipedia|odnoklassniki|liveinternet|otvet\\.mail\\.ru)");
    }

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* /*urlRec*/, const TUrlRec::TExtInfo* /*extInfo*/, const char* url) const override {
        const TString path(url);
        TVector<bool> matches;
        ScannerArray.Scan(path, matches);

        if (matches[NHM_SOFT])
            erf.HasMultimedia = 4;
        else if (matches[NHM_IMAGE] && !matches[NHM_NOTIMAGE])
            erf.HasMultimedia = 1;
        else if (matches[NHM_FILM]  && !matches[NHM_NOTFILM])
            erf.HasMultimedia = 3;
        else if (matches[NHM_MUSIC] && !matches[NHM_NOTMUSIC])
            erf.HasMultimedia = 5;
        else if (matches[NHM_GAME] && !matches[NHM_NOTGAME])
            erf.HasMultimedia = 2;
    }
};

class TMinPathLenHandler : public TUrlHandlerBase {
public:
    explicit TMinPathLenHandler(const TErfCreateConfig& config)
        : TUrlHandlerBase(config)
    {}

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* /*urlRec*/, const TUrlRec::TExtInfo* /*extInfo*/, const char* url) const override {
        const ui32 pathLen = ui32(GetPathAndQuery(url).size());

        const ui32 oldLen =
            erf.MinSemiDuplicatesPathLen  > 0
            ? erf.MinSemiDuplicatesPathLen  // MinSemiDuplicatesPathLen  was inited before
            : Max<ui8>();   // this is main semiduplicate, MinSemiDuplicatesPathLen  was not inited

        erf.MinSemiDuplicatesPathLen = Min<ui32>(pathLen, oldLen);
    }
};

class TTotalDupsHandler : public TUrlHandlerBase {
private:
    mutable ui32 PrevDocId;

public:
    explicit TTotalDupsHandler(const TErfCreateConfig& config)
        : TUrlHandlerBase(config)
        , PrevDocId(Max<ui32>())
    {}

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* urlRec, const TUrlRec::TExtInfo* /*extInfo*/, const char* /*url*/) const override {
        // prevent holding first document into the dups
        if (urlRec->DocId != PrevDocId) {
            PrevDocId = urlRec->DocId;
            return;
        }

        if (erf.TotalDups < Max<ui8>())
            ++erf.TotalDups;
    }
};

class TNonLettersCounterHandler : public IUrlHandler {
public:
    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* /*urlRec*/, const TUrlRec::TExtInfo* /*extInfo*/, const char* url) const override {
        static const ui32 maxValue = std::numeric_limits<ui8>::max();
        ui32 nonLetters = 0;
        for(char ch; ch = *url; url++)
            if (ch < 'A' || (ch > 'Z' && ch < 'a') || ch > 'z')
                ++nonLetters;
        erf.NumNonLettersInUrl = ClampVal<ui32>(nonLetters, 0, maxValue);
    }
};

class THubHandler : public TUrlHandlerBase {
private:
    struct TUrlIdHash {
        ui64 operator() (const urlid_t& id) const {
            return CombineHashes<ui64>(IntHashImpl(id.UrlId), IntHashImpl(id.HostId));
        }
    };
    THashSet<urlid_t, TUrlIdHash> Hubs;
public:
    explicit THubHandler(const TErfCreateConfig& config)
        : TUrlHandlerBase(config)
    {
        TInDatFile<urlid_t> hubsFile("hubs.dat", dbcfg::fbufsize, 0);
        const TString hubsFileName = TString(config.RobotHome) + '/' + dbcfg::fname_dump_hubs;
        if (IsAccessible(config, hubsFileName))
            hubsFile.Open(hubsFileName);

        const urlid_t* hubsRec;
        while (hubsRec = hubsFile.Next())
            Hubs.insert(*hubsRec);
    }

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* urlRec, const TUrlRec::TExtInfo* /*extInfo*/, const char* /*url*/) const override {
        urlid_t id(urlRec->HostId, urlRec->UrlId);
        if (Hubs.contains(id))
            erf.IsHub = 1;
    }
};

class TSimhashHandler : public TUrlHandlerBase {
public:
    explicit TSimhashHandler(const TErfCreateConfig& config)
        : TUrlHandlerBase(config)
        , HostConfig()
        , ClusterCfg(nullptr)
        , SimhashFile("simhash.dat", dbcfg::fbufsize, 0)
        , SimhashRec(nullptr)
        , PrevDocId(0)
    {
        const TString simHashFileName = TString(config.RobotHome) + '/' + dbcfg::fname_dump_simhash;
        if (IsAccessible(config, simHashFileName))
            SimhashFile.Open(simHashFileName);

        if (HostConfig.Load((TString(config.RobotHome) + '/' + dbcfg::fname_hostconfig).data()) != 0) {
            ythrow yexception() << "Couldn't open host config";
        };
        ui32 mySeg = HostConfig.GetMySegmentId();
        ClusterCfg = HostConfig.GetSegmentClusterConfig(mySeg);
    }

    void ProcessUrl(SDocErfInfo3& erf, const TUrlRec* urlRec, const TUrlRec::TExtInfo* /*extInfo*/, const char* /*url*/) const override {
        if (urlRec->DocId < PrevDocId) {
            ythrow yexception() << "Non monotonous DocIds";
        }
        PrevDocId = urlRec->DocId;
        while (!SimhashFile.IsEof() && !SimhashFile.GetError()) {
            const TDocumentSimHash* simhashRec = GetSimHash();
            if (simhashRec == nullptr) {
                break;
            }
            if (simhashRec->DocId == docid_t::invalid) {
                SimhashRec = nullptr;
                continue;
            }
            ui32 cl, ldoc;
            ClusterCfg->Handle2Cluster(simhashRec->DocId, cl, ldoc);
            if (cl != (ui32)Config.RobotCluster) {
                SimhashRec = nullptr;
                continue;
            }
            if (ldoc < urlRec->DocId) {
                SimhashRec = nullptr;
                continue;
            } else if (ldoc == urlRec->DocId) {
                erf.DocStaticSignature1 = Hi32(SimhashRec->SegSimhash);
                erf.DocStaticSignature2 = Lo32(SimhashRec->SegSimhash);
            }
            break;
        }
    }

private:
    const TDocumentSimHash* GetSimHash() const {
        if (SimhashRec != nullptr) {
            return SimhashRec;
        }
        return SimhashRec = SimhashFile.Next();
    }

private:
    THostConfig HostConfig;
    const ClusterConfig* ClusterCfg;

    mutable TInDatFile<TDocumentSimHash> SimhashFile;
    mutable const TDocumentSimHash* SimhashRec;
    mutable ui32 PrevDocId;
};

TErfUrlFiller::TErfUrlFiller(const TErfCreateConfig& config, TErfsRemap& erfs)
    : Config(config)
    , Erfs(erfs)
{
    const SDocErfInfo3::TFieldMask fieldMask = config.GetErfFieldMask();
    if (fieldMask.UrlNGramsModel) {
        Handlers.push_back(TSimpleSharedPtr<IUrlHandler>(new TNgramProcessorHandler(Config)));
    }
    Handlers.push_back(TSimpleSharedPtr<IUrlHandler>(new TLastAccessProcessorHandler(Config)));
    Handlers.push_back(TSimpleSharedPtr<IUrlHandler>(new TUrlLength2Handler));
    Handlers.push_back(TSimpleSharedPtr<IUrlHandler>(new TNonLettersCounterHandler));

    HandlersAll.push_back(TSimpleSharedPtr<IUrlHandler>(new THubHandler(Config)));
    if (fieldMask.DocStaticSignature1 || fieldMask.DocStaticSignature2) {
        HandlersAll.push_back(TSimpleSharedPtr<IUrlHandler>(new TSimhashHandler(Config)));
    }
    HandlersAll.push_back(TSimpleSharedPtr<IUrlHandler>(new TIsIndexPageHandler));
    HandlersAll.push_back(TSimpleSharedPtr<IUrlHandler>(new TMinPathLenHandler(Config)));
    HandlersAll.push_back(TSimpleSharedPtr<IUrlHandler>(new TTotalDupsHandler(Config)));
    HandlersAll.push_back(TSimpleSharedPtr<IUrlHandler>(new THasMultimediaHandler));
    HandlersAll.push_back(TSimpleSharedPtr<IUrlHandler>(new TIsNotCgiHandler));
    HandlersAll.push_back(TSimpleSharedPtr<IUrlHandler>(new TRegionFromUrlHandler(Config)));
}

TErfUrlFiller::~TErfUrlFiller()
{
}

void TErfUrlFiller::ProcessUrls(void)
{
    TTimeLogger logger("process urls");
    TInDatFile<TUrlRec> urlFile(Config.Urls.data(), 4 << 20, 0);
    if (IsAccessible(Config, Config.Urls))
        urlFile.Open(Config.Urls.data());
    THolder<IRemapReader> invRemap(GetInvRemap(Config));

    ui32 docId = (ui32)-1;
    ui32 prevDocId = (ui32)-1;

    const TUrlRec* doc;
    TUrlRec::TExtInfo docExtInfo;
    while (doc = urlFile.Next()) {

        bool hasDocExtInfo = urlFile.GetExtInfo(&docExtInfo);

        if (!invRemap->Remap(doc->DocId, docId))
            continue;

        Y_ASSERT(docId < Erfs.Remap.size());
        SDocErfInfo3& erf = Erfs[docId];
        const char* docName = doc->Name + GetHttpPrefixSize(doc->Name); // https hack: drop scheme

        for (THandlers::const_iterator it = HandlersAll.begin(); it != HandlersAll.end(); ++it)
            (*it)->ProcessUrl(erf, doc, hasDocExtInfo ? &docExtInfo : nullptr, docName);

        if (docId == prevDocId)
            continue; //skip dublicates

        for (THandlers::const_iterator it = Handlers.begin(); it != Handlers.end(); ++it)
            (*it)->ProcessUrl(erf, doc, hasDocExtInfo ? &docExtInfo : nullptr, docName);

        prevDocId = docId;
    }

    logger.SetOK();
}

TRealtimeErfUrlFiller::TRealtimeErfUrlFiller()
{
    Handlers.push_back(TSimpleSharedPtr<IUrlHandler>(new TUrlLength2Handler));
    Handlers.push_back(TSimpleSharedPtr<IUrlHandler>(new TNonLettersCounterHandler));
    Handlers.push_back(TSimpleSharedPtr<IUrlHandler>(new TIsIndexPageHandler));
    Handlers.push_back(TSimpleSharedPtr<IUrlHandler>(new THasMultimediaHandler));
}

TRealtimeErfUrlFiller::~TRealtimeErfUrlFiller()
{
}

void TRealtimeErfUrlFiller::ProcessUrl(const TString& url, SDocErfInfo3& erf)
{
    for (THandlers::const_iterator it = Handlers.begin(); it != Handlers.end(); ++it)
        (*it)->ProcessUrl(erf, nullptr, nullptr, url.data());
}
