#include "output_html.h"
#include "html_hilite.h"

#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/urlmenu/dump/dump.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/scheme/scheme.h>

#include <util/charset/wide.h>
#include <util/stream/mem.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/subst.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {

    static const TString HTML_HEADER =
        "<html>\n"
        "<head>\n"
        "<meta charset=utf-8>\n"
        "<style>\n"
        "body { font-family:Arial,Helvetica,sans-serif; }\n"
        ".query { text-align:center; }\n"
        ".userreq { font-size:18px; line-height:30px; padding-right:16px; }\n"
        ".qmeta { font-size:11px; padding-left:16px; color:#888; }\n"
        ".lsnip { padding-right:16px; }\n"
        ".lsnip, .rsnip { vertical-align:top; }\n"
        ".snipd { font-size:13px; line-height:16px; color:#333; }\n"
        ".snipt { font-size:15px; line-height:20px; color:#000; }\n"
        ".snip { margin:16px auto 24px auto; border:1px solid transparent; }\n"
        ".snip:hover { border:1px dashed #aaa; }\n"
        ".title { font-size:18px; font-weight:400; line-height:24px; }\n"
        ".titlet { font-size:20px; font-weight:400; line-height:24px; margin-bottom:4px; }\n"
        ".tlink { text-decoration:none; color:#00c; }\n"
        ".tlink:hover, .tlink:visited:hover { color:#d00!important; }\n"
        ".tlink:visited { color:#551a8b; }\n"
        ".greenurl { color:#070; font-size:14px; line-height:17px; margin-bottom:2px; }\n"
        ".greenurlt { color:#070; font-size:15px; line-height:15px; padding-bottom:4px; }\n"
        ".greenurlt { text-overflow:ellipsis; overflow:hidden; white-space:nowrap; }\n"
        ".ulink, .ulink:link, .ulink:visited { text-decoration:none; color:#070; outline:0; }\n"
        ".ulink:hover, .ulink:link:hover, .ulink:visited:hover { color:#d00; }\n"
        ".text { word-wrap:break-word; }\n"
        ".list { margin:4px 0; }\n"
        ".lhead, .litem { overflow:hidden; max-width:100%; white-space:nowrap; text-overflow:ellipsis; margin-top:4px; }\n"
        ".litem { position:relative; padding-left:12px; }\n"
        ".litem:before { position:absolute; top:6px; left:3px; width:3px; height:3px; content:''; background-color:#7b7a79; }\n"
        ".dots, .meta { color:#888; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n";

    static const TString HTML_FOOTER =
        "</body>\n"
        "</html>\n";

    static const TString HTML_MIDDLE_DOTS =
        "&nbsp;<b class=dots>... </b>";

    static const TString BEST_ANSWER = "Лучший ответ: ";

    struct TMailRuSpecSnipData {
        TString Question;
        TString Answer;
        TString ExtendedAnswer;
        TString AnswerCount;

        bool operator==(const TMailRuSpecSnipData& another) const {
            return this->Question == another.Question &&
                    this->Answer == another.Answer &&
                    this->ExtendedAnswer == another.ExtendedAnswer &&
                    this->AnswerCount == another.AnswerCount;
        }

        bool operator!=(const TMailRuSpecSnipData& another) const {
            return !(*this == another);
        }
    };

    THtmlOutput::THtmlOutput(IOutputStream& out)
        : Out(out)
    {
        Out << HTML_HEADER;
    }

    static TString GetReportType(const TJob& job) {
        TString reportType;
        const auto& ctx = job.ContextData.GetProtobufData();
        if (ctx.HasReqParams() && ctx.GetReqParams().HasReport()) {
            reportType = ctx.GetReqParams().GetReport();
        }
        if (!reportType) {
            reportType = "?";
        }
        return reportType;
    }

    static bool IsTouchSearch(const TJob& job) {
        return GetReportType(job) == "www-touch";
    }

    static int GetSnipWidth(const TJob& job) {
        int snipWidth = 0;
        const auto& ctx = job.ContextData.GetProtobufData();
        if (ctx.HasReqParams() && ctx.GetReqParams().HasSnipWidth()) {
            snipWidth = ctx.GetReqParams().GetSnipWidth();
        }
        if (snipWidth == 0) {
            if (IsTouchSearch(job)) {
                snipWidth = 290;
            } else {
                snipWidth = 594;
            }
        }
        return snipWidth;
    }

    static TString GetSearchHost(const TJob& job) {
        bool isTurkey = false;
        const auto& ctx = job.ContextData.GetProtobufData();
        if (ctx.HasReqParams() && ctx.GetReqParams().HasIsTurkey()) {
            isTurkey = ctx.GetReqParams().GetIsTurkey();
        }
        return isTurkey ? "www.yandex.com.tr" : "yandex.ru";
    }

    static void PrintRequest(const TJob& job, IOutputStream& out) {
        out << "<div class=query>" << Endl;
        out << "<span class=userreq>";
        out << RehighlightAndHtmlEscape(UTF8ToWide(job.UserReq));
        out << "</span>" << Endl;
        out << "<span class=qmeta>" << job.Region << "</span>" << Endl;
        TString reportType = GetReportType(job);
        bool isTouchSearch = (reportType == "www-touch");
        bool isTabletSearch = (reportType == "www-tablet");
        out << "<span class=qmeta>" << reportType << "</span>" << Endl;
        out << "<a class=qmeta target=_blank href=\"https://";
        out << GetSearchHost(job) << "/search/";
        if (isTouchSearch) {
            out << "touch/";
        }
        if (isTabletSearch) {
            out << "pad/";
        }
        out << "?lr=" << job.Region;
        out << "&amp;text=" << CGIEscapeRet(job.UserReq + " << url:" + job.ArcUrl);
        if (isTouchSearch || isTabletSearch) {
            out << "&amp;noredirect=1";
        }
        out << "\">search</a>";
        out << "</div>" << Endl;
    }

    static void PrintTitle(const TJob& job, const TUtf16String& title, IOutputStream& out) {
        if (IsTouchSearch(job)) {
            out << "<div class=titlet>";
        } else {
            out << "<div class=title>";
        }
        out << "<a class=tlink target=_blank href=\"";
        out << AddSchemePrefix(job.ArcUrl) << "\">";
        out << RehighlightAndHtmlEscape(title);
        out << "</a></div>" << Endl;
    }

    static void PrintHilitedUrl(const TJob& job, const TPassageReply& res, IOutputStream& out) {
        if (IsTouchSearch(job)) {
            out << "<div class=greenurlt>";
        } else {
            out << "<div class=greenurl>";
        }
        if (res.GetUrlMenu() && !IsTouchSearch(job)) {
            TUrlMenuVector urlMenu;
            NUrlMenu::Deserialize(urlMenu, res.GetUrlMenu());
            bool firstPart = true;
            for (const auto& urlPart : urlMenu) {
                if (firstPart) {
                    firstPart = false;
                } else {
                    out << u" › ";
                }
                out << "<a class=ulink target=_blank href=\""
                    << AddSchemePrefix(WideToUTF8(urlPart.first)) << "\">"
                    << RehighlightAndHtmlEscape(urlPart.second) << "</a>";
            }
        } else {
            out << RehighlightAndHtmlEscape(UTF8ToWide(res.GetHilitedUrl()));
        }
        out << "</div>" << Endl;
    }

    static void PrintPassages(const TPassageReply& res, IOutputStream& out) {
        if (res.GetPassages() && res.GetPassagesType() == 0) {
            out << "<div class=text>";
            bool firstPassage = true;
            for (const TUtf16String& passage : res.GetPassages()) {
                if (firstPassage) {
                    firstPassage = false;
                } else {
                    out << HTML_MIDDLE_DOTS;
                }
                out << RehighlightAndHtmlEscape(passage);
            }
            out << "</div>" << Endl;
        }
    }

    static void PrintHeadline(const TPassageReply& res, IOutputStream& out) {
        if (res.GetHeadline()) {
            out << "<div class=text>";
            out << RehighlightAndHtmlEscape(res.GetHeadline());
            out << "</div>" << Endl;
        }
    }

    static void PrintByLink(const TJob& job, const TPassageReply& res, IOutputStream& out) {
        if (res.GetPassages() && res.GetPassagesType() == 1) {
            out << "<div class=text>";
            out << "<span class=meta>";
            if (job.UIL == "ru") {
                out << "Ссылки на страницу содержат";
            } else if (job.UIL == "tr") {
                out << "Şunu içeren sayfaya bağlantı";
            } else {
                out << "Links to the page contain";
            }
            out << ": </span>";
            Y_ASSERT(res.GetPassages().size() == 1);
            out << RehighlightAndHtmlEscape(res.GetPassages()[0]);
            out << "</div>" << Endl;
        }
    }

    static void PrintList(const TPassageReply& res, IOutputStream& out) {
        int firstPassage = 0;
        int lastPassage = res.GetPassages().ysize() - 1;
        TStringBuf specAttrs(res.GetSpecSnippetAttrs());
        while (specAttrs) {
            TStringBuf name = specAttrs.NextTok('\t');
            TStringBuf value = specAttrs.NextTok('\t');
            if (name == "listData") {
                // Value example: [{"if":1,"il":2,"lh":0,"lic":4}]
                TMemoryInput buf(value);
                NJson::TJsonValue data;
                if (NJson::ReadJsonTree(&buf, &data)) {
                    firstPassage = (int)data[0]["if"].GetInteger();
                    lastPassage = (int)data[0]["il"].GetInteger();
                }
                break;
            }
        }
        out << "<div class=list>" << Endl;
        int index = 0;
        for (const TUtf16String& passage : res.GetPassages()) {
            if (index < firstPassage) {
                out << "<div class=lhead>";
            } else if (index <= lastPassage) {
                out << "<div class=litem>";
            } else {
                out << "<div class=text>";
            }
            out << RehighlightAndHtmlEscape(passage);
            out << "</div>" << Endl;
            ++index;
        }
        out << "</div>" << Endl;
    }

    static TVector<TString> GetExtendedPassages(const TPassageReply& res) {
        NSc::TValue clickSnip = NSc::TValue::FromJson(res.GetClickLikeSnip());
        TVector<TString> result;
        if (clickSnip.IsDict() && clickSnip.Has("extended_snippet") &&
            clickSnip["extended_snippet"].IsDict() && clickSnip["extended_snippet"].Has("features") &&
            clickSnip["extended_snippet"]["features"].IsDict() && clickSnip["extended_snippet"]["features"].Has("passages") &&
            clickSnip["extended_snippet"]["features"]["passages"].IsArray())
        {
            for (const NSc::TValue& s : clickSnip["extended_snippet"]["features"]["passages"].GetArray()) {
                if (s.IsString()) {
                    result.push_back(TString{s.GetString()});
                }
            }
        }
        return result;
    }

    static void PrintExtendedSnip(const TPassageReply& res, IOutputStream& out) {
        TStringBuf specAttrs(res.GetSpecSnippetAttrs());
        if (specAttrs.Contains("extdw", 0)) {
            out << "<div class=tlink>Читать Ещё</div>";
        }
        TVector<TString> extensions = GetExtendedPassages(res);
        if (extensions.size() > 0) {
            out << "<div class=text>";
            bool firstPassage = true;
            for (const auto& passage : extensions) {
                if (firstPassage) {
                    firstPassage = false;
                } else {
                    out << HTML_MIDDLE_DOTS;
                }
                out << RehighlightAndHtmlEscape(UTF8ToWide(passage));
            }
            out << "</div>" << Endl;
        }
    }

    static TString GetMailRuSpecSnipField(const TPassageReply& res, const TString& fieldName) {
        NSc::TValue clickSnip = NSc::TValue::FromJson(res.GetClickLikeSnip());
        if (clickSnip.IsDict() && clickSnip.Has("schema_question") && clickSnip["schema_question"].IsDict() &&
            clickSnip["schema_question"].Has("features") && clickSnip["schema_question"]["features"].IsDict() &&
            clickSnip["schema_question"]["features"].Has(fieldName) && clickSnip["schema_question"]["features"][fieldName].IsString())
        {
            return TString{clickSnip["schema_question"]["features"][fieldName].GetString()};
        }
        return "";
    }

    static TMailRuSpecSnipData GetMailruSpecSnip(const TPassageReply& res) {
        TMailRuSpecSnipData snipData;
        snipData.Question = GetMailRuSpecSnipField(res, "question");
        snipData.Answer = GetMailRuSpecSnipField(res, "answer");
        snipData.ExtendedAnswer = GetMailRuSpecSnipField(res, "answer_extended");
        snipData.AnswerCount = GetMailRuSpecSnipField(res, "answer_count");
        return snipData;
    }

    static void PrintMailruSpecSnip(const TMailRuSpecSnipData& mailruSnip, IOutputStream& out) {
        TUtf16String question = RehighlightAndHtmlEscape(UTF8ToWide(mailruSnip.Question));
        TUtf16String answer = RehighlightAndHtmlEscape(UTF8ToWide(mailruSnip.Answer));

        out << "<div class=text>";
        out << question;
        out << "</div>" << Endl;
        out << "<div class=text style=\"margin-top: 4px;\">";
        out << "<b>" << BEST_ANSWER << "</b>";
        out << answer;
        out << "</div>" << Endl;

        TUtf16String extension = RehighlightAndHtmlEscape(UTF8ToWide(mailruSnip.ExtendedAnswer));
        if (extension.size()) {
            out << "<div class=tlink>Читать Ещё</div>";
            out << "<div class=text>";
            out << question;
            out << "</div>" << Endl;
            out << "<div class=text style=\"margin-top: 4px;\">";
            out << "<b>" << BEST_ANSWER << "</b>";
            out << extension;
            out << "</div>" << Endl;
        }
    }

    void THtmlOutput::DoPrint(const TJob& job, const TPassageReply& res, IOutputStream& out, bool diffInVisibleParts) {
        out << "<div class=\"snip ";
        out << (IsTouchSearch(job) ? "snipt" : "snipd");
        out << "\" style=\"width:" << GetSnipWidth(job) << "px\">" << Endl;
        PrintTitle(job, res.GetTitle(), out);
        PrintHilitedUrl(job, res, out);
        if (res.GetHeadlineSrc() == "list_snip") {
            PrintList(res, out);
        } else {
            PrintPassages(res, out);
            PrintHeadline(res, out);
            PrintByLink(job, res, out);
            if (!diffInVisibleParts) {
                PrintExtendedSnip(res, out);
            }
        }
        out << "</div>" << Endl;
    }

    void THtmlOutput::DoPrintMailRu(const TJob& job, const TPassageReply& res, IOutputStream& out) {
        out << "<div class=\"snip ";
        out << (IsTouchSearch(job) ? "snipt" : "snipd");
        out << "\" style=\"width:" << GetSnipWidth(job) << "px\">" << Endl;
        TMailRuSpecSnipData mailruSnip = GetMailruSpecSnip(res);
        TString title = "Ответы mail.ru — " + mailruSnip.AnswerCount + " ответов";
        PrintTitle(job, UTF8ToWide(title), out);
        PrintHilitedUrl(job, res, out);
        PrintMailruSpecSnip(mailruSnip, out);
        out << "</div>" << Endl;
    }

    void THtmlOutput::Process(const TJob& job) {
        PrintRequest(job, Out);
        DoPrint(job, job.Reply, Out, true);
    }

    void THtmlOutput::Complete() {
        Out << HTML_FOOTER;
    }


    TDiffHtmlOutput::TDiffHtmlOutput(IOutputStream& out)
        : Out(out)
    {
        Out << HTML_HEADER;
    }

    void TDiffHtmlOutput::Process(const TJob& job) {
        if (!Differs(job.Reply, job.ReplyExp)) {
            return;
        }
        PrintRequest(job, Out);

        bool diffInVisibleParts = DiffersInVisibleParts(job.Reply, job.ReplyExp);
        bool diffInMailru = GetMailruSpecSnip(job.Reply) != GetMailruSpecSnip(job.ReplyExp);

        Out << "<table align=center cellspacing=0 cellpadding=0>" << Endl;
        if (diffInMailru) {
            Out << "<tr><td class=lsnip>" << Endl;
            THtmlOutput::DoPrintMailRu(job, job.Reply, Out);
            Out << "</td><td class=rsnip>" << Endl;
            THtmlOutput::DoPrintMailRu(job, job.ReplyExp, Out);
            Out << "</td></tr>" << Endl;
        } else {
            Out << "<tr><td class=lsnip>" << Endl;
            THtmlOutput::DoPrint(job, job.Reply, Out, diffInVisibleParts);
            Out << "</td><td class=rsnip>" << Endl;
            THtmlOutput::DoPrint(job, job.ReplyExp, Out, diffInVisibleParts);
            Out << "</td></tr>";
        }
        Out << "</table>" << Endl;
    }

    void TDiffHtmlOutput::Complete() {
        Out << HTML_FOOTER;
    }

} //namespace NSnippets
