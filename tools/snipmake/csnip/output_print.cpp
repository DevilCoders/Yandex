#include "output_print.h"

#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/idl/secondary_snippets.pb.h>

#include <util/generic/string.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/cast.h>
#include <util/string/printf.h>

namespace NSnippets {

    TPrintOutput::TPrintOutput(IOutputStream& out, bool noDoc, bool noSuffix, bool wantAttrs, bool printAltSnippets)
        : Out(out)
        , NoDoc(noDoc)
        , NoSuffix(noSuffix)
        , WantPassageAttrs(wantAttrs)
        , PrintAltSnippets(printAltSnippets)
    {
    }

    TString TPrintOutput::FieldName(const char* name, const TStringBuf& postfix)
    {
        if (!!postfix)
            return Sprintf("%s/%s: ", name, postfix.data());
        else
            return Sprintf("%s: ", name);
    }

    void TPrintOutput::Print(const TPassageReply& res, IOutputStream& out, const TStringBuf& postfix, bool wantAttrs, bool wantAltSnippets) {
        PrintNoRecurse(res, out, postfix, wantAttrs);
        if (!wantAltSnippets) {
            return;
        }
        TString alts = res.GetAltSnippets();
        if (!alts) {
            return;
        }
        NSnippets::NProto::TSecondarySnippets ss;
        if (ss.ParseFromString(Base64Decode(alts))) {
            for (int i = 0; i < ss.snippets_size(); ++i) {
                TString postfixWithIndex = TString{postfix} + "/" + ToString(i + 1);
                TPassageReply alt(ss.GetSnippets(i));
                PrintNoRecurse(alt, out, postfixWithIndex, wantAttrs);
            }
        }
    }

    void TPrintOutput::PrintNoRecurse(const TPassageReply& res, IOutputStream& out, const TStringBuf& postfix, bool wantAttrs) {
        out << "hilitedurl: " << res.GetHilitedUrl() << Endl;
        if (res.GetUrlMenu().size()) {
            out << "urlmenu: " << res.GetUrlMenu() << Endl;
        }
        out << FieldName("title", postfix) << res.GetTitle() << Endl;
        if (!!res.GetHeadlineSrc()) {
            out << FieldName("headline_src", postfix) << res.GetHeadlineSrc() << Endl;
        }
        if (!!res.GetHeadline()) {
            out << FieldName("headline", postfix) << res.GetHeadline() << Endl;
        }
        if (res.GetPassagesType() == 1) {
            out << FieldName("by_link", postfix) << "true" << Endl;
        }
        if (!!res.GetSpecSnippetAttrs()) {
            out << FieldName("spec_attrs", postfix) << res.GetSpecSnippetAttrs() << Endl;
        }
        for (size_t i = 0; i < res.GetMarkersCount(); ++i) {
            out << FieldName("markers", postfix) << res.GetMarker(i) << Endl;
        }
        for (size_t i = 0; i < res.GetPassages().size(); ++i) {
            out << FieldName("passage", postfix) << res.GetPassages()[i] << Endl;
        }
        if (wantAttrs) {
            for (size_t i = 0; i < res.GetPassagesAttrs().size(); ++i) {
                out << FieldName("passageattr", postfix) << res.GetPassagesAttrs()[i] << Endl;
            }
        }
        if (!!res.GetImagesJson()) {
            out << FieldName("images_json", postfix) << res.GetImagesJson() << Endl;
        }
        if (!!res.GetImagesDups()) {
            out << FieldName("images_dups", postfix) << res.GetImagesDups() << Endl;
        }
        if (!!res.GetPreviewJson()) {
            out << FieldName("preview_json", postfix) << res.GetPreviewJson() << Endl;
        }
        if (!!res.GetClickLikeSnip()) {
            out << FieldName("clicklike_json", postfix) << res.GetClickLikeSnip() << Endl;
        }
        if (!!res.GetSnippetsExplanation()) {
            out << FieldName("expl", postfix) << res.GetSnippetsExplanation() << Endl;
        }
    }

    void TPrintOutput::Process(const TJob& job) {
        if (!NoDoc) {
            Out << "document: " << job.ContextData.GetId() << Endl;
        }
        Out << "req: " << job.Req << Endl;
        Out << "userreq: " << job.UserReq << Endl;
        Out << "reqid: " << GetRequestId(job.ContextData) << Endl;
        Out << "region: " << job.Region << Endl;

        if (job.MoreHlReq.size()) {
            Out << "morehlreq: " << job.MoreHlReq << Endl;
        }
        if (job.RegionReq.size()) {
            Out << "regionreq: " << job.RegionReq << Endl;
        }
        Out << "url: " << job.ArcUrl << Endl;
        Print(job.Reply, Out, NoSuffix ? "" : job.BaseExp, WantPassageAttrs, PrintAltSnippets);
        if (job.Exp.size()) {
            Print(job.ReplyExp, Out, NoSuffix ? "" : job.Exp, WantPassageAttrs, PrintAltSnippets);
        }
        Out << Endl;
        Out.Flush();
        Cerr.Flush();
    }

} //namespace NSnippets
