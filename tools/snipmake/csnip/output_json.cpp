#include "output_json.h"

#include "html_hilite.h"
#include "lines_count.h"

#include <tools/snipmake/steam/snippet_json_iterator/snippet_json_iterator.h>

#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/urlmenu/dump/dump.h>

#include <kernel/tarc/docdescr/docdescr.h>

#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>
#include <library/cpp/json/json_reader.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSnippets {

    TJsonOutput::TJsonOutput(IOutputStream& out)
      : Out(out)
    {
    }

    static TString StrEsc(const TString& s) {
        TString res;
        NEscJ::EscapeJ<true, true>(TStringBuf(s), res);
        return res;
    }

    static TString StrEsc(const TUtf16String& s) {
        TString res;
        NEscJ::EscapeJ<true, true>(TStringBuf(WideToUTF8(s)), res);
        return res;
    }

    static TString LnToSp(const TString& s) {
        TString res;
        for (size_t i = 0; i < s.size(); ++i) {
            res += s[i] == '\n' ? ' ' :  s[i];
        }
        return res;
    }

    static void Print(const TPassageReply& res, IOutputStream& out, const TStringBuf& /*postfix*/) {
        out << "\"" << NJsonFields::TITLE << "\" : " << StrEsc(RehighlightAndHtmlEscape(res.GetTitle())) << ", ";
        out << "\"" << NJsonFields::HEADLINE_SRC << "\" : " << StrEsc(res.GetHeadlineSrc()) << ", ";
        out << "\"" << NJsonFields::HEADLINE << "\" : " << StrEsc(RehighlightAndHtmlEscape(res.GetHeadline())) << ", ";
        out << "\"" << NJsonFields::BY_LINK << "\" : " << (res.GetPassagesType() == 1 ? "true" : "false") << ", ";
        TString lines = GetLinesCount(res.GetSnippetsExplanation());
        if (!lines.empty()) {
            out << "\"" << NJsonFields::LINES << "\" : " << StrEsc(lines) << ", ";
        }
        out << "\"" << NJsonFields::PASSAGES << "\" : [ ";
        for (size_t i = 0; i < res.GetPassages().size(); ++i) {
            if (i) {
                out << ", ";
            }
            out << StrEsc(RehighlightAndHtmlEscape(res.GetPassages()[i]));
        }
        out << " ]";
        if (res.GetSpecSnippetAttrs().size()) {
            out << ", \"" << NJsonFields::SPEC_ATTRS << "\": " << StrEsc(res.GetSpecSnippetAttrs());
        }
        if (res.GetPreviewJson().size()) {
            out << ", \"" << NJsonFields::PREVIEW_JSON << "\": " <<
                StrEsc(LnToSp(res.GetPreviewJson()));
        }
        if (res.GetSnippetsExplanation().size()) {
            out << ", \"" << NJsonFields::EXPL << "\" : ";
            if (NJson::ValidateJson(res.GetSnippetsExplanation())) {
                out << res.GetSnippetsExplanation();
            } else {
                out << StrEsc(res.GetSnippetsExplanation());
            }
        }
        if (res.GetClickLikeSnip().size()) {
            out << ", \"" << NJsonFields::CLICKLIKE_JSON << "\": " <<
                StrEsc(LnToSp(res.GetClickLikeSnip()));
        }
        if (res.GetLinkAttrs()) {
            out << ", \"" << NJsonFields::LINK_ATTRS << "\": " << StrEsc(res.GetLinkAttrs());
        }
    }

    void TJsonOutput::DoPrint(const TJob& job, const TPassageReply& res, IOutputStream& out) {
        out << "{";
        out << "\"" << NJsonFields::DOC << "\" : " << StrEsc(job.ContextData.GetId()) << ", ";
        out << "\"" << NJsonFields::REQ << "\" : " << StrEsc(job.Req) << ", ";
        out << "\"" << NJsonFields::USERREQ << "\" : " << StrEsc(job.UserReq) << ", ";
        out << "\"" << NJsonFields::REGION << "\" : " << StrEsc(ToString(job.Region)) << ", ";
        out << "\"" << NJsonFields::URL << "\" : " << StrEsc(job.ArcUrl) << ", ";
        out << "\"" << NJsonFields::HILITEDURL << "\" : " <<
            StrEsc(RehighlightAndHtmlEscape(UTF8ToWide(res.GetHilitedUrl()))) << ", ";

        if (res.GetUrlMenu()) {
            TUrlMenuVector v;
            NUrlMenu::Deserialize(v, res.GetUrlMenu());
            out << "\"" << NJsonFields::URLMENU << "\" : [ ";
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) {
                    out << ", ";
                }
                out << "[ " << StrEsc(WideToUTF8(v[i].first)) << ", " <<
                    StrEsc(RehighlightAndHtmlEscape(v[i].second)) << " ]";
            }
            out << " ], ";
        }

        TDocInfos::const_iterator arcIt = job.DocInfos->find("market");
        if (arcIt != job.DocInfos->end()) {
            out << "\"" << NJsonFields::MARKET << "\" : " << StrEsc(arcIt->second) << ", ";
        }

        int snipWidth = 0;
        const auto& ctx = job.ContextData.GetProtobufData();
        if (ctx.HasReqParams() && ctx.GetReqParams().HasSnipWidth()) {
            snipWidth = ctx.GetReqParams().GetSnipWidth();
        }
        if (snipWidth > 0) {
            out << "\"" << NJsonFields::SNIP_WIDTH << "\" : " << StrEsc(ToString(snipWidth)) << ", ";
        }
        if (ctx.HasLog() && ctx.GetLog().HasHeadlineSrc()) {
            out << "\"original_headline_src\":" << StrEsc(ctx.GetLog().GetHeadlineSrc()) << ", ";
        }
        if (ctx.HasLog() && ctx.GetLog().HasSnippetGta()) {
            out << "\"ms_snippet_gta\":" << StrEsc(ctx.GetLog().GetSnippetGta()) << ", ";
        }

        Print(res, out, TStringBuf());
        out << "}";
    }

    void TJsonOutput::Process(const TJob& job) {
        DoPrint(job, job.Reply, Out);
        Out << Endl;
        Out.Flush();
        Cerr.Flush();
    }


    TDiffJsonOutput::TDiffJsonOutput(IOutputStream& out)
        : Out(out)
    {
    }

    void TDiffJsonOutput::Process(const TJob& job) {
        if (!Differs(job.Reply, job.ReplyExp)) {
            return;
        }
        Out << "[";
        TJsonOutput::DoPrint(job, job.Reply, Out);
        Out << ",";
        TJsonOutput::DoPrint(job, job.ReplyExp, Out);
        Out << "]";
        Out << Endl;
        Out.Flush();
        Cerr.Flush();
    }

} //namespace NSnippets
