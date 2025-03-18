#include <tools/snipmake/argv/opt.h>
#include <tools/snipmake/stopwordlst/stopword.h>
#include <tools/snipmake/snipdat/askctx.h>
#include <tools/snipmake/snipdat/metahost.h>
#include <tools/snipmake/snipdat/xmlsearchin.h>
#include <tools/snipmake/protosnip/patch.h>

#include <kernel/snippets/base/default_snippetizer.h>
#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/iface/passagereplyfiller.h>

#include <kernel/snippets/urlcut/url_wanderer.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/snippets/strhl/goodwrds.h>

#include <kernel/snippets/idl/enums.h>
#include <kernel/snippets/idl/snippets.pb.h>
#include <kernel/snippets/idl/secondary_snippets.pb.h>

#include <search/idl/meta.pb.h>
#include <search/reqparam/reqparam.h>
#include <search/reqparam/snipreqparam.h>
#include <search/request/treatcgi/treatcgi.h>
#include <search/session/compression/report.h>

#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <library/cpp/svnversion/svnversion.h>

#include <library/cpp/neh/rpc.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/builder.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/network/socket.h>
#include <library/cpp/http/io/stream.h>
#include <util/stream/mem.h>
#include <util/stream/output.h>
#include <util/system/hostname.h>
#include <util/stream/file.h>

#include <signal.h>

typedef std::pair<int, float> TFactor;
typedef TVector<TFactor> TFactors;
typedef THashMap<TStringBuf, int> TFactorMapping;

static bool Verbose = false;
static bool UseCgiPatch = false;

static TString ExtractUrl(const NMetaProtocol::TDocument& doc) {
    if (!doc.HasArchiveInfo()) {
        return TString();
    }
    const NMetaProtocol::TArchiveInfo& ar = doc.GetArchiveInfo();
    return ar.HasUrl() ? ar.GetUrl() : TString();
}

class TResponsePatcher {
public:
    virtual void PatchDoc(NMetaProtocol::TDocument&, const TFactorMapping&, const TCgiParameters&,
            const NNeuralNetApplier::TModel*, const NNeuralNetApplier::TModel*) const {
    }

    virtual void PatchCgi(TCgiParameters&) const {
    }

    virtual bool WantPatch(bool isInfo, bool isSnippetInfo) const = 0;

    virtual TString GetSnippetsExplanation(const NMetaProtocol::TDocument&, const NSnippets::NProto::TSnipInfoReqParams*,
                                          const TFactorMapping&, const TCgiParameters&, const NNeuralNetApplier::TModel*,
                                          const NNeuralNetApplier::TModel*) const {
        return TString();
    }
};

class TSnippetPatcher : public TResponsePatcher {
public:
    TWordFilter StopWords;
    TString BaseExp;
    IOutputStream* CtxOut;
    IOutputStream* CtxROut;

    TSnippetPatcher()
        : CtxOut(nullptr)
        , CtxROut(nullptr)
    {
    }

    void PatchCgi(TCgiParameters& cgi) const override {
        NSnippets::AskSnippetsCtx(cgi);
    }

    void PatchDoc(NMetaProtocol::TDocument& doc, const TFactorMapping& factorMapping, const TCgiParameters& cgi,
            const NNeuralNetApplier::TModel* ruFactSnippetDssmApplier, const NNeuralNetApplier::TModel* tomatoDssmApplier) const override {
        NSnippets::TPassageReply reply;
        PassageReply(doc, reply, nullptr, factorMapping, cgi, ruFactSnippetDssmApplier, tomatoDssmApplier);
        NSnippets::PatchByPassageReply(doc, reply);
    }

    bool WantPatch(bool isInfo, bool isSnippetInfo) const override {
        return !isInfo || isSnippetInfo;
    }

    TString GetSnippetsExplanation(const NMetaProtocol::TDocument& doc, const NSnippets::NProto::TSnipInfoReqParams* infoParams,
                                   const TFactorMapping& factorMapping, const TCgiParameters& cgi,
                                   const NNeuralNetApplier::TModel* ruFactSnippetDssmApplier, const NNeuralNetApplier::TModel* tomatoDssmApplier) const override {
        NSnippets::TPassageReply reply;
        PassageReply(doc, reply, infoParams, factorMapping, cgi, ruFactSnippetDssmApplier, tomatoDssmApplier);
        return reply.GetSnippetsExplanation();
    }

private:
    void AddNewStuff(NSnippets::NProto::TSnippetsCtx& ctx, const TCgiParameters& cgi, const TFactorMapping& mapping, const TFactors& factors) const {
        if (UseCgiPatch) {
            TRequestParams rp;
            TreatCgiParams(rp, cgi);
            TCateg relevRegion = ctx.GetReqParams().GetRelevRegion();
            NSnippets::NProto::TSnipReqParams& srp = *ctx.MutableReqParams();
            SetSnipConfRP(rp, srp);
            srp.SetRelevRegion(relevRegion);
        }

        (void)mapping;
        (void)factors;
    }

    void PassageReply(const NMetaProtocol::TDocument& doc, NSnippets::TPassageReply& reply, const NSnippets::NProto::TSnipInfoReqParams* infoParams,
                      const TFactorMapping& factorMapping, const TCgiParameters& cgi, const NNeuralNetApplier::TModel* ruFactSnippetDssmApplier = nullptr,
                      const NNeuralNetApplier::TModel* tomatoDssmApplier = nullptr) const {
        if (!doc.HasArchiveInfo()) {
            if (Verbose) {
                Cout << "no arcinfo for: " << ExtractUrl(doc) << Endl;
            }
            return;
        }
        TFactors factors; // some of them, should be asked for
        for (size_t i = 0; i < doc.BinFactorSize(); ++i) {
            factors.push_back(TFactor(doc.GetBinFactor(i).GetKey(), doc.GetBinFactor(i).GetValue()));
        }
        const NMetaProtocol::TArchiveInfo& ar = doc.GetArchiveInfo();
        TStringBuf ctxs = NSnippets::FindCtx(ar);
        if (!ctxs.size()) {
            if (Verbose) {
                Cout << "no ctx for: " << ExtractUrl(doc) << Endl;
            }
            return;
        }
        if (CtxOut) {
            (*CtxOut) << ctxs << Endl;
        }
        TString raw = Base64Decode(ctxs);

        NSnippets::NProto::TSnippetsCtx ctx;
        if (!ctx.ParseFromString(raw)) {
            if (Verbose) {
                Cout << "broken ctx for: " << ExtractUrl(doc) << Endl;
            }
            return;
        }
        if (infoParams && ctx.HasReqParams()) {
            ctx.MutableReqParams()->MutableInfoRequestParams()->CopyFrom(*infoParams);
        }
        AddNewStuff(ctx, cgi, factorMapping, factors);
        if (CtxROut) {
            TBufferOutput res;
            ctx.SerializeToArcadiaStream(&res);
            TString rctxs = Base64Encode(TStringBuf(res.Buffer().data(), res.Buffer().size()));
            (*CtxROut) << rctxs << Endl;
        }
        PassageReply(ctx, ruFactSnippetDssmApplier, tomatoDssmApplier, reply);
    }

    void PassageReply(const NSnippets::NProto::TSnippetsCtx& ctx, const NNeuralNetApplier::TModel* ruFactSnippetDssmApplier, const NNeuralNetApplier::TModel* tomatoDssmApplier, NSnippets::TPassageReply& reply) const {
        NSnippets::TConfigParams cfgp;
        cfgp.StopWords = &StopWords;
        cfgp.RuFactSnippetDssmApplier = ruFactSnippetDssmApplier;
        cfgp.TomatoDssmApplier = tomatoDssmApplier;
        bool ignoreRegionPhrase = false;
        if (ctx.HasReqParams()) {
            const NSnippets::NProto::TSnipReqParams& rp = ctx.GetReqParams();
            if (rp.HasIgnoreRegionPhrase()) {
                ignoreRegionPhrase = rp.GetIgnoreRegionPhrase();
            }
            cfgp.SRP = &rp;
        }
        if (BaseExp.size()) {
            cfgp.AppendExps.push_back(BaseExp);
        }
        NSnippets::THitsInfoPtr hitsInfo;
        if (ctx.HasHits()) {
            hitsInfo = new NSnippets::THitsInfo();
            hitsInfo->Load(ctx.GetHits());
        }
        NSnippets::TVoidFetcher fetcher;
        NSnippets::TArcManip arcCtx(fetcher, fetcher);
        if (ctx.HasTextArc()) {
            arcCtx.GetTextArc().LoadState(ctx.GetTextArc());
        }
        if (ctx.HasLinkArc()) {
            arcCtx.GetLinkArc().LoadState(ctx.GetLinkArc());
        }
        TRichTreeConstPtr richtree;
        TRichTreeConstPtr moreHlRichtree;
        TRichTreeConstPtr regionPhraseRichtree;
        TInlineHighlighter ih;
        if (ctx.HasQuery() && ctx.GetQuery().HasQtreeBase64()) {
            richtree = DeserializeRichTree(DecodeRichTreeBase64(ctx.GetQuery().GetQtreeBase64()));
        }
        if (richtree.Get() && richtree->Root.Get()) {
            ih.AddRequest(*richtree->Root);
            if (ctx.HasQuery() && ctx.GetQuery().HasMoreHlTreeBase64()) {
                moreHlRichtree = DeserializeRichTree(DecodeRichTreeBase64(ctx.GetQuery().GetMoreHlTreeBase64()));
                if (moreHlRichtree.Get() && moreHlRichtree->Root.Get()) {
                    ih.AddRequest(*moreHlRichtree->Root, nullptr, true);
                }
            }
            if (ctx.HasQuery() && ctx.GetQuery().HasRegionPhraseTreeBase64()) {
                regionPhraseRichtree = DeserializeRichTree(DecodeRichTreeBase64(ctx.GetQuery().GetRegionPhraseTreeBase64()));
                if (!ignoreRegionPhrase && regionPhraseRichtree.Get() && regionPhraseRichtree->Root.Get()) {
                    ih.AddRequest(*regionPhraseRichtree->Root, nullptr, true);
                }
            }
        }

        NUrlCutter::TRichTreeWanderer wanderer(richtree);
        const NSnippets::TPassageReplyContext args(cfgp, hitsInfo, ih, arcCtx, richtree, regionPhraseRichtree, wanderer);

        TAutoPtr<NSnippets::IPassageReplyFiller> passageReplyFiller = CreatePassageReplyFiller(args);
        passageReplyFiller->FillPassageReply(reply);
    }
};

struct TSrv {
    const TString MiddleService;
    const TResponsePatcher& ResponsePatcher;
    const unsigned int Position;
    const TString RealMiddlesnip;
    const NNeuralNetApplier::TModel* RuFactSnippetDssmApplier;
    const NNeuralNetApplier::TModel* TomatoDssmApplier;

    TSrv(const TString& middleService, const TResponsePatcher& responsePatcher, unsigned int position, const TString& realMiddlesnip,
            const NNeuralNetApplier::TModel* ruFactSnippetDssmApplier, const NNeuralNetApplier::TModel* tomatoDssmApplier)
      : MiddleService(middleService)
      , ResponsePatcher(responsePatcher)
      , Position(position)
      , RealMiddlesnip(realMiddlesnip)
      , RuFactSnippetDssmApplier(ruFactSnippetDssmApplier)
      , TomatoDssmApplier(tomatoDssmApplier)
    {
    }

    void ServeRequest(const NNeh::IRequestRef& req) {
        TStringBuf reqData = req->Data();
        TCgiParameters cgi(reqData);

        bool isHumanReadable = (cgi.NumOfValues("hr") > 0 && IsTrue(cgi.Get("hr", 0)));
        bool isInfoRequest = (cgi.NumOfValues("info") > 0);
        NSnippets::NProto::TSnipReqParams snipParams;
        NSnippets::NProto::TSnipInfoReqParams& snipInfoParams = *snipParams.MutableInfoRequestParams();
        TDocHandle infoDocId;

        if (Verbose) {
            Cout << "Got " << (isInfoRequest ? "info " : "") << "request: " << reqData << Endl;
        }

        if (isInfoRequest) {
            TInfoParams infoParams;
            infoParams.Init(cgi.Get("info", 0));
            SetSnipConfInfoRequest(infoParams, snipParams);
            infoDocId.FromString(infoParams.Params.Get("docid", ""));
        }

        bool isSnippetInfoRequest = isInfoRequest && (snipInfoParams.GetInfoRequestType() != NSnippets::INFO_NONE);

        if (ResponsePatcher.WantPatch(isInfoRequest, isSnippetInfoRequest)) {
            cgi.EraseAll("info");
            cgi.EraseAll("hr");
            ResponsePatcher.PatchCgi(cgi);
        }

        TString r = "http://" + MiddleService + cgi.Print();

        if (Verbose) {
            Cout << "Will ask: " << r << Endl;
        }

        NNeh::TResponseRef resp = NNeh::Request(r)->Wait();

        if (!resp || resp->IsError()) {
            const char* empty = "";
            NNeh::TData resd(empty, empty);
            req->SendReply(resd);
            return;
        }

        TStringBuf q = resp->Data;

        if (!ResponsePatcher.WantPatch(isInfoRequest, isSnippetInfoRequest)) {
            NNeh::TData resd(q.data(), q.data() + (q.size()));

            if (Verbose) {
                Cout << "Reply passthrough" << Endl;
            }

            req->SendReply(resd);
            return;
        }

        NMetaProtocol::TReport rep;
        TFactorMapping factorMapping;
        typedef TVector<NMetaProtocol::TDocument*> TDocVec;
        TDocVec docs;

        if (rep.ParseFromArray(q.data(), q.size())) {
            NMetaProtocol::Decompress(rep); // decompresses only if needed
            if (rep.HasHead()) {
                NMetaProtocol::THead& head = *rep.MutableHead();
                for (size_t i = 0; i < head.FactorMappingSize(); ++i) {
                    factorMapping[head.GetFactorMapping(i).GetKey()] = head.GetFactorMapping(i).GetValue();
                }
                if (!!RealMiddlesnip) {
                    TString searchInfo = head.GetSearchInfo();
                    NSc::TValue si = NSc::TValue::FromJson(searchInfo);
                    si["shared-data"][0] = TString("http://") + RealMiddlesnip + "/yandsearch?";
                    head.SetSearchInfo(si.ToJson());
                }
            }
            for (size_t i = 0; i < rep.GroupingSize(); ++i) {
                for (size_t j = 0; j < rep.GetGrouping(i).GroupSize(); ++j) {
                    if (Position) {
                        if (Position > rep.GetGrouping(i).GroupSize()) {
                            rep.MutableGrouping(i)->ClearGroup();
                        } else {
                            NMetaProtocol::TGroup gg = rep.GetGrouping(i).GetGroup(Position - 1);
                            rep.MutableGrouping(i)->ClearGroup();
                            *rep.MutableGrouping(i)->AddGroup() = gg;
                        }
                    }
                    for (size_t k = 0; k < rep.GetGrouping(i).GetGroup(j).DocumentSize(); ++k) {
                        docs.push_back(rep.MutableGrouping(i)->MutableGroup(j)->MutableDocument(k));
                    }
                }
            }
        }

        if (!isInfoRequest) {
            for (TDocVec::iterator ii = docs.begin(), end = docs.end(); ii != end; ++ii) {
                if (Verbose) {
                    Cout << "Will patch: " << ExtractUrl(**ii) << Endl;
                }
                ResponsePatcher.PatchDoc(**ii, factorMapping, cgi, RuFactSnippetDssmApplier, TomatoDssmApplier);
            }

            if (isHumanReadable) {
                TString s = rep.Utf8DebugString();
                NNeh::TData resd(s.data(), s.data() + (s.size()));
                req->SendReply(resd);
            }
            else {
                TBufferOutput res;
                rep.SerializeToArcadiaStream(&res);
                NNeh::TData resd(res.Buffer().Data(), res.Buffer().Data() + res.Buffer().Size());
                req->SendReply(resd);
            }
        }
        else {
            TString result = "document missing";
            for (TDocVec::iterator ii = docs.begin(), end = docs.end(); ii != end; ++ii) {
                NMetaProtocol::TDocument& doc = **ii;
                const TDocHandle docId =
                    doc.HasDocId() ?
                    TDocHandle(doc.GetDocId()) :
                    TDocHandle(doc.GetDocHash(), doc.GetRoute());

                if (docId == infoDocId) {
                    result = ResponsePatcher.GetSnippetsExplanation(doc, &snipInfoParams, factorMapping, cgi, RuFactSnippetDssmApplier, TomatoDssmApplier);
                    break;
                }
            }
            NNeh::TData resd(result.data(), result.data() + (result.size()));
            req->SendReply(resd);
        }
    }
};

static void GetSomeMiddlesearch(TString& middle, TString& service, const TString& xmlsearchScheme, const TString& xmlsearch, ui16 port, const TString& source, const TString& xmltype) {
    middle = "";
    service = "";
    NSnippets::THost h;
    h.Scheme = xmlsearchScheme;
    h.Host = xmlsearch;
    h.Port = port;
    h.FillAddr();
    NSnippets::NXmlSearchIn::TRequest res;
    if (!h.XmlSearch(res, "test", source, xmltype, "")) {
        return;
    }
    TStringBuf scheme;
    TStringBuf hostport;
    TStringBuf path;
    TStringBuf cgi;
    if (!NSnippets::ParseUrl(res.FullQuery, scheme, hostport, path, cgi) || scheme != "http://" && scheme != "http2://") {
        return;
    }
    if (path.size() < 2) {
        return;
    }
    middle = TString(hostport.data(), path.data() + path.size());
    service = TString(path.data() + 1, path.size() - 2);
}

static void LoadDssm(NLastGetopt::TOpts& opt, NLastGetopt::TOptsParseResult& o, NLastGetopt::TOpt& d, THolder<NNeuralNetApplier::TModel>& dssmApplier) {
    TString dssmModelFilename = NLastGetopt::GetOrElse_(opt, o, &d, "");
    if (dssmModelFilename) {
        if (Verbose) {
            Cout << "Reading dssm model " << dssmModelFilename << Endl;
        }
        dssmApplier.Reset(new NNeuralNetApplier::TModel);
        dssmApplier->Load(TBlob::FromFile(dssmModelFilename));
        dssmApplier->Init();
    }
}

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    int port;
    TString middle;
    TString service = "yandsearch";
    TString xmltype;
    TString realMiddlesnip;

    using namespace NLastGetopt;
    TOpts opt;
    TOpt& v = opt.AddLongOption("version").HasArg(NO_ARGUMENT);
    TOpt& verb = opt.AddCharOption('v', NO_ARGUMENT, "verbose output");
    TOpt& cgi = opt.AddCharOption('c', NO_ARGUMENT, "patch context snipparams with treatcgi");

    TOpt& p = opt.AddCharOption('p', REQUIRED_ARGUMENT, "port to listen");
    TOpt& m = opt.AddCharOption('m', REQUIRED_ARGUMENT, "middlesearch host:port[/service], auto or <empty> to get WEB one from xmlsearch.hamster.yandex.ru, autoWEB, autoIMAGES etc for other sources");
    TOpt& t = opt.AddCharOption('t', REQUIRED_ARGUMENT, "use &type=<value> cgi parameter to use for middlesearch autodetection, like pictures");
    TOpt& s = opt.AddCharOption('s', REQUIRED_ARGUMENT, "path/filename of the stopword list file (just get the stopwords.lst file from any production shard)");
    TOpt& E = opt.AddCharOption('E', REQUIRED_ARGUMENT, "baseExp");
    TOpt& r = opt.AddCharOption('r', REQUIRED_ARGUMENT, "dump ctxs to <filename>");
    TOpt& R = opt.AddCharOption('R', REQUIRED_ARGUMENT, "dump mangled ctxs to <filename>");
    TOpt& g = opt.AddCharOption('g', REQUIRED_ARGUMENT, "take only g-th group, 1-based");
    TOpt& j = opt.AddCharOption('j', REQUIRED_ARGUMENT, "number of threads");
    TOpt& d = opt.AddCharOption('d', REQUIRED_ARGUMENT, "dssm model path/filename. Download it with svn export svn+ssh://arcadia.yandex.ru/robots/branches/base/dynamic_ranking_models/production/base/RuFactSnippet.dssm ./RuFactSnippet.dssm");
    TOpt& D = opt.AddCharOption('D', REQUIRED_ARGUMENT, "TOMATO dssm model path/filename. Download it with svn export svn+ssh://arcadia.yandex.ru/robots/branches/base/dynamic_ranking_models/production/base/256.128.otvet_mail_ru.toloka.compressed.dssm ./256.128.otvet_mail_ru.toloka.compressed.dssm");
    TOpt& M = opt.AddCharOption('M', NO_ARGUMENT, "DO NOT replace the metasearch hostname with the middlesnip host:port (will break cookie inforequests)");

    opt.SetFreeArgsNum(0);

    TOptsParseResult o(&opt, argc, argv);

    if (Has_(opt, o, &verb)) {
        Verbose = true;
    }
    if (Has_(opt, o, &cgi)) {
        UseCgiPatch = true;
    }
    if (Has_(opt, o, &v)) {
        Cout << GetProgramSvnVersion() << Endl;
        return 0;
    }
    if (!Has_(opt, o, &p)) {
        Cerr << "missing required argument: -p port" << Endl;
        return 1;
    }
    unsigned int gg = FromString<unsigned int>(GetOrElse_(opt, o, &g, "0"));
    port = FromString<int>(Get_(opt, o, &p));
    middle = GetOrElse_(opt, o, &m, "");
    xmltype = GetOrElse_(opt, o, &t, "");
    if (!Has_(opt, o, &M)) {
        realMiddlesnip = TStringBuilder() << HostName() << ":" << port;
    }
    if (!middle.size() || middle.StartsWith("auto")) {
        if (middle.StartsWith("auto") && middle != "auto") {
            middle = middle.substr(4);
        } else {
            middle = "WEB";
        }
        if (Verbose) {
            Cout << "detecting middlesearch..." << Endl;
        }
        TString middletype = middle;
        GetSomeMiddlesearch(middle, service, "https://", "xmlsearch.hamster.yandex.ru", 443, middletype, xmltype);
        if (!middle.size()) {
            Cerr << "failed to autodetect middlesearch host:port" << Endl;
            return 1;
        }
        if (Verbose) {
            Cout << "detected middlesearch: " << middle << Endl;
        }
    } else {
        size_t p = middle.find('/');
        if (p == TString::npos) { //use default service
            middle = middle + "/" + service + "?";
        } else { //extract service
            service = TString(middle.data() + p + 1, middle.data() + middle.size());
            if (!middle.EndsWith('?')) {
                middle = middle + "?";
            }
        }
    }
    THolder<NNeuralNetApplier::TModel> ruFactSnippetDssmApplier;
    LoadDssm(opt, o, d, ruFactSnippetDssmApplier);
    THolder<NNeuralNetApplier::TModel> tomatoDssmApplier;
    LoadDssm(opt, o, D, tomatoDssmApplier);

    TString stopWordsFile = GetOrElse_(opt, o, &s, "");

    TSnippetPatcher patcher;
    TSrv srv(middle, patcher, gg, realMiddlesnip, ruFactSnippetDssmApplier.Get(), tomatoDssmApplier.Get());
    if (!!stopWordsFile) {
        patcher.StopWords.InitStopWordsList(stopWordsFile.data());
    } else {
        NSnippets::InitDefaultStopWordsList(patcher.StopWords);
    }
    patcher.BaseExp = GetOrElse_(opt, o, &E, "");
    THolder<TUnbufferedFileOutput> out;
    TString fout = GetOrElse_(opt, o, &r, "");
    if (fout.size()) {
        out.Reset(new TUnbufferedFileOutput(fout));
        patcher.CtxOut = out.Get();
    }
    THolder<TUnbufferedFileOutput> rout;
    TString frout = GetOrElse_(opt, o, &R, "");
    if (frout.size()) {
        rout.Reset(new TUnbufferedFileOutput(frout));
        patcher.CtxROut = rout.Get();
    }

    size_t nThreads = FromString<size_t>(GetOrElse_(opt, o, &j, "1"));

    NNeh::IServicesRef sref = NNeh::CreateLoop();
    sref->Add(TString("http://*:") + ToString(port) + "/" + service, srv);
    sref->Loop(nThreads);
    return 0;
}
