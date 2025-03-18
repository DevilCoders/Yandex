#include <tools/snipmake/snipdat/askctx.h>
#include <tools/snipmake/snipdat/metahost.h>

#include <tools/snipmake/common/nohtml.h>

#include <kernel/urlid/doc_handle.h>

#include <kernel/snippets/base/default_snippetizer.h>
#include <kernel/snippets/idl/snippets.pb.h>
#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/snippets/iface/passagereplyfiller.h>
#include <kernel/snippets/urlcut/url_wanderer.h>

#include <kernel/qtree/richrequest/printrichnode.h>

#include <search/idl/meta.pb.h>
#include <search/session/compression/report.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>

#include <kernel/search_daemon_iface/dp.h>

#include <util/stream/input.h>
#include <util/stream/mem.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/strip.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {
    struct TProcessor {
        TString Serp;
        TString PreviewSerp;
        bool SomeDoc;
        TDocHandle Docid;
        TString Url;
        TString SerpPreview;
        TString SerpPreviewHtml;
        TString PreviewProto;
        TString SnipdatPreview;
        TString CtxPreview;
        bool Ctx;
        bool CtxImg;
        bool CtxVid;
        bool CtxMainPage;
        bool PreviewLink;
        TProcessor() {
            SomeDoc = false;
        }
        void DocDbg() {
            Cout << "doc" << '\t' << Docid << '\t' << Url << Endl;
            Cout << "serphtml:" << "\n" << SerpPreviewHtml << Endl;
            Cout << "serp:" << "\n" << SerpPreview << Endl;
            Cout << "proto:" << "\n" << PreviewProto << Endl;
            Cout << "snipdat:" << "\n" << SnipdatPreview << Endl;
            Cout << "ctx:" << "\n" << CtxPreview << Endl;
        }
        void StartInput(TStringBuf serpUrl) {
            Serp = serpUrl;
            PreviewSerp.clear();
            EndDoc();
            Cout << "serp" << '\t' << Serp << Endl;
        }
        void OnPreviewInput(TStringBuf serpUrl) {
            PreviewSerp = serpUrl;
            Cout << "pserp" << '\t' << PreviewSerp << Endl;
        }
        void StartDoc(TStringBuf docid, TStringBuf url) {
            EndDoc();
            SomeDoc = true;
            Url.assign(url.data(), url.size());
            Docid.FromString(docid);
        }
        void OnPreview(TStringBuf previewLink, TStringBuf raw64) {
            Y_UNUSED(previewLink);
            SerpPreviewHtml = Base64Decode(raw64);
            SerpPreview = Strip(NoHtml(SerpPreviewHtml));
            PreviewLink = true;
        }
        void OnPreviewProto(TStringBuf metaUrl, TStringBuf raw64) {
            Y_UNUSED(metaUrl);
            OnIdlPreview(PreviewProto, raw64);
        }
        void OnSnipdatPreview(TStringBuf snipdatLink, TStringBuf raw64) {
            Y_UNUSED(snipdatLink);
            OnIdlPreview(SnipdatPreview, raw64);
        }
        bool DocIdMatch(const NMetaProtocol::TDocument& doc) const {
            const TDocHandle docid =
                doc.HasDocId() ?
                TDocHandle(doc.GetDocId()) :
                TDocHandle(doc.GetDocHash(), doc.GetRoute());
            return docid.DocHash == Docid.DocHash;
        }
        bool UrlMatch(TString url) const {
            return AddSchemePrefix(url) == Url;
        }
        bool GotPreview(TString& res, const NMetaProtocol::TDocument& doc) {
            if (!doc.HasArchiveInfo()) {
                return false;
            }
            const NMetaProtocol::TArchiveInfo& ar = doc.GetArchiveInfo();
            if (DocIdMatch(doc) || ar.HasUrl() && UrlMatch(ar.GetUrl())) {
                if (ar.HasTitle() && ar.GetTitle().size()) {
                    res += "Title: " + ar.GetTitle() + "\n";
                }
                if (ar.HasHeadline() && ar.GetHeadline().size()) {
                    res += "Headline: " + ar.GetHeadline() + "\n";
                } else {
                    res.clear();
                }
                for (size_t i = 0; i < ar.GtaRelatedAttributeSize(); ++i) {
                    const NMetaProtocol::TPairBytesBytes& p = ar.GetGtaRelatedAttribute(i);
                    if (p.GetKey() == DP_HEADLINE_SRC && res.size()) {
                        res += "HeadlineSrc: " + p.GetValue() + "\n";
                    }
                }
                return true;
            } else {
                Cout << "nope" << Endl;
                return false;
            }
        }
        void OnIdlPreview(TString& res, TStringBuf raw64) {
            TString inp = Base64Decode(raw64);
            NMetaProtocol::TReport rep;
            if (!rep.ParseFromArray(inp.data(), inp.size())) {
                return;
            }
            NMetaProtocol::Decompress(rep); // decompresses only if needed
            for (size_t i = 0; i < rep.GroupingSize(); ++i) {
                const NMetaProtocol::TGrouping& g = rep.GetGrouping(i);
                for (size_t j = 0; j < g.GroupSize(); ++j) {
                    const NMetaProtocol::TGroup& gg = g.GetGroup(j);
                    for (size_t k = 0; k < gg.DocumentSize(); ++k) {
                        const NMetaProtocol::TDocument& d = gg.GetDocument(k);
                        if (GotPreview(res, d)) {
                            return;
                        }
                    }
                }
            }
        }
        bool IsMain(TStringBuf u) {
            TStringBuf ht = "http://";
            if (u.StartsWith(ht)) {
                u = TStringBuf(u.data() + ht.size(), u.data() + u.size());
            }
            if (u.EndsWith('/')) {
                u = TStringBuf(u.data(), u.data() + u.size() - 1);
            }
            return u.find('/') == TStringBuf::npos;
        }
        void OnCtx(TStringBuf ctxLink, TStringBuf raw64) {
            Y_UNUSED(ctxLink);
            TString raw = Base64Decode(raw64);
            NSnippets::NProto::TSnippetsCtx ctx;
            if (!ctx.ParseFromArray(raw.data(), raw.size())) {
                return;
            }
            Ctx = true;
            TRichTreePtr richtree;
            NSnippets::TPassageReply reply;
            NSnippets::THitsInfoPtr hitsInfo;
            if (ctx.HasHits()) {
                hitsInfo = new NSnippets::THitsInfo();
                hitsInfo->Load(ctx.GetHits());
            }
            NSnippets::TVoidFetcher fetchText;
            NSnippets::TVoidFetcher fetchLink;
            NSnippets::TArcManip arcCtx(fetchText, fetchLink);
            if (ctx.HasTextArc()) {
                arcCtx.GetTextArc().LoadState(ctx.GetTextArc());
            }
            if (ctx.HasLinkArc()) {
                arcCtx.GetLinkArc().LoadState(ctx.GetLinkArc());
            }
            if (ctx.HasQuery()) {
                if (ctx.GetQuery().HasQtreeBase64()) {
                    TString b64QTree = ctx.GetQuery().GetQtreeBase64();
                    richtree = DeserializeRichTree(DecodeRichTreeBase64(b64QTree));
                }
            }

            TInlineHighlighter ih;
            if (richtree.Get() && richtree->Root.Get()) {
                ih.AddRequest(*richtree->Root);
            }

            NSnippets::TConfigParams cfgp;
            bool mainPage = false;
            if (ctx.HasReqParams()) {
                const NSnippets::NProto::TSnipReqParams& rp = ctx.GetReqParams();
                cfgp.SRP = &rp;
                mainPage = rp.HasMainPage() && rp.GetMainPage();
            }
            cfgp.AppendExps.push_back("fstann");
            NUrlCutter::TRichTreeWanderer wanderer(richtree);
            const TPassageReplyContext args(cfgp, hitsInfo, ih, arcCtx, richtree, nullptr, wanderer);

            TAutoPtr<NSnippets::IPassageReplyFiller> passageReplyFiller = CreatePassageReplyFiller(args);
            passageReplyFiller->FillPassageReply(reply);

            if (reply.GetTitle().size()) {
                CtxPreview += "Title: " + WideToUTF8(reply.GetTitle()) + "\n";
            }
            if (reply.GetHeadline().size()) {
                CtxPreview += "Headline: " + WideToUTF8(reply.GetHeadline()) + "\n";
            } else {
                CtxPreview.clear();
            }
            if (CtxPreview.size()) {
                CtxPreview += "HeadlineSrc: " + reply.GetHeadlineSrc() + "\n";
            }
            TDocInfosPtr di = arcCtx.GetTextArc().GetDocInfosPtr();
            TDocInfos::const_iterator arcIt;
            arcIt = di->find("imagessnip");
            CtxImg = arcIt != di->end() && arcIt->second != "{}"sv;
            arcIt = di->find("vthumb");
            CtxVid = arcIt != di->end();
            CtxMainPage = mainPage;
        }
        void EndDoc() {
            if (SomeDoc) {
                Cout << "doc" << '\t' << Docid << '\t' << Url;
                Cout << '\t' << (SerpPreview.size() ? "txt" : "notxt") << '\t' << (SerpPreviewHtml.find("content-preview__image") == TString::npos ? "noimg" : "img") << '\t' << (SerpPreviewHtml.find("\"video-content\"") == TString::npos ? "novid" : "vid") << '\t' << (SerpPreviewHtml.find("serp-sitelinks") == TString::npos ? "nosl" : "sl");
                Cout << '\t' << (CtxPreview.size() ? "txt" : "notxt") << '\t' << (CtxImg ? "img" : "noimg") << '\t' << (CtxVid ? "vid" : "novid") << '\t' << (CtxMainPage || IsMain(Url) ? "main" : "nomain") << '\t' << (Ctx ? "ctx" : "noctx") << '\t' << (PreviewLink ? "link" : "nolink");
                Cout << Endl;
            }
            SomeDoc = false;
            SerpPreview.clear();
            SerpPreviewHtml.clear();
            PreviewProto.clear();
            SnipdatPreview.clear();
            Ctx = false;
            CtxPreview.clear();
            CtxImg = false;
            CtxVid = false;
            CtxMainPage = false;
            PreviewLink = false;
        }
        void Go(const TString& s) {
            TVector<TStringBuf> v;
            TCharDelimiter<const char> d('\t');
            TContainerConsumer< TVector<TStringBuf> > c(&v);
            SplitString(s.data(), s.data() + s.size(), d, c);
            if (v.empty()) {
                return;
            }
            if (v[0] == "input" && v.size() >= 2) {
                StartInput(v[1]);
                return;
            }
            if (v[0] == "preview_input" && v.size() >= 2) {
                OnPreviewInput(v[1]);
                return;
            }
            if (v[0] == "doc" && v.size() >= 3) {
                StartDoc(v[1], v[2]);
                return;
            }
            if (v[0] == "preview" && v.size() >= 3) {
                OnPreview(v[1], v[2]);
                return;
            }
            if (v[0] == "preview_basereply" && v.size() >= 3) {
                OnPreviewProto(v[1], v[2]);
                return;
            }
            if (v[0] == "metasearch_preview" && v.size() >= 3) {
                OnSnipdatPreview(v[1], v[2]);
                return;
            }
            if (v[0] == "metasearch_ctx" && v.size() >= 3) {
                OnCtx(v[1], v[2]);
                return;
            }
        }
    };
}

int main(int argc, char** argv) {
    --argc, ++argv;
    TString s;
    NSnippets::TProcessor p;
    while (Cin.ReadLine(s)) {
        p.Go(s);
    }
    return 0;
}
