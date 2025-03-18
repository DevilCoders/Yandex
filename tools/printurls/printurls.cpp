// Читает из stdin список нормализованных url-ов,
// В стандартный выход пишет список информацию о тех url-ах из поданных на stdin, которые нашлись в базе
// в формате: (url, archive_docid, archive_url, index_docid, index_url)
// Префикс имени базы передаётся в качестве единственного аргумента командной строки

#include <kernel/doc_remap/remap_reader.h>
#include <kernel/doc_remap/url_dat_remapper.h>
#include <kernel/mirrors/mirrors.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/uri/norm/normalizer.h>

#include <quality/urllib/url_utils.h>

#include <search/meta/generic/msstring.h>

#include <ysite/yandex/common/prepattr.h>

#include <yweb/robot/dbscheeme/baserecords.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/getopt/opt.h>
#include <kernel/keyinv/indexfile/seqreader.h>
#include <library/cpp/microbdb/safeopen.h>

#include <util/charset/utf8.h>
#include <util/digest/numeric.h>
#include <util/folder/dirut.h>
#include <util/generic/buffer.h>
#include <util/generic/hash_set.h>
#include <util/generic/set.h>
#include <util/generic/yexception.h>
#include <util/string/printf.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <util/system/defaults.h>
#include <utility>

const size_t NORMALIZED_URL_BUFFER_SIZE = 10 * 1024;
#define PACK_CONST  1000000

struct TUrlInfo {
    TString Url;
    ui32 ArchiveDocId;
    TString ArchiveUrl;
    ui32 IndexDocId;
    TString IndexUrl;
    ui64 TrieIndex;
};

typedef std::pair<ui32, ui64> TDocIdentity;

struct TTDocIdentityHash {
    size_t operator() (const TDocIdentity &key) const {
        return (size_t)CombineHashes<ui64>(key.first, key.second);
    }
};

typedef THashSet<TDocIdentity, TTDocIdentityHash> TDocs;

void ReadUrlsFromStdin(THashSet<TString>* selectedUrls) {
    TString url;
    while (Cin.ReadLine(url)) {
        selectedUrls->insert(url);
    }
    Cerr << selectedUrls->size() << " urls loaded from stdin." << Endl;
}

class TArchiveUrlsFinderBase {
public:
    virtual bool IsSelectedUrl(const char* url, ui64* index) const noexcept = 0;

    void Find(const char* indexPrefix, TVector<TUrlInfo>* foundUrls) {
        bool hasFrqFile = false;
        TString frqPath = Sprintf("%sfrq", indexPrefix);
        TFileMappedArray<i16> frq;
        if (NFs::Exists(frqPath)) {
            frq.Init(frqPath.data());
            hasFrqFile = true;
        }

        TArchiveIterator arcIter;
        arcIter.Open(Sprintf("%sarc", indexPrefix).data());

        char normalizedUrlBuffer[NORMALIZED_URL_BUFFER_SIZE];

        for (TArchiveHeader* curHdr = arcIter.NextAuto(); curHdr; curHdr = arcIter.NextAuto())
            if (!hasFrqFile || frq[curHdr->DocId] != -1) {
                TBlob extInfo = arcIter.GetExtInfo(curHdr);
                TDocDescr docDescr;
                docDescr.UseBlob(extInfo.Data(), static_cast<unsigned>(extInfo.Size()));

                if (docDescr.IsAvailable()) {
                    if (NormalizeUrl(normalizedUrlBuffer, NORMALIZED_URL_BUFFER_SIZE, docDescr.get_url())) {
                        ui64 urlIndex = -1;
                        if (IsSelectedUrl(normalizedUrlBuffer, &urlIndex)) {
                            foundUrls->push_back(TUrlInfo());
                            TUrlInfo& urlInfo = foundUrls->back();
                            urlInfo.Url = normalizedUrlBuffer;
                            urlInfo.ArchiveDocId = curHdr->DocId;
                            urlInfo.ArchiveUrl = docDescr.get_url();
                            urlInfo.TrieIndex = urlIndex;
                        }
                    }
                }
            }
    }
    virtual ~TArchiveUrlsFinderBase() {}
};

class TArchiveUrlsHashFinder : public TArchiveUrlsFinderBase {
private:
    const THashSet<TString>* SelectedUrls;

public:
    TArchiveUrlsHashFinder(const THashSet<TString>* selectedUrls)
        : SelectedUrls(selectedUrls)
    {
    }

    bool IsSelectedUrl(const char* url, ui64* index) const noexcept override {
        *index = -1;
        return SelectedUrls->find(url) != SelectedUrls->end();
    }
};

class TArchiveUrlsHashSmartFinder : public TArchiveUrlsFinderBase {
private:
    THashSet<TString> SelectedUrls;
    Nydx::TUriNormalizer Normalizer;

public:
    TArchiveUrlsHashSmartFinder(const THashSet<TString>* selectedUrls) {
        for (THashSet<TString>::const_iterator url = selectedUrls->begin();
            url != selectedUrls->end();
            ++url)
        {
            TString normalizedUrl = Normalizer.NormalizeUrl(*url);
            SelectedUrls.insert(normalizedUrl);
        }
    }

    bool IsSelectedUrl(const char* url, ui64* index) const noexcept override {
        *index = -1;
        TString normalizedUrl = Normalizer.NormalizeUrl(url);
        return SelectedUrls.find(normalizedUrl) != SelectedUrls.end();
    }
};

class TArchiveUrlsPackedFinder : public TArchiveUrlsFinderBase {
private:
    TMappedFile UrlsMapFile;
    THolder< TCompactTrie<char> > UrlsTrie;

public:
    TArchiveUrlsPackedFinder(const TString& urlsFile)
        : UrlsMapFile(urlsFile.data())
    {
        UrlsTrie.Reset(new TCompactTrie<char>(reinterpret_cast<const char*>(UrlsMapFile.getData()), UrlsMapFile.getSize()));
    }

    bool IsSelectedUrl(const char* url, ui64* urlIndex) const noexcept override {
        if (!UrlsTrie->Find(url, strlen(url), urlIndex)) {
            *urlIndex = -1;
            return false;
        }

        return true;
    }
};

class TArchiveUrlsTrivialFinder : public TArchiveUrlsFinderBase {
public:
    TArchiveUrlsTrivialFinder()
    {
    }

    bool IsSelectedUrl(const char*, ui64*) const noexcept override {
        return true;
    }
};

void PrintPackedArchiveUrls(const TString& indexPrefix, const TString& urlsFile, bool printIndices, bool printSrcDocIds = false) {
    TVector<TUrlInfo> foundUrls;
    TArchiveUrlsPackedFinder finder(urlsFile);
    THolder<TRemapReader> invremap;
    finder.Find(indexPrefix.data(), &foundUrls);

    if (printSrcDocIds) {
        TString invremapFile = Sprintf("%siarr", indexPrefix.data());
        if (NFs::Exists(invremapFile)) {
            invremap.Reset(new TRemapReader(invremapFile.data()));
        }
    }

    for (TVector<TUrlInfo>::const_iterator it = foundUrls.begin(); it != foundUrls.end(); ++it) {
        Cout << it->Url << '\t' << it->ArchiveDocId;
        if (printSrcDocIds) {
            ui32 srcDocId;
            if (invremap.Get() && invremap->Remap(it->ArchiveDocId, srcDocId)) {
                Cout << '\t' << srcDocId;
            }
        }
        if (printIndices) {
            Cout << '\t' << it->TrieIndex;
        }
        Cout << '\n';
    }
}

static void ArchivePrintUrls(const char* indexPrefix, bool ignoreInput, bool smartUrlsNorm = false) {
    THashSet<TString> selectedUrls;

    THolder<TArchiveUrlsFinderBase> finder;
    if (!ignoreInput) {
        ReadUrlsFromStdin(&selectedUrls);
        if (smartUrlsNorm) {
            finder.Reset(new TArchiveUrlsHashSmartFinder(&selectedUrls));
        } else {
            finder.Reset(new TArchiveUrlsHashFinder(&selectedUrls));
        }
    } else {
        finder.Reset(new TArchiveUrlsTrivialFinder());
    }
    TVector<TUrlInfo> foundUrls;
    finder->Find(indexPrefix, &foundUrls);

    for (TVector<TUrlInfo>::const_iterator it = foundUrls.begin(); it != foundUrls.end(); ++it) {
        Cout << it->Url << '\t' << it->ArchiveDocId << '\n';
    }
}

static void NormalizeInputUrl(TString& url, Nydx::TUriNormalizer& normalizer) {
    TUtf16String wideUrl = CharToWide(url, CODES_UTF8);
    wideUrl = PrepareURL(wideUrl);
    url = WideToUTF8(wideUrl);
    url = normalizer.NormalizeUrl(url);
}

static void LowerUrl(TString& url, ECharset inputCharset) {
    TUtf16String wideUrl = CharToWide(url, inputCharset);
    wideUrl.to_lower();
    url = WideToUTF8(wideUrl);
}

static void PrintUrlsFromIndexKeys(const char* indexPrefix, bool ignoreInput,
                                   bool smartUrlsNorm, bool lowerUrl) {
    Nydx::TUriNormalizer normalizer;
    long inputUrlsTotal = 0;
    THashMap<TString,TString> normUrls;
    if (!ignoreInput) {
        THashSet<TString> inputUrls;
        ReadUrlsFromStdin(&inputUrls);
        inputUrlsTotal = inputUrls.size();
        for (THashSet<TString>::iterator it = inputUrls.begin(); it != inputUrls.end(); ++it) {
            TString url(CutHttpPrefix(*it));

            if (smartUrlsNorm) {
                NormalizeInputUrl(url, normalizer);
            }

            if (lowerUrl) {
                LowerUrl(url, CODES_UTF8);
            }

            normUrls[url] = *it;
        }
    }
    else {
        Cerr << "Printing all urls" << Endl;
    }

    THashMap<TString,ui64> foundUrls;
    ui64 collisions = 0;
    Cerr << "Reading index keys" << Endl;
    TSequentYandReader UrlReader;
    TString urlKeyPrefix = "#url=\"";
    for (UrlReader.Init(indexPrefix, urlKeyPrefix.data()); UrlReader.Valid(); UrlReader.Next()) {
        TString url = UrlReader.CurKey().Text + urlKeyPrefix.length();

        if (smartUrlsNorm) {
            url = normalizer.NormalizeUrl(url);
        }

        if (lowerUrl) {
            LowerUrl(url, CODES_YANDEX);
        }

        if (ignoreInput || normUrls.contains(url)) {
            TString origUrl = ignoreInput ? url : normUrls[url];
            for (TSequentPosIterator it(UrlReader); it.Valid(); ++it) {
                Cout << origUrl << '\t' << it.Doc() << Endl;
                if (foundUrls.contains(url) && foundUrls[url] != it.Doc()) {
                    ++collisions;
                }
                else {
                    foundUrls[url] = it.Doc();
                }
            }
        }
    }

    int percFound = (ignoreInput || inputUrlsTotal == 0) ? 100 : 100*foundUrls.size()/inputUrlsTotal;
    Cerr << foundUrls.size() << " urls found [" << percFound << "%]" << Endl;
    if (collisions > 0) {
        Cerr << "WARN: " << collisions << " url -> doc_id map collisions found" << Endl;
    }
}

static void PrintUrls(const char* indexPrefix) {
    // Читаем все url-ы из stdin в память
    THashSet<TString> selectedUrls;
    ReadUrlsFromStdin(&selectedUrls);

    bool hasArrFile = false;
    TString iarrPath = Sprintf("%siarr", indexPrefix);
    TFileMappedArray<ui32> iarr;
    if (NFs::Exists(iarrPath)) {
        iarr.Init(iarrPath.data());
        hasArrFile = true;
        Cerr << "Using " << indexPrefix << "iarr" << Endl;
    }

    TArchiveUrlsHashFinder finder(&selectedUrls);
    TVector<TUrlInfo> foundUrls;
    finder.Find(indexPrefix, &foundUrls);
    Cerr << foundUrls.size() << " urls found [" << 100*foundUrls.size()/selectedUrls.size() << "]" << Endl;
    selectedUrls.clear();

    for (TVector<TUrlInfo>::iterator it = foundUrls.begin(); it != foundUrls.end(); ++it) {
        it->IndexDocId = hasArrFile ? iarr[it->ArchiveDocId] : it->ArchiveDocId;
    }

    // Строим карту соответствия индексного docid месту, куда нужно записать его url
    std::map<ui32, TString> indexDocid2IndexUrl;
    for (size_t i = 0; i < foundUrls.size(); i++) {
        TUrlInfo& urlInfo = foundUrls[i];
        indexDocid2IndexUrl[urlInfo.IndexDocId] = urlInfo.IndexUrl;
    }

    Cerr << "Reading index keys" << Endl;
    // Прочитаем в индексе все ключи, начинающиеся с #url, вытащим из них docid,
    // и если это интересущий нас docid, то запомним его url
    TSequentYandReader UrlReader;
    UrlReader.Init(indexPrefix, "#url=\"");
    while (UrlReader.Valid()) {
        TSequentPosIterator it(UrlReader);
        while (it.Valid()) {
            const ui32 indexDocId = it.Doc();
            std::map<ui32, TString>::iterator iter = indexDocid2IndexUrl.find(indexDocId);
            if (iter != indexDocid2IndexUrl.end()) {
                iter->second = UrlReader.CurKey().Text + 6;
            }
            ++it;
        }
        UrlReader.Next();
    }

    // Распечатаем результаты
    for (size_t i = 0; i < foundUrls.size(); i++) {
        const TUrlInfo& urlInfo = foundUrls[i];
        Cout << urlInfo.Url << '\t'
            << urlInfo.ArchiveDocId << '\t'
            << urlInfo.ArchiveUrl << '\t'
            << urlInfo.IndexDocId << '\t'
            << urlInfo.IndexUrl << '\n';
    }
}

class THashHasUrlSmart : public IHasUrl {
    TUrls Urls;
    typedef THashMap<TString, TIndexPositions> TUrl2Positions;
    TUrl2Positions Url2Positions;
    size_t Size;
    Nydx::TUriNormalizer Normalizer_;

public:
    THashHasUrlSmart(const TUrls& urls)
        : Urls(urls)
    {
        for (size_t i = 0; i < Urls.size(); ++i) {
            TString url = Normalizer_.NormalizeUrl(Urls[i]);
            Url2Positions[url].push_back(i);
        }
        Size = Urls.ysize();
    }

    bool HasUrl(const TString& url, const TIndexPositions** positions) const override {
        TString nurl = Normalizer_.NormalizeUrl(url);

        TUrl2Positions::const_iterator toUrl = Url2Positions.find(nurl);
        if (toUrl != Url2Positions.end()) {
            *positions = &toUrl->second;
            return true;
        } else {
            return false;
        }
    }

    size_t GetSize() const override {
        return Size;
    }

    TString GetUrl(size_t index) const override {
        return Urls[index];
    }
};

void SimplePrintUrls(const char* indexPrefix, bool useMirrors, bool extraData, bool smartNormalization = false) {
    Nydx::TUriNormalizer Normalizer_;
    TString line;
    bool read = true;

    mirrors mrs;
    if (useMirrors) {
        TString mirrFile;
        sprintf(mirrFile, "%s/mirrors.res", indexPrefix);
        mrs.load(mirrFile.c_str(), false);
    }

    while (read) {
        TUrls urls;
        TVector<ui32> exData;
        TUrls resolvedUrls;

        int i = 0;
        for(; (i < PACK_CONST) && Cin.ReadLine(line); i++) {
            TString url = line;

            if (extraData) {
                size_t pos = line.find('\t');
                if (pos == TString::npos) {
                    ythrow yexception() << "Input string should contain 2 fields: " <<  line.data();
                }
                ui32 data = FromString<ui32>(line.data() + pos + 1);
                url = line.substr(0, pos);
                exData.push_back(data);
            }

            urls.push_back(url);
        }

        if (useMirrors) {
            for(size_t j = 0; j < urls.size(); j++) {
                TString l;
                if (smartNormalization) {
                    l = Normalizer_.NormalizeUrl(urls[j]);
                } else {
                    l = NormalizeUrl(urls[j]);
                }
                if (l.size() < 4) {
                    resolvedUrls.push_back("");
                } else {
                    UnmirrorUrl(mrs, l);
                    resolvedUrls.push_back(l);
                }
            }
        }

        if (i < PACK_CONST) {
            read = false;
        }

        TDocIds docIds;
        if (smartNormalization) {
            THashHasUrlSmart hashHas(urls);
            Urls2DocIds(indexPrefix, hashHas, &docIds);
        } else if (!useMirrors) {
            Urls2DocIds(indexPrefix, urls, &docIds, true);
        } else {
            Urls2DocIds(indexPrefix, resolvedUrls, &docIds, true);
        }

        THolder<TInvRemapReader> remapReader;
        TString remapFilename = Sprintf("%s/indexarr", indexPrefix);
        if (NFs::Exists(remapFilename)) {
            remapReader.Reset(new TInvRemapReader(remapFilename.data()));
        }

        for (size_t j = 0; j < urls.size(); ++j) {
            if (docIds[j] != (ui32)-1) {
                if (extraData) {
                    Cout << urls[j] << "\t" << exData[j] << Endl;
                } else {
                    ui32 arcDocId = docIds[j];
                    ui32 srcDocId = arcDocId;
                    //Ignore url, which have archive docid and don't have search docid - may be it could be spam
                    if (!remapReader || remapReader->Remap(arcDocId, srcDocId)) {
                        Cout << urls[j] << "\t" << srcDocId << "\t" << arcDocId << Endl;
                    }
                }
            }
        }
    }
}

void PrintPackedUrls(const TString& index,
    const TString& urlsFile,
    bool printHelpData = false,
    bool printIndexes = false,
    bool lowerHost = false,
    bool printOrig = false,
    bool smartUrlsNorm = false)
{
    TMappedFile urlsMap(urlsFile);
    Nydx::TUriNormalizer Normalizer_;

    TString urldatFile = index + "/url.dat";
    TInDatFile<TUrlRec> urldat(urldatFile.data(), 1);
    urldat.Open(urldatFile.data());

    THolder<TRemapReader> invremap;
    TString invremapFile = Sprintf("%s/indexiarr", index.data());
    if (NFs::Exists(invremapFile)) {
        invremap.Reset(new TRemapReader(invremapFile.data()));
    }

    TDocs docids;
    while (const TUrlRec* url = urldat.Next()) {
        TString normalized;
        if (smartUrlsNorm) {
            normalized = Normalizer_.NormalizeUrl(url->Name);
        } else {
            normalized = AddSchemePrefix(NormalizeUrl(url->Name));
        }
        TString toPrint = normalized;
        if (printOrig)
            toPrint = url->Name;

        if (lowerHost)
            normalized.to_lower(0,GetHost(normalized).size());

        ui64 urlIndex;

        if (TCompactTrie<char>(reinterpret_cast<const char*>(urlsMap.getData()), urlsMap.getSize()).Find(normalized.data(), normalized.size(), &urlIndex)) {
            TDocs::const_iterator iter = docids.find(TDocIdentity(url->DocId, urlIndex));
            if (iter == docids.end()) {

                ui32 arcid = url->DocId;
                ui32 srcid = arcid;

                if (invremap.Get()) {
                    if (!invremap->Remap(arcid, srcid)) {
                        continue;
                    }
                }

                Cout << toPrint << "\t" << srcid << "\t" << arcid;

                if (printIndexes) {
                    Cout << "\t" << urlIndex;
                }

                if (printHelpData) {
                    Cout << "\t" << url->ModTime << "\t" << (int)url->Status << "\t" << url->HttpCode;
                }
                Cout << "\n";

            }
            docids.insert(TDocIdentity(url->DocId, urlIndex));
        }
    }
}

struct TUrlWithData {
    TString Url;
    ui64 Data;

    inline bool operator < (const TUrlWithData& otherUrlWithData) const {
        int result = compare(Url, otherUrlWithData.Url);
        if (result != 0) {
            return result < 0;
        }

        return Data < otherUrlWithData.Data;
    }
};

typedef TVector<TUrlWithData> TUrlsWithData;

void PackUrls(const char* urlsPath, const char* packedUrlsPath, bool extraData, bool originalUrls = false, bool smartUrlsNorm = false) {
    Nydx::TUriNormalizer Normalizer_;
    TFileInput urlsFile(urlsPath);
    TUrlsWithData urls;
    TString url;
    ui64 index = 0;

    while (urlsFile.ReadLine(url)) {
        if (!url.size()) {
            continue;
        }

        ui64 data = index++;
        if (extraData) {
            size_t pos = url.find('\t');
            if (pos == TString::npos) {
                ythrow yexception() << "Input string should contain 2 fields: " <<  url.data();
            }
            data = FromString<ui64>(url.data() + pos + 1);
            url = url.substr(0, pos);
        }

        TString normUrl = "";
        if (originalUrls) {
            normUrl = url;
        } else {
            if (smartUrlsNorm) {
                normUrl = Normalizer_.NormalizeUrl(url);
            } else {
                normUrl = AddSchemePrefix(NormalizeUrl(url));
            }
        }

        TUrlWithData urlWithData = {normUrl, data};
        urls.push_back(urlWithData);
    }

    if (!urls.empty()) {
        std::sort(urls.begin(), urls.end());

        TUrlsWithData::iterator it1 = urls.begin() + 1;
        TUrlsWithData::iterator it2 = urls.begin();
        while (it1 != urls.end()) {
            if (it1->Url == it2->Url) {
                ++it1;
            } else {
                *++it2 = *it1++;
            }
        }

        urls.erase(it2 + 1, urls.end());
    }

    TCompactTrieBuilder<char> builder(CTBF_PREFIX_GROUPED);
    for (TUrlsWithData::const_iterator it = urls.begin(); it != urls.end(); ++it) {
        builder.Add(it->Url.data(), it->Url.size(), it->Data);
    }

    TBufferOutput raw;
    builder.Save(raw);

    TFixedBufferFileOutput packedUrlsFile(packedUrlsPath);
    CompactTrieMinimize<TCompactTriePacker<ui64> >(packedUrlsFile, raw.Buffer().Data(), raw.Buffer().Size(), false);
}

void PrintUsage() {
    Cerr << "printurls [-a [-A]|-s|-m|-x|-n] index < urls" << Endl;
    Cerr << "printurls [-dl] -p index urls" << Endl;
    Cerr << "printurls [-dl] -p [-x] index urls" << Endl;
    Cerr << "printurls -u [-x] index urls" << Endl;
    Cerr << "printurls -U [-x] index urls" << Endl;
    Cerr << "printurls -c [-x] urls packed_urls" << Endl;
    Cerr << Endl;
    Cerr << "-d - print additional info: modifTime, urlStatus, extHttpCode " << Endl;
    Cerr << "-n - smart urls normalization" << Endl;
    Cerr << "-s - use url.dat for retrieving docids" << Endl;
    Cerr << "-A - print all urls. ignore input data" << Endl;
    Cerr << "-a - use archive for retrieving docids and print only archive docid" << Endl;
    Cerr << "-m - use mirrors resolver for printing urls, only with 's' option" << Endl;
    Cerr << "-p - print urls using url.dat and packed urls list" << Endl;
    Cerr << "-l - lower host before search (for -p option only)" << Endl;
    Cerr << "-u - print urls using archive and packed urls list" << Endl;
    Cerr << "-U - print urls using archive, packed urls list and url.dat" << Endl;
    Cerr << "-i - print urls from index keys" << Endl;
    Cerr << "-L - lower the whole url before search (for -i option only)" << Endl;
    Cerr << "-x - add or print extra data" << Endl;
    Cerr << "-c - create packed urls list" << Endl;
    Cerr << "-o - print or pack original, not normalized urls" << Endl;
}

int main(int argc, char** argv) {
    Opt opt(argc, argv, "AasmplcduUxoniL");
    int optlet;

    bool simple = false;
    bool archive = false;
    bool useMirrors = false;
    bool printFromPacked = false;
    bool printFromPackedArchive = false;
    bool printFromIndexKeys = false;
    bool createPacked = false;
    bool printModifData = false;
    bool printSrcDocIds = false;
    bool extraData = false;
    bool lowerHost = false;
    bool lowerUrl = false;
    bool originalUrls = false;
    bool ignoreInput = false;
    bool smartUrlsNorm = false;

    while ((optlet = opt()) != EOF) {
        switch (optlet) {
            case 'n':
                smartUrlsNorm = true;
                break;
            case 'a':
                archive = true;
                break;
            case 'A':
                ignoreInput = true;
                break;
            case 's':
                simple = true;
                break;
            case 'm' :
                useMirrors = true;
                break;
            case 'c':
                createPacked = true;
                break;
            case 'p':
                printFromPacked = true;
                break;
            case 'l':
                lowerHost = true;
                break;
            case 'L':
                lowerUrl = true;
                break;
            case 'u':
                printFromPackedArchive = true;
                break;
            case 'd':
                printModifData = true;
                break;
            case 'U':
                printFromPackedArchive = true;
                printSrcDocIds = true;
                break;
            case 'x':
                extraData = true;
                break;
            case 'o':
                originalUrls = true;
                break;
            case 'i':
                printFromIndexKeys = true;
                break;
            default:
                PrintUsage();
                return 1;
        }
    }

    argc -= opt.Ind;
    argv += opt.Ind;

    const char* const indexPrefix = argv[0];

    if (printFromPacked || printFromPackedArchive || createPacked) {
        if (argc < 2) {
            PrintUsage();
            return 1;
        }

        if (printFromPacked) {
            const char* urlsFile = argv[1];
            PrintPackedUrls(indexPrefix, urlsFile, printModifData, extraData, lowerHost, originalUrls, smartUrlsNorm);

        } else if (printFromPackedArchive) {
            const char* urlsFile = argv[1];
            PrintPackedArchiveUrls(indexPrefix, urlsFile, extraData, printSrcDocIds);

        } else if (createPacked) {
            const char* urlsFile = argv[0];
            const char* packedUrlsFile = argv[1];
            PackUrls(urlsFile, packedUrlsFile, extraData, originalUrls, smartUrlsNorm);
        }
    } else {
        if (argc != 1) {
            PrintUsage();
            return 1;
        }

        if (archive) {
            ArchivePrintUrls(indexPrefix, ignoreInput, smartUrlsNorm);
        } else if (simple) {
            SimplePrintUrls(indexPrefix, useMirrors, extraData, smartUrlsNorm);
        } else if (printFromIndexKeys) {
            PrintUrlsFromIndexKeys(indexPrefix, ignoreInput, smartUrlsNorm, lowerUrl);
        } else {
            PrintUrls(indexPrefix);
        }
    }

    return 0;
}
