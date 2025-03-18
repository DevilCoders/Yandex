#include <tools/snipmake/argv/opt.h>
#include <tools/snipmake/snipdat/askctx.h>
#include <tools/snipmake/snipdat/metahost.h>
#include <tools/snipmake/stopwordlst/stopword.h>
#include <tools/snipmake/protosnip/patch.h>

#include <kernel/snippets/base/default_snippetizer.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/idl/snippets.pb.h>
#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/iface/passagereplyfiller.h>
#include <kernel/snippets/hits/ctx.h>
#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/urlcut/url_wanderer.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <kernel/search_daemon_iface/dp.h>

#include <search/idl/meta.pb.h>
#include <search/session/compression/report.h>

#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/neh/rpc.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/ptr.h>
#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/subst.h>
#include <util/stream/buffer.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/stream/mem.h>
#include <util/stream/zlib.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

static bool Verbose = false;
static bool VeryVerbose = false;
static bool JustWatch = false;
TWordFilter StopWords;

static void PassageReply(const NSnippets::NProto::TSnippetsCtx& ctx, NSnippets::TPassageReply& reply) {
    NSnippets::TConfigParams cfgp;
    cfgp.StopWords = &StopWords;
    bool ignoreRegionPhrase = false;
    if (ctx.HasReqParams()) {
        const NSnippets::NProto::TSnipReqParams& rp = ctx.GetReqParams();
        if (rp.HasIgnoreRegionPhrase()) {
            ignoreRegionPhrase = rp.GetIgnoreRegionPhrase();
        }
        cfgp.SRP = &rp;
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
    NSnippets::TPassageReplyContext args(cfgp, hitsInfo, ih, arcCtx, richtree, regionPhraseRichtree, wanderer);

    TAutoPtr<NSnippets::IPassageReplyFiller> passageReplyFiller = CreatePassageReplyFiller(args);
    passageReplyFiller->FillPassageReply(reply);
}

static void AskSnippetsCtx(NJson::TJsonValue* clientCtx, TCgiParameters& globalCtx) {
    globalCtx.InsertUnescaped("gta", "_SnippetsCtx");
    if (!clientCtx) {
        return;
    }
    const NJson::TJsonValue::TMapType& m = clientCtx->GetMap();
    for (auto it : m) {
        TString snip = "wantctx=da";
        const NJson::TJsonValue* snp;
        if (it.second.GetValuePointer("snip", &snp) && snp->IsArray()) {
            for (size_t i = 0; i < snp->GetArray().size(); ++i) {
                snip += ";" + snp->GetArray()[i].GetString();
            }
        }
        NJson::TJsonValue a(NJson::JSON_ARRAY);
        a.AppendValue(NJson::TJsonValue(snip));
        (*clientCtx)[it.first]["snip"] = a;
        (*clientCtx)[it.first]["gta"].AppendValue(NJson::TJsonValue("_SnippetsCtx"));
    }
}

static void AskSnippetsCtx(TString& postData) {
    TCgiParameters cgi(postData);
    const TString clientCtxStr = cgi.Get("client_ctx");
    const TString globalCtxStr = cgi.Get("global_ctx");

    TCgiParameters globalCtx(globalCtxStr);

    if (clientCtxStr) {
        NJson::TJsonValue clientCtx;
        TMemoryInput mi(clientCtxStr.data(), clientCtxStr.size());
        if (!NJson::ReadJsonTree(&mi, &clientCtx)) {
            ythrow yexception() << "can't parse client_ctx" << Endl;
        }

        AskSnippetsCtx(&clientCtx, globalCtx);

        cgi.ReplaceUnescaped("client_ctx", NJson::WriteJson(&clientCtx, false));
    } else {
        AskSnippetsCtx(nullptr, globalCtx);
    }

    cgi.ReplaceUnescaped("global_ctx", globalCtx.Print());

    postData = cgi.Print();
}

struct TSrv {
    TString UpperService;
    IOutputStream* CtxOut;

    TSrv(const TString& upperService, IOutputStream* ctxOut)
      : UpperService(upperService)
      , CtxOut(ctxOut)
    {
    }

    void HackCompressed(TString& s) {
        TString decompr;
        try {
            TMemoryInput mi(s.data(), s.size());
            TZLibDecompress z(&mi);
            decompr = z.ReadAll();
        } catch (const TZLibDecompressorError&) {
            return;
        }
        s = decompr;
    }

    void ServeRequest(const NNeh::IRequestRef& req) {
        TStringBuf reqData_ = req->Data();
        TString reqData = TString(reqData_);

        HackCompressed(reqData);

        if (VeryVerbose) {
            Cout << "Got request: " << reqData << Endl;
        } else if (Verbose) {
            Cout << "Got request." << Endl;
        }

        if (!JustWatch) {
            AskSnippetsCtx(reqData);
        }

        NNeh::TMessage r(UpperService, reqData);

        NNeh::TResponseRef resp = NNeh::Request(r)->Wait();

        if (!resp || resp->IsError()) {
            if (resp) {
                Cerr << "upper req error: " << resp->GetErrorText() << Endl;
            }
            const char* empty = "";
            NNeh::TData resd(empty, empty);
            req->SendReply(resd);
            return;
        }

        TStringBuf q = resp->Data;

        if (VeryVerbose) {
            Cout << "Got reply: " << Base64Encode(q) << Endl;
        } else if (Verbose) {
            Cout << "Got reply." << Endl;
        }

        if (!JustWatch) {
            NMetaProtocol::TReport rep;
            if (rep.ParseFromArray(q.data(), q.size())) {
                NMetaProtocol::Decompress(rep); // decompresses only if needed
                for (size_t i = 0; i < rep.GroupingSize(); ++i) {
                    for (size_t j = 0; j < rep.GetGrouping(i).GroupSize(); ++j) {
                        for (size_t k = 0; k < rep.GetGrouping(i).GetGroup(j).DocumentSize(); ++k) {
                            const auto& doc = rep.GetGrouping(i).GetGroup(j).GetDocument(k);
                            TString src;
                            bool hasCtx = false;
                            TString ctxs;
                            if (doc.HasArchiveInfo()) {
                                const NMetaProtocol::TArchiveInfo& ar = doc.GetArchiveInfo();
                                for (size_t l = 0; l < ar.GtaRelatedAttributeSize(); ++l) {
                                    const auto& kv = ar.GetGtaRelatedAttribute(l);
                                    if (kv.GetKey() == "_SnippetsCtx") {
                                        ctxs = kv.GetValue();
                                        hasCtx = true;
                                        if (doc.HasServerDescr()) {
                                            src = doc.GetServerDescr();
                                        }
                                    }
                                }
                            }
                            if (hasCtx) {
                                if (CtxOut) {
                                    (*CtxOut) << src << '\t' << ctxs << Endl;
                                }
                                TString raw = Base64Decode(ctxs);

                                NSnippets::NProto::TSnippetsCtx ctx;
                                if (ctx.ParseFromString(raw)) {
                                    NSnippets::TPassageReply reply;
                                    PassageReply(ctx, reply);
                                    NSnippets::PatchByPassageReply(*rep.MutableGrouping(i)->MutableGroup(j)->MutableDocument(k), reply);
                                }
                            }
                        }
                    }
                }
                if (VeryVerbose) {
                    TString s;
                    google::protobuf::io::StringOutputStream out(&s);
                    google::protobuf::TextFormat::Print(rep, &out);
                    Cout << "Answer: " << s << Endl;
                } else if (Verbose) {
                    Cout << "Answering." << Endl;
                }
                TBufferOutput res;
                rep.SerializeToArcadiaStream(&res);
                NNeh::TData resd(res.Buffer().data(), res.Buffer().data() + (res.Buffer().size()));

                req->SendReply(resd);
                return;
            }
        }

        NNeh::TData resd(q.data(), q.data() + (q.size()));

        req->SendReply(resd);
        return;
    }
};

static TString GetSomeUpperService(const TString& domain) {
    TCgiParameters cgi;
    cgi.InsertUnescaped("text", "test");
    NSnippets::AskReportEventlog(cgi);
    TString report = "http://www.hamster.yandex." + domain + "/search/?" + cgi.Print();

    NNeh::TResponseRef resp = NNeh::Request(report)->Wait();
    if (!resp || resp->IsError()) {
        ythrow yexception() << "can't get eventlog for test query";
    }
    TStringBuf q = resp->Data;
    NJson::TJsonValue v;
    TMemoryInput mi(q.data(), q.size());
    if (!NJson::ReadJsonTree(&mi, &v)) {
        ythrow yexception() << "json_dump isn't json";
    }
    NSnippets::TUpperReq u;
    if (!NSnippets::ExtractReportUpperReq(v, u)) {
        ythrow yexception() << "can't get upper req from json_dump";
    }
    return "post://" + u.Host + ":" + ToString(u.Port) + u.Path;
}

int main(int argc, char** argv) {
    int port;
    using namespace NLastGetopt;
    TOpts opt;
    TOpt& v = opt.AddLongOption("version").HasArg(NO_ARGUMENT);
    TOpt& verb = opt.AddCharOption('v', NO_ARGUMENT, "verbose output");
    TOpt& vverb = opt.AddLongOption("vv", "extra verbose output").HasArg(NO_ARGUMENT);
    TOpt& w = opt.AddLongOption("spy", "transparent mode, no snippet replace or ctx fetching").HasArg(NO_ARGUMENT);

    TOpt& dom = opt.AddCharOption('d', REQUIRED_ARGUMENT, "search domain for uppersearch detection (ru, com, tr, ...)");
    TOpt& p = opt.AddCharOption('p', REQUIRED_ARGUMENT, "port to listen");
    TOpt& r = opt.AddCharOption('r', REQUIRED_ARGUMENT, "dump ctxs to <filename>");
    opt.SetFreeArgsNum(0);

    TOptsParseResult o(&opt, argc, argv);

    if (Has_(opt, o, &v)) {
        Cout << GetProgramSvnVersion() << Endl;
        return 0;
    }
    if (!Has_(opt, o, &p)) {
        Cerr << "missing required argument: -p port" << Endl;
        return 1;
    }
    if (Has_(opt, o, &verb)) {
        Verbose = true;
    }
    if (Has_(opt, o, &vverb)) {
        Verbose = true;
        VeryVerbose = true;
    }
    if (Has_(opt, o, &w)) {
        JustWatch = true;
    }
    NSnippets::InitDefaultStopWordsList(StopWords);
    THolder<TUnbufferedFileOutput> out;
    TString fout = GetOrElse_(opt, o, &r, "");
    if (fout.size()) {
        out.Reset(new TUnbufferedFileOutput(fout));
    }
    TString domain = GetOrElse_(opt, o, &dom, "ru");
    port = FromString<int>(Get_(opt, o, &p));
    TString upperService = GetSomeUpperService(domain);
    if (Verbose) {
        Cout << "detected uppersearch: " << upperService << Endl;
    }
    TSrv srv(upperService, out.Get());

    NNeh::IServicesRef sref = NNeh::CreateLoop();
    sref->Add(TString("post://*:") + ToString(port) + "/yandsearch", srv);
    sref->Loop(1);
    return 0;
}
