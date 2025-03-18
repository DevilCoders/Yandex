
#include "jobqueue.h"
#include "outputqueue.h"
#include "truncate.h"
#include "json_dump_parse.h"

#include <tools/snipmake/snipdat/askctx.h>
#include <tools/snipmake/snipdat/metahost.h>
#include <tools/snipmake/snipdat/xmlsearchin.h>
#include <tools/snipmake/reqrestr/reqrestr.h>
#include <tools/snipmake/download_contexts/common/querylog.h>

#include <search/idl/meta.pb.h>
#include <search/session/compression/report.h>
#include <kernel/snippets/idl/snippets.pb.h>

#include <ysite/yandex/common/prepattr.h>

#include <library/cpp/colorizer/output.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/datetime/cputimer.h>
#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/random/random.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/printf.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/system/mutex.h>

namespace NSnippets
{
static const int MAX_SEARCHER_USES = 30;
static const char* const BLACKHOLE_ADDRESS = "mcquack.search.yandex.net:65432";

TMutex cerrMutex;

class THamsterParams
{
public:
    TString InputFile;
    TString OutputFile;
    TString ResumeFile;
    bool Verbose = false;
    bool Quiet = false;
    TString SearchDomainPrefix;
    THashMap<TString, TString> SearcherAddress;
    TString ExtraCgiParams;
    bool CanMisspell = false;
    bool FamilySafe = false;
    int NumberOfThreads = 1;
    bool NoBlackhole = false;
    bool TakeOneRandomDoc = false;
    bool IgnoreRestriction = false;
    bool FullSerpOnly = false;
    TVector<TString> Sources;
    TString SearchEndpoint;
    size_t MaxPosition = 10;

public:
    THamsterParams(int argc, const char* argv[]) {
        auto addSearcherAddress = [&](const TString& str) {
            TStringBuf buf(str);
            TStringBuf name = buf.NextTok(':');
            if (!name || !buf) {
                ythrow NLastGetopt::TUsageException() << "Searcher hostname must be specified as SOURCE:host:port, e.g. WEB:mcquack:12345";
            }
            SearcherAddress[TString(name)] = TString(buf);
        };

        NLastGetopt::TOpts opts;
        opts.AddHelpOption();
        opts.AddLongOption('i', "input", "input file, stdin is used if omitted")
            .StoreResult(&InputFile);
        opts.AddLongOption('o', "output", "output file, stdout is used if omitted")
            .StoreResult(&OutputFile);
        opts.AddLongOption('r', "resume", "file for saving download state")
            .StoreResult(&ResumeFile);
        opts.AddLongOption('v', "verbose", "be verbose")
            .NoArgument()
            .SetFlag(&Verbose);
        opts.AddLongOption('q', "quiet", "be quiet")
            .NoArgument()
            .SetFlag(&Quiet);
        opts.AddLongOption("beta", "beta name (the part before .hamster.yandex.tld)")
            .StoreResult(&SearchDomainPrefix);
        opts.AddLongOption("misspell", "allow query autocorrection (don't add nomisspell=1)")
            .SetFlag(&CanMisspell)
            .NoArgument();
        opts.AddLongOption("family", "safe search (don't add family=none)")
            .SetFlag(&FamilySafe)
            .NoArgument();
        opts.AddLongOption('m', "middlesearch", "use custom middle search (format is SOURCE:host:port, may specify multiple sources)")
            .Handler1T(TString(), addSearcherAddress)
            .RequiredArgument();
        opts.AddLongOption('j', "jobs", "number of threads")
            .DefaultValue("1")
            .StoreResult(&NumberOfThreads);
        opts.AddLongOption('f', "full-search", "do a full XML search instead of blackholing the query")
            .NoArgument()
            .SetFlag(&NoBlackhole);
        opts.AddLongOption("one", "take only one random document from each request")
            .NoArgument()
            .SetFlag(&TakeOneRandomDoc);
        opts.AddLongOption("no-restr", "igrone url restriction")
            .NoArgument()
            .SetFlag(&IgnoreRestriction);
        opts.AddLongOption("full-serp", "return full serps only")
            .NoArgument()
            .SetFlag(&FullSerpOnly);
        opts.AddLongOption('e', "extra-cgi-params", "extra cgi params for middlesearch")
            .StoreResult(&ExtraCgiParams)
            .DefaultValue("");
        opts.AddLongOption('p', "max-position", "max position for url 0..9")
            .StoreResult(&MaxPosition)
            .DefaultValue("9");
        opts.SetFreeArgsMin(0);
        opts.SetFreeArgsMax(1);
        opts.SetFreeArgTitle(0, "mode", "Hamster type: images, video, web [MAY BE BROKEN, PLEASE CHECK RESULTS]");
        NLastGetopt::TOptsParseResult o(&opts, argc, argv);
        // Sources = {"WEB", "WEB_MISSPELL", "QUICK"};
        Sources = {"WEB"};
        SearchEndpoint = "/search/";
        if (o.GetFreeArgs()) {
            const TString& mode = o.GetFreeArgs().at(0);
            if (mode == "images") {
                Sources = {"IMAGES"};
                SearchEndpoint = "/images/search/";
            } else if (mode == "video") {
                Sources = {"VIDEO"};
                SearchEndpoint = "/video/search/";
            }
        }
        if (Quiet) {
            Verbose = false;
        }
        if (NumberOfThreads < 1) {
            NumberOfThreads = 1;
        }
        if (IgnoreRestriction) {
            FullSerpOnly = false;
        }
        if (SearchDomainPrefix) {
            SearchDomainPrefix = SearchDomainPrefix + ".hamster.yandex.";
        }
    }
};

class THamsterRequest
{
public:
    TString Query;
    TString CorrectedQuery;
    TString Region;
    TString SearchDomain;
    TString UIL;
    TString RequestId;
    TString SnipWidth;
    TString ReportType;
    TVector<TQueryResultDoc> Docs;
    TVector<TQueryResultDoc> MisspellDocs;

public:
    THamsterRequest(const TString& line) {
        TQueryLog queryLog = TQueryLog::FromString(line);
        Query = queryLog.Query;
        if (queryLog.CorrectedQuery.Defined()) {
            CorrectedQuery = queryLog.CorrectedQuery.GetRef();
        }
        Region = queryLog.UserRegion;
        SearchDomain = queryLog.DomRegion;
        UIL = queryLog.UILanguage;
        RequestId = queryLog.RequestId;
        if (queryLog.SnipWidth.Defined()) {
            SnipWidth = queryLog.SnipWidth.GetRef();
        }
        if (queryLog.ReportType.Defined()) {
            ReportType = queryLog.ReportType.GetRef();
        }
        for (const auto& doc : queryLog.Docs) {
            if (doc.Source.Contains("MISSPELL")) {
                MisspellDocs.push_back(doc);
            } else {
                Docs.push_back(doc);
            }
        }
    }
};

class THamsterError: public yexception
{
};

class THamsterWheel : public ILineProcessor
{
private:
    const THamsterParams& Params;
    TOutputQueue& OutputQueue;
    TDomHosts WizardSearch;
    TDomHosts FullSearch;
    int SearcherUses;
    THashMap<TString, TSimpleSharedPtr<THost>> CurrentSearchers;

private:
    struct TExtractedDoc
    {
        TString Context;
        TString SnippetGta;
        TString Headline;
        TString HeadlineSrc;
    };

    void ExtractContexts(const TString& protoReport, TVector<TExtractedDoc>& contexts,
            const TVector<TQueryResultDoc>& urls)
    {
        NMetaProtocol::TReport report;

        if (!report.ParseFromString(protoReport)) {
            throw THamsterError() << "Failed to parse response as a TReport message";
        }
        NMetaProtocol::Decompress(report); // decompresses only if needed
        if (report.GroupingSize() < 1) {
            return;
        }
        const NMetaProtocol::TGrouping& grouping = report.GetGrouping(0);
        if (grouping.GetIsFlat() != NMetaProtocol::TGrouping::YES) {
            throw THamsterError() << "Weird grouping in meta response";
        }

        THashSet<TString> canonicalUrls;
        for (auto&& url : urls) {
            canonicalUrls.insert(PrepareURL(url.Url));
        }

        for (size_t i = 0; i < grouping.GroupSize(); ++i) {
            const NMetaProtocol::TGroup& group = grouping.GetGroup(i);
            for (size_t j = 0; j < group.DocumentSize(); ++j) {
                TExtractedDoc ctx;

                const NMetaProtocol::TDocument& doc = group.GetDocument(j);
                if (!doc.HasArchiveInfo()) {
                    continue;
                }
                const NMetaProtocol::TArchiveInfo& ar = doc.GetArchiveInfo();
                if (urls && !canonicalUrls.contains(PrepareURL(ar.GetUrl()))) {
                    continue;
                }
                if (ar.HasHeadline()) {
                    ctx.Headline = ar.GetHeadline();
                }
                for (size_t i = 0; i < ar.GtaRelatedAttributeSize(); ++i) {
                    const auto& key = ar.GetGtaRelatedAttribute(i).GetKey();
                    const auto& value = ar.GetGtaRelatedAttribute(i).GetValue();
                    if (key == "_SnippetsCtx") {
                        ctx.Context = value;
                    } else if (key == "Snippet") {
                        ctx.SnippetGta = value;
                    } else if (key == "_HeadlineSrc") {
                        ctx.HeadlineSrc = value;
                    }
                }

                if (ctx.Context) {
                    contexts.push_back(ctx);
                }
            }
        }
    }

    static TString BuildWizardCgi(const THamsterParams& params, bool useBlackhole)
    {
        // &noapache_json_req=app_host:upper is here to force the middle metasearch to use the plain old meta protocol instead of the apphost proto
        // we don't understand the apphost protocol yet
        TString result("noreask=1&noredirect=1&noapache_json_req=app_host:upper&json_dump=eventlog");

        if (!params.CanMisspell) {
            result += "&nomisspell=1";
        }
        if (!params.FamilySafe) {
            result += "&family=none";
        }
        if (!params.NoBlackhole && useBlackhole) {
            for (const TString& src : params.Sources) {
                result += Sprintf("&metahost=%s:%s", src.data(), BLACKHOLE_ADDRESS);
            }
        }
        result += "&" + params.ExtraCgiParams;
        return result;
    }

public:
    THamsterWheel(const THamsterParams& params, TOutputQueue& outputQueue)
        : Params(params)
        , OutputQueue(outputQueue)
        , WizardSearch(params.SearchDomainPrefix, BuildWizardCgi(params, true))
        , FullSearch(params.SearchDomainPrefix, BuildWizardCgi(params, false))
        , SearcherUses(0)
        , CurrentSearchers()
    {
        for (const TString& src : Params.Sources) {
            TSimpleSharedPtr<THost> host(new THost());
            if (Params.SearcherAddress.contains(src)) {
                const TString &addr = Params.SearcherAddress.at(src);
                if (!ParseHostport("http://", addr, *host)) {
                    ythrow yexception() << "Invalid host:port format: " << addr;
                }
            }
            CurrentSearchers[src] = host;
        }
    }

    virtual ~THamsterWheel()
    {
    }

    void Request(const THamsterRequest& request, size_t maxDocs, TVector<TString>& resultContexts,
            ui64& xmlTime, ui64& midTime)
    {
        TString searchDomain = request.SearchDomain;
        if (!searchDomain) {
            searchDomain = "ru";
        }
        const THost& dhost = (SearcherUses == 0 ? FullSearch : WizardSearch).Get(searchDomain);
        if (!dhost.CanFetch()) {
            throw THamsterError() << "Unknown domain: " << searchDomain;
        }

        if (Params.Verbose) {
            TGuard<TMutex> g(&cerrMutex);
            Cerr << dhost.Host << ":" << dhost.Port << Params.SearchEndpoint << "?..." << dhost.ExtraCgiParams << Endl;
        }

        TSimpleTimer xmlTimer;
        const int MAX_TRY_COUNT = 3;
        bool responseOk = false;
        TString response;
        THashMap<TString, TString> queryBySource;

        for (int tryCount = MAX_TRY_COUNT; tryCount > 0 && !responseOk; --tryCount) {
            response = dhost.RawSearch(Params.SearchEndpoint, request.Query, request.Region, request.UIL);
            if (!ParseEventlogFromJsonDump(response, queryBySource)) {
                continue;
            }
            for (const TString& src : Params.Sources) {
                if (queryBySource.contains(src)) {
                    responseOk = true;
                    break;
                }
            }
        }
        if (!responseOk) {
            throw THamsterError() << "Eventlog dump failed (" << MAX_TRY_COUNT << " times)";
        }
        xmlTime = xmlTimer.Get().MilliSeconds();

        for (const TString& src : Params.Sources) {
            THost mhost;
            TStringBuf mpath;
            TStringBuf mcgi;

            if (!queryBySource.contains(src)) {
                continue;
            }

            if (!ParseUrl(queryBySource.at(src), mhost, mpath, mcgi)) {
                throw THamsterError() << "search eventlog does not contain valid source query";
            }

            if (SearcherUses == 0) {
                *CurrentSearchers[src] = mhost;
            } else {
                mhost = *CurrentSearchers[src];
            }

            TCgiParameters cgi(mcgi);
            if (request.SnipWidth) {
                cgi.InsertUnescaped("snip", ToString("snip_width=") + request.SnipWidth);
            }
            if (request.ReportType) {
                cgi.InsertUnescaped("snip", ToString("report=") + request.ReportType);
            }
            FixMultipleSnip(cgi);
            if (request.Docs) {
                TVector<TString> urls;
                for (auto& doc : request.Docs) {
                    urls.push_back(doc.Url);
                }
                AddRestrToQtrees(cgi, urls);
            }
            cgi.ReplaceUnescaped("gopl", ToString(maxDocs));
            AskSnippetsCtx(cgi);
            TSimpleTimer midTimer;
            TString metaResponse = mhost.Fetch(TString{mpath} + cgi.Print());
            midTime = midTimer.Get().MilliSeconds();
            if (Params.Verbose) {
                TGuard<TMutex> g(&cerrMutex);
                Cerr << mhost.Host << ":" << mhost.Port << mpath << cgi.Print() << Endl;
            }
            TVector<TExtractedDoc> docs;
            ExtractContexts(metaResponse, docs, request.Docs);
            if (docs.size() > maxDocs) {
                docs.resize(maxDocs);
            }
            if (Params.FullSerpOnly && docs.size() < request.Docs.size()) {
                throw THamsterError() << "Partial result [" << docs.size()
                        << "/" << request.Docs.size() << "]";
            }
            if (Params.TakeOneRandomDoc && docs.size() > 1) {
                size_t randomPosition = RandomNumber<size_t>(docs.size());
                TExtractedDoc randomCtx = docs[randomPosition];
                docs.assign(1, randomCtx);
            }
            for (const TExtractedDoc& doc : docs) {
                NProto::TSnippetsCtx ctx;
                if (ctx.ParseFromString(Base64Decode(doc.Context))) {
                    ctx.MutableLog()->SetRequestId(request.RequestId);
                    size_t regionId;
                    if (TryFromString<size_t>(request.Region, regionId)) {
                        ctx.MutableLog()->SetRegionId(regionId);
                    }
                    ctx.MutableLog()->SetHeadlineSrc(doc.HeadlineSrc);
                    ctx.MutableLog()->SetHeadline(doc.Headline);
                    ctx.MutableLog()->SetSnippetGta(doc.SnippetGta);
                    TString data;
                    Y_PROTOBUF_SUPPRESS_NODISCARD ctx.SerializeToString(&data);
                    resultContexts.push_back(Base64Encode(data));
                }
            }
        }

        if (SearcherUses == 0) {
            SearcherUses = MAX_SEARCHER_USES;
        } else {
            SearcherUses--;
        }
    }

    void ProcessLine(const TString& line, size_t lineNum) override
    {
        if (OutputQueue.CheckCompleted(lineNum)) {
            return;
        }

        THamsterRequest request(line);
        if (request.Docs && !request.MisspellDocs) {
            request.CorrectedQuery.clear();
        }
        if (Params.IgnoreRestriction) {
            request.Docs.clear();
            request.MisspellDocs.clear();
        }
        bool noRestriction = !request.Docs && !request.MisspellDocs;
        TVector<TString> contexts;
        size_t maxDocs = (request.Query && request.CorrectedQuery) ?
                         (Params.MaxPosition + 1) / 2 : Params.MaxPosition + 1;
        if (request.Query && (request.Docs || noRestriction)) {
            if (!ProcessRequest(request, maxDocs, lineNum, contexts)) {
                OutputQueue.AddFailedResult(lineNum);
                return;
            }
        }
        if (request.CorrectedQuery && (request.MisspellDocs || noRestriction)) {
            request.Query = request.CorrectedQuery;
            request.Docs = request.MisspellDocs;
            if (!ProcessRequest(request, maxDocs, lineNum, contexts)) {
                OutputQueue.AddFailedResult(lineNum);
                return;
            }
        }
        OutputQueue.AddCompletedResult(lineNum, contexts);
    }

    bool ProcessRequest(const THamsterRequest& request, size_t maxDocs,
            size_t lineNum, TVector<TString>& contexts) {
        if (request.Docs) {
            maxDocs = request.Docs.size();
        }
        ui64 xmlTime = 0;
        ui64 midTime = 0;
        TString errorMessage;
        size_t oldSize = contexts.size();
        try {
            Request(request, maxDocs, contexts, xmlTime, midTime);
        } catch (yexception& e) {
            errorMessage = e.what();
        }

        if (!Params.Quiet) {
            TGuard<TMutex> g(&cerrMutex);
            Cerr << NColorizer::LightBlue() << "#" << lineNum << " ";
            Cerr << NColorizer::Old() << TruncateUtf8(request.Query, 60) << " ";
            if (!errorMessage) {
                size_t resultCount = contexts.size() - oldSize;
                if (Params.TakeOneRandomDoc && resultCount == 1) {
                    Cerr << NColorizer::LightGreen() << "[1/" << maxDocs << "]";
                } else if (request.Docs && resultCount < maxDocs) {
                    Cerr << NColorizer::LightGreen() << "[";
                    Cerr << NColorizer::LightRed() << resultCount;
                    Cerr << NColorizer::LightGreen() << "/" << maxDocs << "]";
                } else if (resultCount == 0) {
                    Cerr << NColorizer::LightGreen() << "[";
                    Cerr << NColorizer::LightRed() << "0";
                    Cerr << NColorizer::LightGreen() << "]";
                } else {
                    Cerr << NColorizer::LightGreen() << "[" << resultCount << "]";
                }
            } else {
                Cerr << NColorizer::LightRed() << errorMessage;
            }
            Cerr << NColorizer::DarkGray() << "  hamster:" << xmlTime << "ms  middle:" << midTime << "ms";
            Cerr << NColorizer::Old() << Endl;
        }
        return !errorMessage;
    }
};

class THamsterWheelFactory : public ILineProcessorFactory
{
private:
    const THamsterParams& Params;
    TOutputQueue& OutputQueue;
public:
    THamsterWheelFactory(const THamsterParams& params, TOutputQueue& outputQueue)
        : Params(params)
        , OutputQueue(outputQueue)
    {
    }
    ILineProcessor* CreateProcessor() override {
        return new THamsterWheel(Params, OutputQueue);
    }
    void DestroyProcessor(ILineProcessor* processor) override {
        delete (THamsterWheel*)processor;
    }
};

void RunHamster(int argc, const char* argv[])
{
    THamsterParams params(argc, argv);
    TOutputQueue outputQueue(params.OutputFile, params.ResumeFile);
    THamsterWheelFactory factory(params, outputQueue);

    {
        TProcessLineJobQueue jobQueue(factory, params.NumberOfThreads);
        jobQueue.AddAllLines(params.InputFile);
    }
}
} // namespace NSnippets

int main(int argc, const char* argv[])
{
    NSnippets::RunHamster(argc, argv);
}
