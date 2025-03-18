#include <tools/segutils/segcommon/data_utils.h>

#include <kernel/indexer/dtcreator/dtcreator.h>
#include <kernel/indexer/dtcreator/dthandler.h>
#include <kernel/indexer/faceproc/docattrinserter.h>
#include <kernel/indexer/tfproc/forumlib/forums.h>
#include <kernel/recshell/recshell.h>
#include <kernel/segutils/numerator_utils.h>
#include <kernel/tarc/iface/farcface.h>
#include <kernel/tarc/markup_zones/text_markup.h>

#include <ysite/directtext/textarchive/createarc.h>

#include <library/cpp/getopt/opt2.h>
#include <library/cpp/html/pdoc/pds.h>

#include <util/charset/wide.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>

using namespace NSegutils;

template<typename Char>
struct TNewlineEscaper
{
    TBasicStringBuf<Char> Data;
    TNewlineEscaper(const TBasicStringBuf<Char>& d)
        : Data(d)
    {}
    TNewlineEscaper(const Char* begin, size_t len)
        : Data(begin, len)
    {}
};

template<typename Char>
static IOutputStream& operator<<(IOutputStream& to, const TNewlineEscaper<Char>& what)
{
    TBasicStringBuf<Char> data = what.Data;
    static const Char toEscape[] = {'\n', '\r', '\\'};
    size_t pos = 0;
    for (;;) {
        size_t nextPos = data.find_first_of(TBasicStringBuf<Char>(toEscape, Y_ARRAY_SIZE(toEscape)), pos);
        if (nextPos == TWtringBuf::npos)
            break;
        to << data.SubStr(pos, nextPos - pos);
        switch (data[nextPos]) {
        case '\n':
            to << "\\n";
            break;
        case '\r':
            to << "\\r";
            break;
        case '\\':
            to << "\\\\";
            break;
        default:
            Y_ASSERT(0);
        }
        pos = nextPos + 1;
    }
    to << data.SubStr(pos);
    return to;
}

class TFakeInserter : public IDocumentDataInserter
{
public:
    void StoreLiteralAttr(const char*, const char*, TPosting) override
    {}
    void StoreLiteralAttr(const char*, const wchar16*, size_t, TPosting) override
    {}
    void StoreDateTimeAttr(const char*, time_t) override
    {}
    void StoreIntegerAttr(const char*, const char*, TPosting) override
    {}
    void StoreKey(const char*, TPosting) override
    {}
    void StoreZone(const char* zoneName, TPosting begin, TPosting end, bool /*archiveOnly*/) override
    {
        Cout << "\t" << zoneName << " at " << FormatPosting(begin) << " - " << FormatPosting(end) << "\n";
    }
    void StoreArchiveZoneAttr(const char* name, const wchar16* value, size_t length, TPosting /*pos*/) override
    {
        Cout << "\t\t" << name << " = " << TNewlineEscaper<wchar16>(value, length) << "\n";
    }
    void StoreLemma(const wchar16*, size_t, const wchar16*, size_t, ui8, TPosting, ELanguage) override
    {}
    void StoreTextArchiveDocAttr(const TString& name, const TString& value) override
    {
        Cout << "[document attribute]\t" << name << " = " << TNewlineEscaper<char>(value) << Endl;
    }
    void StoreFullArchiveDocAttr(const TString&, const TString&) override
    {}
    void StoreErfDocAttr(const TString&, const TString&) override
    {}
    void StoreGrpDocAttr(const TString&, const TString&, bool) override
    {}
};

class TFakeInserterVerbose : public TFakeInserter
{
    TVector<TArchiveSent> Sentences;
    TArchiveMarkupZones Zones;
    typedef TVector<TArchiveZoneSpan>::const_iterator TArchiveZoneSpanIter;
    TMap<EArchiveZone, TArchiveZoneSpanIter> ArchiveZoneSpanIters;
public:
    TFakeInserterVerbose(const TArchiveHeader* arcHeader)
    {
        const TBlob docText = GetArchiveDocText(arcHeader);
        GetSentencesByNumbers((const ui8*)docText.Data(), TVector<int>(), &Sentences, nullptr, false);
        GetArchiveMarkupZones((const ui8*)docText.Data(), &Zones);
    }
    void StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly) override
    {
        TFakeInserter::StoreZone(zoneName, begin, end, archiveOnly);

        EArchiveZone zoneId = FromString(zoneName);
        if (!ArchiveZoneSpanIters.contains(zoneId))
            ArchiveZoneSpanIters[zoneId] = Zones.GetZone(zoneId).Spans.begin();

        if (ArchiveZoneSpanIters[zoneId] == Zones.GetZone(zoneId).Spans.end()) {
            return;
        }

        const TArchiveZoneSpan& span = *ArchiveZoneSpanIters[zoneId];

        Cout << "\"\"\"\n";
        for (size_t s = span.SentBeg - 1; s < Sentences.size() && s < span.SentEnd; ++s) {
            size_t pos = 0;
            size_t len = (size_t)-1;
            if (s + 1 == span.SentBeg) {
                pos = span.OffsetBeg;
            }
            if (s + 1 == span.SentEnd) {
                len = span.OffsetEnd - pos;
            }
            const TUtf16String& text = Sentences[s].OnlyText;
            Cout << WideToUTF8(text.c_str() + pos, Min<size_t>(text.size() - pos, len)) << "\n";
        }
        Cout << "\"\"\"\n";

        ++ArchiveZoneSpanIters[zoneId];
    }
};

class TWorkers
{
    THtProcessor HtProcessor;
    THolder<TRecognizerShell> Recognizer;
    THolder<NIndexerCore::TDirectTextCreator> DTCreator;
    THolder<TFixedBufferFileOutput> TarcFile;
    THolder<TArchiveCreator> TarcCreator;
    TBuffer TarcBuffer;
    THolder<TBufferOutput> TarcOutput;
public:
    TWorkers()
        : HtProcessor()
    {
    }
    void ConfigureParser(const char* filename)
    {
        HtProcessor.Configure(filename);
    }
    void InitRecognizer(const char* dictfile)
    {
        Recognizer.Reset(new TRecognizerShell(dictfile));
    }
    void InitTextArchiveWriter(const char* filename)
    {
        TarcFile.Reset(new TFixedBufferFileOutput(filename));
        WriteTextArchiveHeader(*TarcFile.Get());
        TarcCreator.Reset(new TArchiveCreator(*TarcFile, TArchiveCreator::WriteBlob));
        NIndexerCore::TDTCreatorConfig dtcCfg;
        DTCreator.Reset(new NIndexerCore::TDirectTextCreator(dtcCfg, TLangMask(LI_BASIC_LANGUAGES), LANG_UNK));
    }
    void InitTextArchiveWriterVerbose()
    {
        TarcOutput.Reset(new TBufferOutput(TarcBuffer));
        TarcCreator.Reset(new TArchiveCreator(*TarcOutput));
        NIndexerCore::TDTCreatorConfig dtcCfg;
        DTCreator.Reset(new NIndexerCore::TDirectTextCreator(dtcCfg, TLangMask(LI_BASIC_LANGUAGES), LANG_UNK));
    }
    void ProcessDoc(const THtmlDocument& f, int docId);
};

void TWorkers::ProcessDoc(const THtmlDocument& f, int docId)
{
    TAutoPtr<IParsedDocProperties> props(HtProcessor.ParseHtml(f.Html.data(), f.Html.size(), f.Url.data()));
    ECharset charset = f.ForcedCharset;
    ELanguage primaryLang = LanguageByName(f.ForcedLanguage);
    if (charset == CODES_UNKNOWN) {
        if (!Recognizer)
            ythrow yexception() << "unknown charset and no recognizer";
        Recognizer->Recognize(HtProcessor.GetStorage().Begin(), HtProcessor.GetStorage().End(),
            &charset, &primaryLang, nullptr, TRecognizerShell::THints());
    }
    props->SetProperty(PP_CHARSET, NameByCharset(charset));

    TNumeratorHandlers Handlers;
    TForumsHandler ForumsHandler;
    THolder<NIndexerCore::TDirectTextHandler> TarcHandler;
    Handlers.AddHandler(&ForumsHandler);
    if (!!DTCreator) {
        TarcHandler.Reset(new NIndexerCore::TDirectTextHandler(*DTCreator));
        Handlers.AddHandler(TarcHandler.Get());
        TarcOutput->Buffer().Clear();
    }

    ForumsHandler.OnAddDoc(f.Url.data(), f.Time.Timestamp, primaryLang);
    if (!!DTCreator)
        DTCreator->AddDoc(docId, primaryLang);
    HtProcessor.NumerateHtml(Handlers, props.Get());
    if (!!DTCreator) {
        DTCreator->CommitDoc();
        TFullDocAttrs docAttrs;
        TDocAttrInserter inserter(&docAttrs);
        ForumsHandler.OnCommitDoc(&inserter);
        inserter.Flush();
        DTCreator->ProcessDirectText(*TarcCreator, nullptr, &docAttrs);
        DTCreator->ClearDirectText();
        DTCreator->ClearLemmatizationCache();
    }
    Cout << ForumsHandler.GetGeneratorName()
        << "\t" << ForumsHandler.GetNumPosts()
        << "\t" << ForumsHandler.GetFirstPostDate().ToString()
        << "\t" << ForumsHandler.GetLastPostDate().ToString()
        << "\t" << ForumsHandler.GetNumDifferentAuthors()
        << "\t" << f.Url
        << "\n";

    THolder<IDocumentDataInserter> fakeInserter;
    if (!!TarcOutput) {
        const TArchiveHeader* arcHeader = reinterpret_cast<const TArchiveHeader*>(TarcBuffer.Data());
        fakeInserter.Reset(new TFakeInserterVerbose(arcHeader));
    } else {
        fakeInserter.Reset(new TFakeInserter);
    }
    ForumsHandler.OnCommitDoc(fakeInserter.Get());
}

int main_exc(int argc, char* argv[])
{
    Opt2 opt(argc, argv, "a:c:f:m:Mo:dv", 0);
    bool downloadMode        = opt.Has('d', "download mode - read URLs from stdin, download via HTTP");
    const char* inArc        = opt.Arg('a', "<archive>: tag archive for processing", nullptr);
    const char* configDir    = opt.Arg('c', "<configDir>: dir with htparser.ini (optional) and dict.dict", nullptr);
    const char* fileDir      = opt.Arg('f', "<fileDir>: dir with (optionally) gzipped files", nullptr);
    const char* metadataFile = opt.Arg('m', "<metadata>: use file with additional metadata", nullptr);
    bool metadataIsFirstLine = opt.Has('M', "use first line of input files as metadata");
    const char* outArc       = opt.Arg('o', "<outArchive>: create text archive with results of processing", nullptr);
    bool verbose             = opt.Has('v', "print text with zones");
    opt.AutoUsageErr("");

    int modesCount = (downloadMode ? 1 : 0) + (inArc ? 1 : 0) + (fileDir ? 1 : 0);
    if (modesCount != 1) {
        Cerr << "error: exactly one of -d, -a, -f must be given";
        return 1;
    }
    if (verbose && outArc) {
        Cerr << "error: can't run verbose mode with archive mode";
        return 2;
    }

    TWorkers workers;

    try {
        if (configDir)
            workers.ConfigureParser(TString::Join(configDir, "/htparser.ini").data());
    } catch (...) {
        // just ignore any errors (most likely htparser.ini is missing)
    }

    if (configDir)
        workers.InitRecognizer(TString::Join(configDir, "/dict.dict").data());

    if (verbose)
        workers.InitTextArchiveWriterVerbose();
    else if (outArc)
        workers.InitTextArchiveWriter(outArc);


    THtmlFileReader::EMetaDataMode metaMode = THtmlFileReader::MDM_None;
    if (metadataIsFirstLine)
        metaMode = THtmlFileReader::MDM_FirstLine;
    if (metadataFile)
        metaMode = THtmlFileReader::MDM_Mapping;
    THtmlFileReader fileReader(metaMode);
    if (metadataFile)
        fileReader.InitMapping(metadataFile);
    else if (metadataIsFirstLine)
        fileReader.MetaData.HtmlComment = true;
    fileReader.SetDirectory(fileDir);
    fileReader.CommonTime.SetTimestamp(time(nullptr));
    // format of metadata: <filename>\t<url>\t[maybe other fields\t]<download date>
    fileReader.FileNameColumn = 0;
    fileReader.MetaData.Url.Col = 1;
    fileReader.MetaData.Date.Col = -1;

    int docId = 0;

    if (downloadMode) {
        TString url;
        THtmlDocument doc;
        while (Cin.ReadLine(url)) {
            doc.Clear();
            doc.Url = url;
            try {
                Fetch(doc);
            } catch (...) {
                Cerr << "failed to download document " << url << ": " << CurrentExceptionMessage() << "\n";
                continue;
            }
            workers.ProcessDoc(doc, docId++);
        }
    } else if (inArc) {
        TFullArchiveIterator it;
        it.Open(inArc);
        while (it.NextAuto()) {
            const TFullArchiveDocHeader* hdr = it.GetFullHeader();
            if (hdr->MimeType != MIME_HTML)
                continue;
            THtmlDocument f;
            f.Url = it.GetUrl();
            const char* docText = it.MakeDocText();
            f.Html = TStringBuf(docText, it.GetDocTextSize());
            f.Time.SetTimestamp(hdr->IndexDate);
            f.ForcedCharset = (ECharset)hdr->Encoding;
            f.ForcedLanguage = NameByLanguage((ELanguage)hdr->Language);
            workers.ProcessDoc(f, it.GetDocId());
        }
    } else {
        TFileList fileList;
        fileList.Fill(fileDir);
        TVector<TString> allFiles;
        const char* fname;
        while ((fname = fileList.Next()) != nullptr) {
            allFiles.push_back(fname);
        }
        Sort(allFiles.begin(), allFiles.end());
        for (TVector<TString>::const_iterator it = allFiles.begin(); it != allFiles.end(); ++it) {
            THtmlFile f = fileReader.Read(it->data());
            if (f.Url == "http://")
                f.Url = "http://fake.host/" + *it;
            workers.ProcessDoc(f, docId++);
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    try {
        return main_exc(argc, argv);
    } catch (...) {
        printf("exception: %s\n", CurrentExceptionMessage().data());
        return 1;
    }
}
