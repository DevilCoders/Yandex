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

#include <kernel/qtree/richrequest/richnode.h>

#include <kernel/snippets/idl/enums.h>
#include <kernel/snippets/idl/snippets.pb.h>

#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/urlcut/url_wanderer.h>

#include <search/idl/meta.pb.h>
#include <search/reqparam/reqparam.h>
#include <search/reqparam/snipreqparam.h>
#include <search/session/compression/report.h>

#include <yweb/robot/kiwi/clientlib/client.h>

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcio.h>
#include <kernel/walrus/advmerger.h>

#include <library/cpp/svnversion/svnversion.h>

#include <library/cpp/neh/rpc.h>
#include <library/cpp/neh/neh.h>

#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/network/socket.h>
#include <library/cpp/http/io/stream.h>
#include <util/stream/mem.h>
#include <util/stream/output.h>
#include <util/stream/file.h>

static bool Verbose = false;

struct TSrv {
    NKiwi::TSession Session;
    TString KiwiReq;
    const TString MiddleService;
    TWordFilter StopWords;
    TString BaseExp;
    IOutputStream* CtxOut;
    IOutputStream* CtxROut;

    TSrv(const TString& middleService, const TString& kiwiReq)
      : Session(100, 100000000, 30)
      , KiwiReq(kiwiReq)
      , MiddleService(middleService)
      , CtxOut(nullptr)
      , CtxROut(nullptr)
    {
    }

    TStringBuf ReadBuf(TStringBuf& raw) {
        Y_ASSERT(raw.size() >= sizeof(ui32));
        const ui32 bufferSize = *reinterpret_cast<const ui32*>(raw.data());
        raw.Skip(sizeof(ui32));
        Y_ASSERT(raw.size() >= bufferSize);
        TStringBuf res(raw.data(), bufferSize);
        raw.Skip(bufferSize);
        return res;
    }
    void ReadKeyInvs(TStringBuf raw, TVector<TPortionBuffers>& res) {
        while (raw.size()) {
            TStringBuf key = ReadBuf(raw);
            TStringBuf inv = ReadBuf(raw);
            res.push_back(TPortionBuffers(key.data(), key.size(), inv.data(), inv.size()));
        }
    }
    bool ParseKiwi(const NKiwi::TKiwiObject& data, TStringBuf name, TStringBuf& val) {
        const NKiwi::NTuples::TTuple* t = data.FindByLabel(name.data(), nullptr, true);
        return t && t->GetValue(val);
    }
    bool ParseKiwi(const NKiwi::TKiwiObject& data, TStringBuf name, TString& val) {
        TStringBuf res;
        if (ParseKiwi(data, name, res)) {
            val.assign(res.data(), res.size());
            return true;
        }
        return false;
    }
    bool ParseKiwi(const NKiwi::TKiwiObject& data, TStringBuf name, ui8& val) {
        const NKiwi::NTuples::TTuple* t = data.FindByLabel(name.data(), nullptr, true);
        return t && t->GetValue(val);
    }
    bool ParseKiwi(const NKiwi::TKiwiObject& data, TStringBuf name, i32& val) {
        const NKiwi::NTuples::TTuple* t = data.FindByLabel(name.data(), nullptr, true);
        return t && t->GetValue(val);
    }

    typedef std::pair<int, float> TFactor;
    typedef TVector<TFactor> TFactors;
    typedef THashMap<TStringBuf, int> TFactorMapping;
    void AddNewStuff(NMetaProtocol::TArchiveInfo& ar, NSnippets::NProto::TSnippetsCtx& ctx, const TCgiParameters& cgi, const TFactorMapping& mapping, const TFactors& factors, const TString& url, const TString& lastAccess, const TString& modTime) {
        (void)ctx;
        (void)cgi;
        (void)mapping;
        (void)factors;
        (void)modTime;
        THashSet<TString> gtas;
        for (size_t i = 0; i < ar.GtaRelatedAttributeSize(); ++i) {
            gtas.insert(ar.GetGtaRelatedAttribute(i).GetKey());
        }
        ar.ClearGtaRelatedAttribute();
        ctx.MutableTextArc()->SetData(TString());
        ctx.MutableTextArc()->SetExtInfo(TString());
        if (!url.size()) {
            return;
        }
        TString kiwiUrl = AddSchemePrefix(url);
        TString kiwiReq = KiwiReq;
        TString ver;
        if (lastAccess.size()) {
            ver = ":0:" + lastAccess;
        }
        while (kiwiReq.find(":VER") != TString::npos) {
            kiwiReq.replace(kiwiReq.find(":VER"), 4, ver);
        }
        NKiwi::TSyncReader reader(Session, NKwTupleMeta::KT_DOC_DEF, kiwiReq, nullptr);
        NKiwi::TKiwiObject data;
        TString err;
        NKiwi::TReaderBase::EReadStatus res = reader.Read(kiwiUrl, data, NKiwi::TReaderBase::READ_FASTEST, &err);
        if (res == NKiwi::TReaderBase::READ_OK || res == NKiwi::TReaderBase::READ_PARTIAL) {
            TStringBuf arc;
            TBlob extInfo;
            TBlob dataBlob;
            if (ParseKiwi(data, "arc", arc) && arc.size()) {
                const TArchiveHeader* arcData = (const TArchiveHeader*)arc.data();
                extInfo = GetArchiveExtInfo(arcData);
                dataBlob = GetArchiveDocText(arcData);
            } else {
                Cerr << "kiwi has no arc for " << url << " (v" << ver << ")" << Endl;
            }
            ctx.MutableTextArc()->SetData(TString(dataBlob.AsCharPtr(), dataBlob.Size()));
            ctx.MutableTextArc()->SetExtInfo(TString(extInfo.AsCharPtr(), extInfo.Size()));
            { //part of mergearchive stuff
                ui8 lang = 0;
                ui8 mime = 0;
                ui8 encoding = 0;
                i32 lastAccess = 0;
                i32 modTime = 0;
                TString imagessnip;
                TString ruwiki;
                TString ruwikisitelinks;
                TString enwiki;
                TString enwikisitelinks;
                TString yaca;
                TString yacatr;
                TString sitelinks;
                TString market;
                TString vthumb;
                ParseKiwi(data, "lang", lang);
                ParseKiwi(data, "la", lastAccess);
                ParseKiwi(data, "mt", modTime);
                ParseKiwi(data, "imagessnip", imagessnip);
                ParseKiwi(data, "ruwiki", ruwiki);
                ParseKiwi(data, "ruwikisitelinks", ruwikisitelinks);
                ParseKiwi(data, "enwiki", enwiki);
                ParseKiwi(data, "enwikisitelinks", enwikisitelinks);
                ParseKiwi(data, "yaca", yaca);
                ParseKiwi(data, "yacatr", yacatr);
                ParseKiwi(data, "sitelinks", sitelinks);
                ParseKiwi(data, "market", market);
                ParseKiwi(data, "vthumb", vthumb);

                const TString extInfo = *ctx.MutableTextArc()->MutableExtInfo();
                TBuffer old(extInfo.data(), extInfo.size());
                TDocDescr docDescr;
                docDescr.UseBlob(&old);

                if (ParseKiwi(data, "mime", mime)) {
                    docDescr.set_mimetype((MimeTypes)mime);
                }
                if (!ParseKiwi(data, "encoding", encoding)) {
                    encoding = (ui8)docDescr.get_encoding();
                }

                TDocInfos docInfos;
                docDescr.ConfigureDocInfos(docInfos);
                if (lang) {
                    const char *langName = IsoNameByLanguage(static_cast<ELanguage>(lang));
                    if (langName && *langName) {
                        docInfos["lang"] = langName;
                    }
                }
                TString laStr;
                if (lastAccess) {
                    laStr = ToString(lastAccess);
                    docInfos["lastaccess"] = laStr.data();
                }
                if (imagessnip.size()) {
                    docInfos["imagessnip"] = imagessnip.data();
                }
                if (ruwiki.size() && ruwiki.StartsWith("wiki=")) {
                    docInfos["wiki"] = ruwiki.data() + 5;
                } else if (enwiki.size() && enwiki.StartsWith("wiki=")) {
                    docInfos["wiki"] = enwiki.data() + 5;
                }
                if (enwikisitelinks.size() && enwikisitelinks.StartsWith("enwiki_sitelinks")) {
                    docInfos["enwiki_sitelinks"] = enwikisitelinks.data() + 17;
                }
                if (ruwikisitelinks.size() && ruwikisitelinks.StartsWith("ruwiki_sitelinks")) {
                    docInfos["ruwiki_sitelinks"] = ruwikisitelinks.data() + 17;
                }
                if (yaca.size() && yaca.StartsWith("catalog=")) {
                    docInfos["catalog"] = yaca.data() + 8;
                }
                if (yacatr.size() && yacatr.StartsWith("yaca_tr=")) {
                    docInfos["yaca_tr"] = yacatr.data() + 8;
                }
                if (sitelinks.size() && sitelinks.StartsWith("sitelinks=")) {
                    docInfos["sitelinks"] = sitelinks.data() + 10;
                }
                if (market.size() && market.StartsWith("market=")) {
                    docInfos["market"] = market.data() + 7;
                }
                if (vthumb.size() && vthumb.StartsWith("vthumb=")) {
                    docInfos["vthumb"] = vthumb.data() + 7;
                }

                for (THashSet<TString>::const_iterator it = gtas.begin(); it != gtas.end(); ++it) {
                    if (docInfos.find(it->data()) != docInfos.end()) {
                        NMetaProtocol::TPairBytesBytes* g = ar.AddGtaRelatedAttribute();
                        g->SetKey(*it);
                        g->SetValue(docInfos[it->data()]);
                    }
                }
                TString mimes = ToString((ui8)docDescr.get_mimetype()) + " ";
                {
                    NMetaProtocol::TPairBytesBytes* g = ar.AddGtaRelatedAttribute();
                    g->SetKey("_MimeType");
                    g->SetValue(mimes);
                }

                TDocInfoExtWriter ew;
                for (TDocInfos::const_iterator toDI = docInfos.begin(); toDI != docInfos.end(); ++toDI) {
                    ew.Add(toDI->first, toDI->second);
                }
                docDescr.RemoveExtension(DocInfo);
                ew.Write(docDescr);
                TBuffer blob(DEF_ARCH_BLOB);
                TDocDescr dd;
                dd.UseBlob(&blob);
                dd.CopyUrlDescr(docDescr);
                dd.SetUrlAndEncoding(url.data(), (ECharset)encoding);
                if (modTime) {
                    docDescr.set_mtime(modTime);
                }
                dd.CopyExtensions(docDescr);
                ctx.MutableTextArc()->SetExtInfo(blob.Data(), blob.Size());
            }
            TStringBuf keyinv;
            if (ParseKiwi(data, "keyinv", keyinv)) {
                TVector<TPortionBuffers> portions;
                ReadKeyInvs(keyinv, portions);
                if (!portions.empty()) {
                    NIndexerCore::TMemoryPortion res(IYndexStorage::FINAL_FORMAT);
                    MergeMemoryPortions(&portions[0], portions.size(), IYndexStorage::FINAL_FORMAT, nullptr, false, res);
                    // do stuff
                }
            } else {
                Cerr << "kiwi has no keyinv for " << url << " (v" << ver << ")" << Endl;
            }
        } else {
            Cerr << "kiwi error (" << (int)res << ") for " << url << "(v" << ver << "): " << err << Endl;
        }
    }

    TString GetGta(const NMetaProtocol::TArchiveInfo& ar, const TStringBuf& key) {
        for (size_t i = 0; i < ar.GtaRelatedAttributeSize(); ++i) {
            if (ar.GetGtaRelatedAttribute(i).GetKey() == key) {
                return ar.GetGtaRelatedAttribute(i).GetValue();
            }
        }
        return TString();
    }
    TString GetLastAccess(const NMetaProtocol::TArchiveInfo& ar) {
        return GetGta(ar, "lastaccess");
    }
    TString GetModTime(const NMetaProtocol::TArchiveInfo& ar) {
        return GetGta(ar, "_ModTime");
    }

    void PassageReply(NMetaProtocol::TDocument& doc, NSnippets::TPassageReply& reply, const NSnippets::NProto::TSnipInfoReqParams* infoParams, const TFactorMapping& factorMapping, const TCgiParameters& cgi) {
        if (!doc.HasArchiveInfo()) {
            return;
        }
        TFactors factors; // some of them, should be asked for
        for (size_t i = 0; i < doc.BinFactorSize(); ++i) {
            factors.push_back(TFactor(doc.GetBinFactor(i).GetKey(), doc.GetBinFactor(i).GetValue()));
        }
        NMetaProtocol::TArchiveInfo& ar = *doc.MutableArchiveInfo();
        TStringBuf ctxs = NSnippets::FindCtx(ar);
        if (!ctxs.size()) {
            return;
        }
        if (CtxOut) {
            (*CtxOut) << ctxs << Endl;
        }
        TString raw = Base64Decode(ctxs);

        NSnippets::NProto::TSnippetsCtx ctx;
        if (!ctx.ParseFromString(raw)) {
            return;
        }
        if (infoParams && ctx.HasReqParams()) {
            ctx.MutableReqParams()->MutableInfoRequestParams()->CopyFrom(*infoParams);
        }
        AddNewStuff(ar, ctx, cgi, factorMapping, factors, (ar.HasUrl() ? ar.GetUrl() : ""), GetLastAccess(ar), GetModTime(ar));
        if (CtxROut) {
            TBufferOutput res;
            ctx.SerializeToArcadiaStream(&res);
            TString rctxs = Base64Encode(TStringBuf(res.Buffer().data(), res.Buffer().size()));
            (*CtxROut) << rctxs << Endl;
        }
        PassageReply(ctx, reply);
    }

    void PassageReply(const NSnippets::NProto::TSnippetsCtx& ctx, NSnippets::TPassageReply& reply) {
        NSnippets::TConfigParams cfgp;
        cfgp.StopWords = &StopWords;
        if (ctx.HasReqParams()) {
            cfgp.SRP = &ctx.GetReqParams();
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
        TRichTreePtr richtree;
        TInlineHighlighter ih;
        if (ctx.HasQuery() && ctx.GetQuery().HasQtreeBase64()) {
            richtree = DeserializeRichTree(DecodeRichTreeBase64(ctx.GetQuery().GetQtreeBase64()));
        }
        if (richtree.Get() && richtree->Root.Get()) {
            ih.AddRequest(*richtree->Root);
        }

        NUrlCutter::TRichTreeWanderer wanderer(richtree);
        const NSnippets::TPassageReplyContext args(cfgp, hitsInfo, ih, arcCtx, richtree, nullptr, wanderer);

        TAutoPtr<NSnippets::IPassageReplyFiller> passageReplyFiller = CreatePassageReplyFiller(args);
        passageReplyFiller->FillPassageReply(reply);
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

        bool opaqueRequest = isInfoRequest && (snipInfoParams.GetInfoRequestType() == NSnippets::INFO_NONE);

        if (!opaqueRequest) {
            cgi.EraseAll("info");
            cgi.EraseAll("hr");
            cgi.InsertUnescaped("gta", "lastaccess");
            cgi.InsertUnescaped("gta", "_ModTime");
            NSnippets::AskSnippetsCtx(cgi);
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

        if (opaqueRequest) {
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
                const NMetaProtocol::THead& head = rep.GetHead();
                for (size_t i = 0; i < head.FactorMappingSize(); ++i) {
                    factorMapping[head.GetFactorMapping(i).GetKey()] = head.GetFactorMapping(i).GetValue();
                }
            }
            for (size_t i = 0; i < rep.GroupingSize(); ++i) {
                for (size_t j = 0; j < rep.GetGrouping(i).GroupSize(); ++j) {
                    for (size_t k = 0; k < rep.GetGrouping(i).GetGroup(j).DocumentSize(); ++k) {
                        docs.push_back(rep.MutableGrouping(i)->MutableGroup(j)->MutableDocument(k));
                    }
                }
            }
        }
        if (!isInfoRequest) {
            for (TDocVec::iterator ii = docs.begin(), end = docs.end(); ii != end; ++ii) {
                NSnippets::TPassageReply reply;
                PassageReply(**ii, reply, nullptr, factorMapping, cgi);
                NSnippets::PatchByPassageReply(**ii, reply);
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
                    NSnippets::TPassageReply reply;
                    PassageReply(doc, reply, &snipInfoParams, factorMapping, cgi);
                    result = reply.GetSnippetsExplanation();
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

int main(int argc, char** argv) {
    int port;
    TString middle;
    TString service = "yandsearch";
    TString xmltype;
    using namespace NLastGetopt;
    TOpts opt;
    TOpt& v = opt.AddLongOption("version").HasArg(NO_ARGUMENT);
    TOpt& verb = opt.AddCharOption('v', NO_ARGUMENT, "verbose output");

    TOpt& p = opt.AddCharOption('p', REQUIRED_ARGUMENT, "port to listen");
    TOpt& m = opt.AddCharOption('m', REQUIRED_ARGUMENT, "middlesearch host:port[/service], auto or <empty> to get WEB one from xmlsearch.hamster.yandex.ru, autoWEB, autoIMAGES etc for other sources");
    TOpt& t = opt.AddCharOption('t', REQUIRED_ARGUMENT, "use &type=<value> cgi parameter to use for middlesearch autodetection, like pictures");
    TOpt& s = opt.AddCharOption('s', REQUIRED_ARGUMENT, "path/filename of the stopword list file (just get the stopwords.lst file from any production shard)");
    TOpt& E = opt.AddCharOption('E', REQUIRED_ARGUMENT, "baseExp");
    TOpt& r = opt.AddCharOption('r', REQUIRED_ARGUMENT, "dump ctxs to <filename>");
    TOpt& R = opt.AddCharOption('R', REQUIRED_ARGUMENT, "dump mangled ctxs to <filename>");

    opt.SetFreeArgsNum(0);

    TOptsParseResult o(&opt, argc, argv);

    if (Has_(opt, o, &verb)) {
        Verbose = true;
    }
    if (Has_(opt, o, &v)) {
        Cout << GetProgramSvnVersion() << Endl;
        return 0;
    }
    if (!Has_(opt, o, &p)) {
        Cerr << "missing required argument: -p port" << Endl;
        return 1;
    }
    port = FromString<int>(Get_(opt, o, &p));
    middle = GetOrElse_(opt, o, &m, "");
    xmltype = GetOrElse_(opt, o, &t, "");
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
    TString stopWordsFile = GetOrElse_(opt, o, &s, "");

    TString kiwi = "prewalrus = Prewalrus_robot_stable_2014_08_19(); return prewalrus[14] as keyinv, prewalrus[15] as arc, $Language:VER as lang, $LastAccess:VER as la, $HttpModTime:VER as mt, ?SnippetImages:VER as imagessnip, ?SnippetRuwiki as ruwiki, ?SnippetEnwiki as enwiki, ?SnippetEnwikiSitelinks as enwikisitelinks, ?SnippetCatalog as yaca, ?SnippetSitelinks as sitelinks, ?SnippetRuwikiSitelinks as ruwikisitelinks, ?SnippetMarket as market, ?SnippetVideo as vthumb, $MimeType:VER as mime, $Encoding:VER as encoding, ?SnippetYacaTr as yacatr;";
    TSrv srv(middle, kiwi);
    if (!!stopWordsFile) {
        srv.StopWords.InitStopWordsList(stopWordsFile.data());
    } else {
        NSnippets::InitDefaultStopWordsList(srv.StopWords);
    }
    srv.BaseExp = GetOrElse_(opt, o, &E, "");
    THolder<TUnbufferedFileOutput> out;
    TString fout = GetOrElse_(opt, o, &r, "");
    if (fout.size()) {
        out.Reset(new TUnbufferedFileOutput(fout));
        srv.CtxOut = out.Get();
    }
    THolder<TUnbufferedFileOutput> rout;
    TString frout = GetOrElse_(opt, o, &R, "");
    if (frout.size()) {
        rout.Reset(new TUnbufferedFileOutput(frout));
        srv.CtxROut = rout.Get();
    }

    NNeh::IServicesRef sref = NNeh::CreateLoop();
    sref->Add(TString("http://*:") + ToString(port) + "/" + service, srv);
    sref->Loop(1);
    return 0;
}
