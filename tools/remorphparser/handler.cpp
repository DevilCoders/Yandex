#include "handler.h"

#include "arc_reader.h"
#include "time_monitor.h"
#include "corpus_tagger.h"

#include <FactExtract/Parser/common/sdocattributes.h>
#include <FactExtract/Parser/common/docreaders/stdinyandexdocsreader.h>
#include <FactExtract/Parser/common/docreaders/docbody.h>

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/nodeiterator.h>
#include <kernel/tarc/iface/fulldoc.h>

#include <dict/corpus/corpus.h>

#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/directtext/dt_input_symbol.h>
#include <kernel/remorph/directtext/dt_processor.h>
#include <kernel/remorph/facts/json_printer.h>
#include <kernel/remorph/info/info.h>
#include <kernel/remorph/input/richtree/richtree.h>
#include <kernel/remorph/text/textprocessor.h>
#include <kernel/remorph/text/result_print.h>
#include <kernel/remorph/tokenizer/worker_pool.h>
#include <kernel/remorph/tokenizer/callback.h>

#include <kernel/gazetteer/gazetteer.h>
#include <kernel/gazetteer/richtree/gztres.h>
#include <kernel/indexer/baseproc/docprocessor.h>
#include <kernel/indexer/baseproc/indexconf.h>
#include <kernel/indexer/direct_text/dt.h>
#include <kernel/indexer/direct_text/fl.h>
#include <kernel/indexer/faceproc/docattrs.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/parseddoc/pdstorage.h>
#include <kernel/indexer/parseddoc/pdstorageconf.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/strip.h>

namespace NRemorphParser {

namespace {

struct IHandler {
    virtual ~IHandler() {}

    virtual void ProcessFile(const TString& file) = 0;
    virtual void ProcessText(const TString& uri, const TWtringBuf& text) = 0;
    virtual void ProcessStream(IInputStream& in) = 0;
    virtual void ProcessCorpusHeader(const TString* title) = 0;
    virtual void ProcessCorpusText(IInputStream& input, ui32 id) = 0;
    virtual void ProcessQuery(const NToken::TSentenceInfo& sentInfo, const TRichNodePtr& richTree) = 0;
    virtual void ProcessIndexFile(const TString& file) = 0;
    virtual void ProcessIndexStream(IInputStream& in) = 0;
    virtual void ProcessIndexCorpusText(IInputStream& input, ui32 id) = 0;
    virtual void Finish() = 0;
};

template <bool MultiThreaded, bool TextOutput>
class THandlerBase: public IHandler, public NIndexerCore::IDirectTextCallback2 {
protected:
    THolder<NText::TResultPrinter<MultiThreaded>> Printer;
    THolder<TGazetteer> Gazetteer;
    THolder<const NGeoGraph::TGeoGraph> GeoGraph;
    const TRunOpts* Opts;

protected:
    THandlerBase(const TRunOpts& opts)
        : Printer(TextOutput ? new NText::TResultPrinter<MultiThreaded>(*opts.Output) : nullptr)
        , Opts(&opts)
    {
        if (!Opts->GztPath.Empty()) {
            Gazetteer.Reset(new TGazetteer(Opts->GztPath));
            if (Opts->InitGeoGazetteer) {
                GeoGraph.Reset(new NGeoGraph::TGeoGraph(Gazetteer.Get()));
                Gazetteer->SetGeoGraph(GeoGraph.Get());
            }
        }

        if (TextOutput) {
            Printer->SetPrintVerbosity(Opts->PrintVerbosity);
            Printer->SetPrintUnmatched(Opts->PrintUnmatched);
            Printer->SetInvertGrep(Opts->PrintInvertGrep);
            Printer->SetColorized(Opts->Colorized);
        }
    }

    TUtf16String CreateSentenceText(const NIndexerCore::TDirectTextEntry2* entries, size_t count) {
        TUtf16String res;
        // Indexer inserts special NULL tokens at the beginning if there is a space before a sentence or
        // it starts parsing a plain text. Ignore spaces from such tokens.
        bool skipLeaders = true;
        for (size_t i = 0; i < count; ++i) {
            const NIndexerCore::TDirectTextEntry2& entry = entries[i];
            if (entry.Token) {
                res.AppendNoAlias(entry.Token);
                skipLeaders = false;
            }
            if (nullptr == entry.Token && skipLeaders)
                continue;

            for (ui32 sp = 0; sp < entry.SpaceCount; ++sp) {
                res.AppendNoAlias(entry.Spaces[sp].Space, entry.Spaces[sp].Length);
            }
        }
        return res;
    }

    virtual void InternalProcessFile(const TString& file) = 0;
    virtual void InternalProcessText(const TWtringBuf& text) = 0;
    virtual void InternalProcessStream(IInputStream& in) = 0;
    virtual void InternalProcessCorpusText(IInputStream& input, ui32 id) = 0;
    virtual void InternalProcessQuery(const NToken::TSentenceInfo& sentInfo, NReMorph::NRichNode::TGztResultIter* gztIter,
        const NReMorph::NRichNode::TNodeInputSymbols& symbols) = 0;
    virtual void InternalProcessIndex(const NToken::TSentenceInfo& sentInfo,
        const NDT::TDTInputSymbols& inputSymbols) = 0;

    struct TDTSplitCallback {
        THandlerBase& Handler;
        NToken::TSentenceInfo SentInfo;

        TDTSplitCallback(THandlerBase& h)
            : Handler(h)
        {
        }

        inline void operator()(const NIndexerCore::TDirectTextEntry2* entries, size_t count,
            const NDT::TDTInputSymbols& symbols) {

            SentInfo.Text = Handler.CreateSentenceText(entries, count);
            SentInfo.Pos.first = entries[0].OrigOffset;
            SentInfo.Pos.second = SentInfo.Pos.first + SentInfo.Text.length();
            Handler.InternalProcessIndex(SentInfo, symbols);
            ++SentInfo.SentenceNum;
        }
    };

public:
    void ProcessFile(const TString& file) override {
        if (TextOutput) {
            Printer->SetCurrentDoc(file);
        }
        InternalProcessFile(file);
        if (TextOutput && !Printer->GetPrintUnmatched() && 0 == Printer->GetResultCount() &&
            NText::PV_GREP != Printer->GetPrintVerbosity()) {
            *(Opts->Output) << file << Endl;
        }
    }

    void ProcessText(const TString& uri, const TWtringBuf& text) override {
        if (TextOutput) {
            Printer->SetCurrentDoc(uri);
        }
        InternalProcessText(text);
        if (TextOutput && !Printer->GetPrintUnmatched() && 0 == Printer->GetResultCount() &&
            NText::PV_GREP != Printer->GetPrintVerbosity())
            *(Opts->Output) << uri << Endl;
    }

    void ProcessStream(IInputStream& in) override {
        if (TextOutput) {
            Printer->SetCurrentDoc(TString());
        }
        InternalProcessStream(in);
    }

    void ProcessCorpusHeader(const TString* title) override {
        Y_UNUSED(title);
    }

    void ProcessCorpusText(IInputStream& input, ui32 id) override {
        if (TextOutput) {
            Printer->SetCurrentDoc(::ToString(id));
        }
        InternalProcessCorpusText(input, id);
    }

    void ProcessQuery(const NToken::TSentenceInfo& sentInfo, const TRichNodePtr& richTree) override {
        TConstNodesVector nodes;
        if (Opts->TokenizerOpts.MultitokenSplit == NToken::MS_ALL) {
            GetChildNodes(*richTree, nodes, IsWord, false);
        } else {
            GetChildNodes(*richTree, nodes, IsWordOrMultitoken, false);
        }

        TVector<size_t> offsets;
        NReMorph::NRichNode::TNodeInputSymbols inputSymbols = Opts->QueryPunctuation
            ? NReMorph::NRichNode::CreateInputSymbols(nodes, offsets, Opts->Lang)
            : NReMorph::NRichNode::CreateInputSymbols(nodes, Opts->Lang);

        THolder<TGztResults> gztResults;
        if (Gazetteer.Get()) {
            TGztResults gztResults(richTree, Gazetteer.Get());
            NReMorph::NRichNode::TGztResultIter iter(gztResults, nodes, &NReMorph::NRichNode::IsDividedByComma, offsets);
            InternalProcessQuery(sentInfo, &iter, inputSymbols);
        } else {
            InternalProcessQuery(sentInfo, nullptr, inputSymbols);
        }
    }

    void ProcessIndexFile(const TString& file) override {
        if (TextOutput) {
            Printer->SetCurrentDoc(file);
        }
        TIFStream fs(file);
        ProcessIndexStream(fs);
    }

    void ProcessIndexStream(IInputStream& in) override {
        TString text = in.ReadAll();

        TParsedDocStorageConfig storageConfig;
        TIndexProcessorConfig indexConfig;

        indexConfig.DefaultLangMask = Opts->Lang;
        TAutoPtr<NIndexerCore::TParsedDocStorage> parsedDocStorage(new NIndexerCore::TParsedDocStorage(storageConfig));
        NIndexerCore::TDocumentProcessor documentProcessor(parsedDocStorage, &indexConfig);

        documentProcessor.AddDirectTextCallback(this);

        TDocInfoEx docInfoEx;
        TFullArchiveDocHeader docHeader;
        docHeader.MimeType = MIME_TEXT;
        docHeader.Encoding = static_cast<i8>(Opts->Encoding);
        docInfoEx.DocHeader = &docHeader;
        docInfoEx.DocText = text.data();
        docInfoEx.DocSize = text.size();
        TFullDocAttrs docAttrs;

        documentProcessor.ProcessOneDoc(&docInfoEx, &docAttrs);
        documentProcessor.Term();
    }

    void ProcessIndexCorpusText(IInputStream& input, ui32 id) override {
        if (TextOutput) {
            Printer->SetCurrentDoc(ToString(id));
        }
        ProcessIndexStream(input);
    }

    void ProcessDirectText2(IDocumentDataInserter* /*inserter*/,
        const NIndexerCore::TDirectTextData2& directText, ui32 /*docId*/) override {
        TDTSplitCallback callback(*this);
        NDT::SplitDirectText(callback, directText, Opts->Lang);
    }

    void Finish() override {
    }
};


template <bool MultiThreaded, class TMatcher>
class TRulesHandler: public THandlerBase<MultiThreaded, true> {
private:
    typedef THandlerBase<MultiThreaded, true> TBase;
    typedef typename NText::TTextProcessor<TMatcher>::TResults TResults;

private:
    typename NText::TTextProcessor<TMatcher>::TPtr Processor;

protected:
    void InternalProcessFile(const TString& file) override {
        Processor->ProcessFile(*TBase::Printer, file, TBase::Opts->Encoding, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
    }

    void InternalProcessText(const TWtringBuf& text) override {
        Processor->ProcessText(*TBase::Printer, text, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
    }

    void InternalProcessStream(IInputStream& in) override {
        Processor->ProcessStream(*TBase::Printer, in, TBase::Opts->Encoding, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
    }

    void InternalProcessCorpusText(IInputStream& input, ui32 id) override {
        Y_UNUSED(id);
        InternalProcessStream(input);
    }

    void InternalProcessQuery(const NToken::TSentenceInfo& sentInfo,
        NReMorph::NRichNode::TGztResultIter* gztIter, const NReMorph::NRichNode::TNodeInputSymbols& symbols) override {

        NReMorph::NRichNode::TNodeInput input;
        if (nullptr != gztIter) {
            if (!Processor->CreateInput(input, symbols, *gztIter)) {
                REPORT(VERBOSE, "No usable gazetteer results");
                return;
            }
        } else {
            // Create single-branch input
            input.Fill(symbols.begin(), symbols.end());
        }

        TResults results;
        Processor->Process(input, results);
        if (int(TRACE_DEBUG) <= GetVerbosityLevel()) {
            for (typename TResults::const_iterator iRes = results.begin(); iRes != results.end(); ++iRes) {
                REPORT(DEBUG, "Rule " << iRes->Get()->ToDebugString(GetVerbosityLevel(), input));
            }
        }

        (*TBase::Printer)(sentInfo, input, results);
    }

    void InternalProcessIndex(const NToken::TSentenceInfo& sentInfo, const NDT::TDTInputSymbols& inputSymbols) override {
        NDT::TDTInput input;
        if (!Processor->CreateInput(input, inputSymbols, TBase::Gazetteer.Get())) {
            REPORT(VERBOSE, "No usable gazetteer results");
            return;
        }

        TResults results;
        Processor->Process(input, results);

        if (int(TRACE_DEBUG) <= GetVerbosityLevel()) {
            for (typename TResults::const_iterator r = results.begin(); r != results.end(); ++r) {
                REPORT(DEBUG, "Rule " << r->Get()->ToDebugString(GetVerbosityLevel(), input));
            }
        }

        (*TBase::Printer)(sentInfo, input, results);
    }

public:
    TRulesHandler(const TRunOpts& opts)
        : TBase(opts)
        , Processor()
    {
        Processor = new NText::TTextProcessor<TMatcher>(opts.MatcherPath, TBase::Gazetteer.Get(), opts.BaseDir);
        switch (opts.MatcherMode) {
        case TRunOpts::MM_Match:
            Processor->SetMode(NMatcher::SM_MATCH);
            break;
        case TRunOpts::MM_MatchAll:
            Processor->SetMode(NMatcher::SM_MATCH_ALL);
            break;
        case TRunOpts::MM_MatchBest:
            Processor->SetMode(NMatcher::SM_MATCH_ALL);
            Processor->SetResolveResultAmbiguity(true);
            Processor->SetResultRankMethod(opts.RankMethod);
            break;
        case TRunOpts::MM_Search:
            Processor->SetMode(NMatcher::SM_SEARCH);
            break;
        case TRunOpts::MM_SearchAll:
            Processor->SetMode(NMatcher::SM_SEARCH_ALL);
            break;
        case TRunOpts::MM_SearchBest:
            Processor->SetMode(NMatcher::SM_SEARCH_ALL);
            Processor->SetResolveResultAmbiguity(true);
            Processor->SetResultRankMethod(opts.RankMethod);
            break;
        case TRunOpts::MM_Solutions:
            Processor->SetMode(NMatcher::SM_SEARCH_ALL);
            TBase::Printer->SetPrintResultMode(NText::PRM_SOLUTIONS);
            TBase::Printer->SetSolutionRankMethod(opts.RankMethod);
            break;
        default:
            Y_FAIL("Unimplemented mode");
        }

        Processor->SetResolveGazetteerAmbiguity(!opts.AllGazetteerResults);
        Processor->SetGazetteerRankMethod(opts.GazetteerRankMethod);
        Processor->SetThreads(opts.Threads);
    }
};

template <bool MultiThreaded, bool CorpusedOutput, bool JsonOutput>
class TFactsHandler: public THandlerBase<MultiThreaded, !(CorpusedOutput || JsonOutput)> {
private:
    typedef THandlerBase<MultiThreaded, !(CorpusedOutput || JsonOutput)> TBase;

private:
    THolder<NText::TTextFactProcessor> Processor;
    THolder<NCorpus::TJsonCorpus> TagsCorpus;
    THolder<TCorpusTagger<MultiThreaded>> CorpusTagger;
    THolder<NFact::TJsonPrinter<MultiThreaded>> JsonPrinter;

protected:
    void InternalProcessFile(const TString& file) override {
        if (CorpusedOutput) {
            Processor->ProcessFile(*CorpusTagger, file, TBase::Opts->Encoding, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
        } else if (JsonOutput) {
            Processor->ProcessFile(*JsonPrinter, file, TBase::Opts->Encoding, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
        } else {
            Processor->ProcessFile(*TBase::Printer, file, TBase::Opts->Encoding, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
        }
    }

    void InternalProcessText(const TWtringBuf& text) override {
        if (CorpusedOutput) {
            Processor->ProcessText(*CorpusTagger, text, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
        } else if (JsonOutput) {
            Processor->ProcessText(*JsonPrinter, text, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
        } else {
            Processor->ProcessText(*TBase::Printer, text, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
        }
    }

    void InternalProcessStream(IInputStream& in) override {
        if (CorpusedOutput) {
            Processor->ProcessStream(*CorpusTagger, in, TBase::Opts->Encoding, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
        } else if (JsonOutput) {
            Processor->ProcessStream(*JsonPrinter, in, TBase::Opts->Encoding, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
        } else {
            Processor->ProcessStream(*TBase::Printer, in, TBase::Opts->Encoding, TBase::Opts->TokenizerOpts, TBase::Opts->Lang);
        }
    }

    void InternalProcessCorpusText(IInputStream& input, ui32 id) override {
        if (CorpusedOutput) {
            CorpusTagger->NewText(id);
        }
        InternalProcessStream(input);
    }

    void InternalProcessQuery(const NToken::TSentenceInfo& sentInfo, NReMorph::NRichNode::TGztResultIter* gztIter, const NReMorph::NRichNode::TNodeInputSymbols& symbols) override {
        Y_ASSERT(!CorpusedOutput);

        TVector<NFact::TFactPtr> facts;
        if (nullptr != gztIter) {
            Processor->CollectFacts(symbols, *gztIter, facts);
        } else {
            Processor->CollectFacts(symbols, facts);
        }

        if (JsonOutput) {
            (*JsonPrinter)(sentInfo, symbols, facts);
        } else {
            (*TBase::Printer)(sentInfo, symbols, facts);
        }
    }

    void InternalProcessIndex(const NToken::TSentenceInfo& sentInfo, const NDT::TDTInputSymbols& inputSymbols) override {
        Y_ASSERT(!CorpusedOutput);

        TVector<NFact::TFactPtr> facts;
        Processor->CollectFacts(inputSymbols, facts);

        if (JsonOutput) {
            (*JsonPrinter)(sentInfo, inputSymbols, facts);
        } else {
            (*TBase::Printer)(sentInfo, inputSymbols, facts);
        }
    }

    void ProcessCorpusHeader(const TString* title) override {
        if (CorpusedOutput) {
            CorpusTagger->SetHeader(title);
        }
    }

    void Finish() override {
        if (CorpusedOutput) {
            try {
                TagsCorpus->Save(*(TBase::Opts->Output), NCorpus::CT_TAGS);
            } catch (const NCorpus::TInvalidTextError& error) {
                Y_ASSERT(false);
            }
        }
    }

public:
    TFactsHandler(const TRunOpts& opts)
        : TBase(opts)
        , Processor()
        , TagsCorpus(CorpusedOutput ? new NCorpus::TJsonCorpus() : nullptr)
        , CorpusTagger(CorpusedOutput ? new TCorpusTagger<MultiThreaded>(*TagsCorpus) : nullptr)
        , JsonPrinter(JsonOutput ? new NFact::TJsonPrinter<MultiThreaded>(*(TBase::Opts->Output)) : nullptr)
    {
        Processor.Reset(new NText::TTextFactProcessor(opts.MatcherPath, TBase::Gazetteer.Get()));
        Processor->SetThreads(opts.Threads);
        Processor->SetResolveFactAmbiguity(false);
        switch (opts.FactMode) {
        case TRunOpts::FM_Best:
            Processor->SetResolveFactAmbiguity(true);
            Processor->SetFactRankMethod(opts.RankMethod);
            break;
        case TRunOpts::FM_Solutions:
            if (!CorpusedOutput) {
                TBase::Printer->SetPrintResultMode(NText::PRM_SOLUTIONS);
                TBase::Printer->SetSolutionRankMethod(opts.RankMethod);
            }
            break;
        default:
            break;
        }
    }
};

inline bool IsAcceptedDoc(const SDocumentAttribtes& attrs, const TRunOpts& opts) {
    if (!opts.FilterLang.Empty() && !opts.FilterLang.SafeTest(attrs.m_Language))
        return false;

    if (!opts.FilterEncoding.Empty() && !opts.FilterEncoding.SafeTest(attrs.m_Charset))
        return false;

    if (!opts.FilterMimeType.Empty() && !opts.FilterMimeType.SafeTest(attrs.m_MimeType))
        return false;

    return true;
}

inline void SplitLines(NToken::ITokenizerCallback& cb, IInputStream& in, ECharset encoding) {
    TString line;
    NToken::TSentenceInfo info;
    for (; in.ReadLine(line); ++info.SentenceNum) {
        info.Text = CharToWide<true>(line, encoding);
        if (!::StripString(info.Text).Empty()) {
            cb.OnSentence(info);
        }
    }
}

inline void ProcessLines(NToken::ITokenizerCallback& cb, IInputStream& in, const TRunOpts& opts) {
    if (opts.Threads > 1) {
        NToken::TWorkerPoolPtr pool(new NToken::TWorkerPool(opts.Threads));
        NToken::TPoolCallbackProxy proxy(*pool, cb);
        SplitLines(proxy, in, opts.Encoding);
    } else {
        SplitLines(cb, in, opts.Encoding);
    }
}

struct TQueryCallback: public NToken::ITokenizerCallback {
    IHandler& Handler;
    const TRunOpts& Opts;

    TQueryCallback(IHandler& h, const TRunOpts& opts)
        : Handler(h)
        , Opts(opts)
    {
    }

    void OnSentence(const NToken::TSentenceInfo& sentInfo) override {
        TRichNodePtr richTree;
        TUtf16String line = sentInfo.Text;
        size_t tabPos = line.find('\t');
        if (TUtf16String::npos != tabPos) {
            line.erase(tabPos);
        }
        try {
            richTree = CreateRichNode(line, TCreateTreeOptions(Opts.Lang));
        }
        catch (const yexception& e) {
            Cerr << "ERROR: " << e.what() << Endl;
            return;
        }
        Handler.ProcessQuery(sentInfo, richTree);
    }

    inline static void Process(IInputStream& in, IHandler& handler, const TRunOpts& opts) {
        TQueryCallback cb(handler, opts);
        ProcessLines(cb, in, opts);
    }

    inline static void Process(const TString& path, IHandler& handler, const TRunOpts& opts) {
        TIFStream input(path);
        Process(input, handler, opts);
    }
};

inline void ProcessDoc(IInputStream& input, IHandler& handler, const TRunOpts& opts) {
    TTimeMonitor tm(opts.TimeLimit, "stdin");

    switch (opts.Format) {
    case TRunOpts::FMT_Text:
        handler.ProcessStream(input);
        break;
    case TRunOpts::FMT_Query:
        TQueryCallback::Process(input, handler, opts);
        break;
    case TRunOpts::FMT_Index:
        handler.ProcessIndexStream(input);
        break;
    default:
        Y_FAIL("Unimplemented input format.");
    }
}

inline void ProcessDoc(const TString& path, IHandler& handler, const TRunOpts& opts) {
    TTimeMonitor tm(opts.TimeLimit, path);

    switch (opts.Format) {
    case TRunOpts::FMT_Text:
        handler.ProcessFile(path);
        break;
    case TRunOpts::FMT_Query:
        TQueryCallback::Process(path, handler, opts);
        break;
    case TRunOpts::FMT_Index:
        handler.ProcessIndexFile(path);
        break;
    default:
        Y_FAIL("Unimplemented input format.");
    }
}

inline void ProcessCorpusDoc(const NCorpus::TCorpus::TText& text, IHandler& handler, const TRunOpts& opts) {
    REPORT(INFO, "Processing corpus text: " << text.Getid());

    ui32 id = text.Getid();
    TStringInput input(text.Gettext());

    TTimeMonitor tm(opts.TimeLimit, "corpus");

    switch (opts.Format) {
    case TRunOpts::FMT_Text:
        handler.ProcessCorpusText(input, id);
        break;
    case TRunOpts::FMT_Query:
        TQueryCallback::Process(input, handler, opts);
        break;
    case TRunOpts::FMT_Index:
        handler.ProcessIndexCorpusText(input, id);
        break;
    default:
        Y_FAIL("Unimplemented input format.");
    }
}

inline void ProcessCorpus(IInputStream& input, IHandler& handler, const TRunOpts& opts) {
    NCorpus::TJsonCorpus textCorpus(input, NCorpus::CT_TEXT);

    const TString* title = nullptr;
    if (textCorpus.Hastitle()) {
        title = &textCorpus.Gettitle();
    }
    handler.ProcessCorpusHeader(title);

    for (size_t t = 0; t < textCorpus.textsSize(); ++t) {
        ProcessCorpusDoc(textCorpus.Gettexts(t), handler, opts);
    }
}

inline void ProcessCorpus(const TString& path, IHandler& handler, const TRunOpts& opts) {
    TIFStream input(path);
    ProcessCorpus(input, handler, opts);
}

inline void ProcessArc(IInputStream& input, IHandler& handler, const TRunOpts& opts) {
    Y_UNUSED(input);
    Y_UNUSED(handler);
    Y_UNUSED(opts);
    Y_ASSERT(false);
}

inline void ProcessArc(const TString& path, IHandler& handler, const TRunOpts& opts) {
    TArcReader arcReader(path);
    TUtf16String text;
    SDocumentAttribtes textInfo;
    while (arcReader.GetNextTextData(&textInfo, &text)) {
        if (IsAcceptedDoc(textInfo, opts)) {
            TTimeMonitor tm(opts.TimeLimit, textInfo.m_strUrl);
            handler.ProcessText(textInfo.m_strUrl, text);
        }
    }
}

inline void ProcessTarcView(IInputStream& in, IHandler& handler, const TRunOpts& opts) {
    CStdinYandexDocsReader yandexDocsReader(in, opts.Encoding);
    CDocBody docBody;
    TString uri;
    SDocumentAttribtes attrs;
    while (yandexDocsReader.GetNextDocInfo(uri, attrs)) {
        if (IsAcceptedDoc(attrs, opts)) {
            docBody.Reset();
            if (!yandexDocsReader.GetDocBodyByAdress(uri, docBody)) {
                REPORT(WARN, "YandexDocsReader: Cannot get document body, uri=" << uri);
                continue;
            }
            TTimeMonitor tm(opts.TimeLimit, attrs.m_strUrl);
            handler.ProcessText(attrs.m_strUrl, docBody.GetBody());
            attrs.Reset();
        }
    }
}

inline void ProcessTarcView(const TString& path, IHandler& handler, const TRunOpts& opts) {
    TIFStream input(path);
    ProcessTarcView(input, handler, opts);
}

// TODO: вынести arc и tarcview из форматов в одельную опцию (опции?).
template <typename TInputType>
inline void ProcessInput(TInputType input, IHandler& handler, const TRunOpts& opts) {
    switch (opts.InMode) {
    case TRunOpts::IM_Plain:
        ProcessDoc(input, handler, opts);
        break;
    case TRunOpts::IM_Corpus:
        ProcessCorpus(input, handler, opts);
        break;
    case TRunOpts::IM_Arc:
        ProcessArc(input, handler, opts);
        break;
    case TRunOpts::IM_Tarcview:
        ProcessTarcView(input, handler, opts);
        break;
    default:
        Y_FAIL("Unimplemented input mode.");
    }
}

inline TAutoPtr<IHandler> CreateHandler(const TRunOpts& opts) {
    bool multiThread = opts.Threads > 1;
    bool corpusedOutput = opts.OutMode == TRunOpts::OM_CorpusTags;
    bool jsonOutput = opts.OutMode == TRunOpts::OM_FactsJson;
    switch (opts.MatcherType) {
    case TRunOpts::MT_Fact:
        if (multiThread) {
            if (corpusedOutput) {
                return new TFactsHandler<true, true, false>(opts);
            } else if (jsonOutput) {
                return new TFactsHandler<true, false, true>(opts);
            } else {
                return new TFactsHandler<true, false, false>(opts);
            }
        } else {
            if (corpusedOutput) {
                return new TFactsHandler<false, true, false>(opts);
            } else if (jsonOutput) {
                return new TFactsHandler<false, false, true>(opts);
            } else {
                return new TFactsHandler<false, false, false>(opts);
            }
        }
    case TRunOpts::MT_Remorph:
        if (multiThread) {
            return new TRulesHandler<true, NReMorph::TMatcher>(opts);
        } else {
            return new TRulesHandler<false, NReMorph::TMatcher>(opts);
        }
    case TRunOpts::MT_Tokenlogic:
        if (multiThread) {
            return new TRulesHandler<true, NTokenLogic::TMatcher>(opts);
        } else {
            return new TRulesHandler<false, NTokenLogic::TMatcher>(opts);
        }
    case TRunOpts::MT_Char:
        if (multiThread) {
            return new TRulesHandler<true, NReMorph::TCharEngine>(opts);
        } else {
            return new TRulesHandler<false, NReMorph::TCharEngine>(opts);
        }
    default:
        Y_FAIL("Unimplemented matcher type");
    }
    return nullptr;
}

}

void ProcessDocs(const TRunOpts& opts) {
    if (opts.Info) {
        Cout << NReMorph::REMORPH_INFO << Endl;
        return;
    }

    TAutoPtr<IHandler> handler = CreateHandler(opts);

    if (!opts.FreeArgs.empty()) {
        for (TVector<TString>::const_iterator i = opts.FreeArgs.begin(); i != opts.FreeArgs.end(); ++i) {
            REPORT(INFO, "Processing input from: " << *i);
            ProcessInput(*i, *handler, opts);
        }
    } else {
        ProcessInput<IInputStream&>(Cin, *handler, opts);
    }

    handler->Finish();
}

} // NRemorphParser
