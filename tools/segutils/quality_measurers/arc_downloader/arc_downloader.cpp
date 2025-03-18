#include <tools/segutils/segcommon/data_utils.h>

#include <kernel/segutils/numerator_utils.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/tarc/markup_zones/arcreader.h>

#include <library/cpp/getopt/opt2.h>
#include <library/cpp/lcs/lcs_via_lis.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <util/digest/city.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/zlib.h>
#include <util/string/strip.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>

// n threads
// sample size
// source arc
// target dir
// target mapping

// go through arc
// count urls
// for each url roll the dice
// collect downloaded urls in hash, count successfull samples
// if insufficient successfull samples redo ignoring used in previous attempts
// if having enough samples or out of urls exit

namespace NSegutils {

struct TFileSaver {
    TString FilesDir;
    TString FilesTemplate;
    TString FilesMapping;

    THolder<TUnbufferedFileOutput> Mapping;

    TMutex SaveMutex;

    ui64 Count;

    TFileSaver()
        : Count()
    {}

    bool Valid() {
        return !!FilesDir && FilesTemplate.find("%u") != TString::npos && !!FilesMapping;
    }

    void Init() {
        Mapping.Reset(new TUnbufferedFileOutput(FilesMapping));
    }

    void Save(TStringBuf mappingline, TStringBuf filecontent) {
        bool compress = FilesTemplate.EndsWith(".gz");
        TFsPath file;

        {
            TGuard<TMutex> guard(SaveMutex);

            TString fname = Sprintf(FilesTemplate.data(), Count++);
            file = TFsPath(FilesDir) / fname;

            {
                *Mapping << fname << "\t" << mappingline << "\n";
                Clog << fname << "\t" << mappingline << Endl;
            }
        }

        {
            TUnbufferedFileOutput fout(file.c_str());

            if (compress) {
                TZLibCompress c(&fout, ZLib::GZip);
                c << filecontent;
            } else {
                fout << filecontent;
            }
        }
    }

    static TFileSaver& Get() {
        return *Singleton<TFileSaver>();
    }
};

struct THasher {
    NSegm::THashValues* Hashes;

    THasher()
        : Hashes()
    {}

    void SetHashes(NSegm::THashValues* h) {
        Hashes = h;
    }

    void ProcessToken(const TWideToken& t) {
        Hashes->push_back(CityHash64((const char*)t.Token, t.Leng * sizeof(TChar)));
    }

    virtual ~THasher() {}
};

struct THashingTokenizer : public THasher, public ITokenHandler {
    void OnToken(const TWideToken& token, size_t, NLP_TYPE type) override {
        if (EqualToOneOf(type, NLP_WORD, NLP_MARK, NLP_INTEGER, NLP_FLOAT))
            ProcessToken(token);
    }
};

struct THashingNumerator : public THasher, public INumeratorHandler {
    void OnTokenStart(const TWideToken& token, const TNumerStat&) override {
        ProcessToken(token);
    }
};

struct TTaskContext {
    TParserContext ParserCtx;

    TUtf16String Sample;

    NSegm::THashValues SampleHashes;
    NSegm::THashValues DocHashes;

    THtmlDocument Document;

    ui32 MinWords;
    float Threshold;

    explicit TTaskContext(TStringBuf confdir, ui32 minwords, float thrsh = 0.75)
        : ParserCtx(confdir)
        , MinWords(minwords)
        , Threshold(thrsh)
    {
    }

    bool Try(TStringBuf url, TDateTime time, TStringBuf sample) {
        Clear();

        Document.Url = url;
        Document.Time = time;

        try {

            {
                Fetch(Document);

                THashingNumerator num;
                num.SetHashes(&DocHashes);
                ParserCtx.NumerateDocument(&num, Document);
            }

            {
                THashingTokenizer tok;

                UTF8ToWide<true>(sample, Sample);
                tok.SetHashes(&SampleHashes);
                {
                    TNlpTokenizer t(tok);
                    t.Tokenize(Sample);
                }
            }

            if (SampleHashes.size() < MinWords)
                return false;

            float res = NLCS::MeasureLCS<size_t>(DocHashes, SampleHashes);

            return SampleHashes.empty() || res / SampleHashes.size() > Threshold;
        } catch (const yexception& e) {
            Clog << "Failed to process '" << Document.Url << "' : " << e.what() << Endl;
            return false;
        }
    }

    TString GetMappingLine() const {
        return Sprintf("%s\t%s", Document.Url.data(), Document.Time.Date.ToString("%d/%m/%Y").data());
    }

    TStringBuf GetFileContent() const {
        return Document.Html;
    }

    void Clear() {
        Sample.clear();
        SampleHashes.clear();
        DocHashes.clear();
        Document.Clear();
    }
};

struct TResultCounters {
    TAtomicCounter Success;
    TAtomicCounter Failure;
};

struct TDloadTask : public IObjectInQueue {
    TString Url;
    TString Sample;
    TDateTime Time;

    TDloadTask(TString url, TString sample, TDateTime time)
        : Url(url)
        , Sample(sample)
        , Time(time)
    {}

    void Process(void* res) override {
        TTaskContext* ctx = (TTaskContext*)res;

        if (ctx->Try(Url, Time, Sample)) {
            TFileSaver::Get().Save(ctx->GetMappingLine(), ctx->GetFileContent());
        }

        delete this;
    }
};

struct TDloadSettings {
    ui32 DloadThreads;
    ui32 MinWords;

    TString ConfigDir;
    TString SourcePath;

    TDloadSettings(int argc, const char** argv)
        : DloadThreads()
        , MinWords()
    {
        Init(argc, argv);
    }

    TFileSaver& FSaver() {
        return TFileSaver::Get();
    }

    void Init(int argc, const char** argv) {
        Opt2 opt(argc, (char*const*)argv, "d:c:s:f:m:t:w:");
        try {
            DloadThreads = FromString<ui32>(
                            opt.Arg('d', " <thr> - download threads ", "16"));
            ConfigDir =     opt.Arg('c', " <config dir> - dir with config for parser and dict for recognizer ", "", true);
            SourcePath =    opt.Arg('s', " <input> - input to read samples from ", "", true);
            FSaver().FilesDir =
                            opt.Arg('f', " <filedir> - dir to write downloaded files to ", "", true);
            FSaver().FilesMapping =
                            opt.Arg('m', " <mapping> - file to write metainformation about downloaded ", "", true);
            FSaver().FilesTemplate =
                            opt.Arg('t', " <fname template> - template for filename ", "%u.gz");
            MinWords = FromString<ui32>(
                            opt.Arg('w', " <min words in sample> ", "10", true));

            if (argc > 1 && !strcmp("--help", argv[1])) {
                opt.AutoUsage();
                exit(0);
            }

        } catch (const yexception&) {
            opt.AutoUsage();
            exit(1);
        }

        if (!DloadThreads || !SourcePath || !ConfigDir || !FSaver().Valid()) {
            opt.AutoUsage();
            exit(1);
        }

        FSaver().Init();
    }
};

class TDloadQueue : public TThreadPool {
    const TDloadSettings* Settings;
public:
    TDloadQueue(const TDloadSettings* sets)
        : Settings(sets)
    {
        Start(sets->DloadThreads, sets->DloadThreads * 30);
    }

    void* CreateThreadSpecificResource() override {
        return new TTaskContext(Settings->ConfigDir, Settings->MinWords);
    }

    void DestroyThreadSpecificResource(void* resource) override {
        delete (TTaskContext*)resource;
    }
};

void ProcessArchive(const TDloadSettings* settings) {
    typedef THashSet<TString> TRequestedUrls;
    TRequestedUrls urls;

    TString line;
    while (Cin.ReadLine(line)) {
        StripInPlace(line);
        if (!!line)
            urls.insert(AddSchemePrefix(line));
    }

    typedef THashMap<TString, ui32> TDocMapping;
    TDocMapping mapping;

    {
        TArchiveIterator arcIter;
        arcIter.Open(settings->SourcePath.data());

        for (const TArchiveHeader* curHdr = arcIter.NextAuto();
                        curHdr;
                        curHdr = arcIter.NextAuto()) {
            TBlob extInfo = arcIter.GetExtInfo(curHdr);
            TDocDescr docDescr;
            docDescr.UseBlob(extInfo.Data(), (unsigned int) extInfo.Size());

            TString url = AddSchemePrefix(docDescr.get_url());

            if (urls.contains(url))
                mapping[url] = curHdr->DocId;
        }
    }

    typedef TVector<std::pair<ui32, const TString*> > TIds;
    TIds ids;

    for (TDocMapping::const_iterator it = mapping.begin(); it != mapping.end(); ++it)
        ids.push_back(std::make_pair(it->second, &it->first));

    Sort(ids.begin(), ids.end());

    {
        TArchiveIterator arcIter;
        arcIter.Open(settings->SourcePath.data());
        TDloadQueue queue(settings);
        TCondVar var;
        TMutex m;
        const TArchiveHeader* curHdr = nullptr;
        for (TIds::const_iterator it = ids.begin(); it != ids.end(); ++it) {
            while (curHdr = arcIter.NextAuto())
                if (it->first <= curHdr->DocId)
                    break;

            if (!curHdr)
                break;

            if (it->first != curHdr->DocId)
                continue;

            {
                TString text;
                TDateTime dt;

                {
                    TBlob extInfo = arcIter.GetExtInfo(curHdr);
                    TDocDescr docDescr;
                    docDescr.UseBlob(extInfo.Data(), (unsigned int) extInfo.Size());

                    TBlob doctext = arcIter.GetDocText(curHdr);
                    TStringOutput sout(text);
                    PrintDocText(sout, doctext);

                    dt.SetTimestamp(docDescr.get_mtime());
                }

                TDloadTask* t = new TDloadTask(*it->second, text, dt);

                while (!queue.Add(t))
                    var.WaitD(m, TInstant::Seconds(5));
            }
        }
    }
}

}

int main(int argc, const char** argv) {
    NSegutils::TDloadSettings settings(argc, argv);
    NSegutils::ProcessArchive(&settings);
}
