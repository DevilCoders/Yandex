#include "segserverscommon.h"
#include <util/string/cast.h>
#include <util/string/escape.h>
#include <util/string/type.h>

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <signal.h>

using namespace NDater;
using namespace NSegm;

namespace NSegutils {

struct TRenderSettings {
    bool ShowImages;
    bool ShowLinks;
    ui32 ShowDates;
    bool ShowFeatures;
    int  Dater2Mode;

    TRenderSettings()
        : ShowImages()
        , ShowLinks()
        , ShowDates(0)
        , ShowFeatures()
        , Dater2Mode(ND2::DM_ALL_DATES_MAIN_RANGES)
    {}
};

TString RenderDate(const TDaterDate& d) {
    return d ? d.ToString("%d/%m/%Y from %F, %P") : "";
}

void InsertDates(const TDatePositions& dp, TEventStorage& st) {
    TString fmt;
    TUtf16String btext;
    TUtf16String etext;

    for (TDatePositions::const_iterator it = dp.begin(); it != dp.end(); ++it) {
        fmt = Sprintf("<span class=dateregion title=\"%s-%s, %s\">",
                      it->Begin.ToString().data(), it->End.ToString().data(), RenderDate(*it).data());
        btext = UTF8ToWide(fmt);
        etext = u"<!--/dateregion--></span>";
        st.InsertSpan(*it, "DATE", btext, etext);
    }
}

void InsertSegments(const TSegmentSpans& sp, TEventStorage& st) {
    TString fmt;
    TUtf16String btext;
    TUtf16String etext;

    for (TSegmentSpans::const_iterator it = sp.begin(); it != sp.end(); ++it) {
        TString main = (it->InMainContentNews ? "-main" : "");
        TString mainlabel = (it->InMainContentNews ? "<br>main content" : "");
        TString segname = GetSegmentName((ESegmentType)it->Type);

        fmt = Sprintf("\n<table class='segmentspan-%s segmentspan%s' title=\"%s%s:%s-%s\">"
                        "\n<tr><td class=segmentinfo><a href=#%u name=%u>%u.</a> %s<br>\n"
                        "score:&nbsp;%.3f%s<br>\n"
                        "<span class=featuresswitch>features</span>\n"
                        "<table class=segmentfeatures>\n"
                        "<tr><td>words<td>%u\n"
                        "<tr><td>link words<td>%u\n"
                        "<tr><td>links<td>%u\n"
                        "<tr><td>local links<td>%u\n"
                        "<tr><td>owners<td>%u\n"
                        "<tr><td>inputs<td>%u\n"
                        "<tr><td>blocks<td>%u\n"
                        "<tr><td>symbols<td>%u\n"
                        "<tr><td>alphas<td>%u\n"
                        "<tr><td>spaces<td>%u\n"
                        "<tr><td>commas<td>%u\n"
                        "<tr><td>ads<td>css:%u htext:%u\n"
                        "<tr><td>comments<td>css:%u htext:%u\n"
                        "<tr><td>footer<td>css:%u text:%u\n"
                        "<tr><td>poll<td>css:%u\n"
                        "<tr><td>menu<td>css:%u\n"
                        "<tr><td>header<td>is:%u has:%u\n"
                        "<tr><td colspan=2>front block"
                        "<tr><td colspan=2>inc:%u dep:%u b:%u p:%u i:%u s:%u\n"
                        "<tr><td colspan=2>back block"
                        "<tr><td colspan=2>inc:%u dep:%u b:%u p:%u i:%u s:%u\n"
                        "</table>\n"
                        "\n<td class=segmenttext>",
                        segname.data(), main.data(), segname.data(), main.data(),
                        it->Begin.ToString().data(),
                        it->End.ToString().data(),
                        ui32(it - sp.begin()), ui32(it - sp.begin()), ui32(it - sp.begin()), segname.data(),
                        it->Weight, mainlabel.data(),
                        it->Words, it->LinkWords,
                        it->Links, it->LocalLinks,
                        it->Domains, it->Inputs, it->Blocks,
                        it->SymbolsInText, it->AlphasInText, it->SpacesInText, it->CommasInText,
                        it->AdsCSS, it->AdsHeader,
                        it->CommentsCSS, it->CommentsHeader,
                        it->FooterCSS, it->FooterText,
                        it->PollCSS, it->MenuCSS,
                        it->IsHeader, it->HasHeader,
                        it->FirstBlock.Included, it->FirstBlock.Depth,
                        it->FirstBlock.Blocks, it->FirstBlock.Paragraphs, it->FirstBlock.Items, it->FirstBlock.HasSuperItem,
                        it->LastBlock.Included, it->LastBlock.Depth,
                        it->LastBlock.Blocks, it->LastBlock.Paragraphs, it->LastBlock.Items, it->LastBlock.HasSuperItem
                        );
        btext = UTF8ToWide(fmt);
        etext = u"<!--/segmentspan--></table>";
        st.InsertSpan(*it, "SEGMENT", btext, etext);
    }
}

template <typename TSpansVec>
void InsertSpans(TEventStorage& st, const TSpansVec& v, const TString& attr,
                 const TUtf16String& beg, const TUtf16String& end, const TString& lbl) {
    for (typename TSpansVec::const_iterator it = v.begin(); it != v.end(); ++it)
        st.InsertSpan(*it, attr,
                      beg + UTF8ToWide(Sprintf(" title=\"%s:%s-%s\" >%s",
                                               attr.data(), it->Begin.ToString().data(), it->End.ToString().data(), lbl.data())), end);
}

void MarkSpans(TDaterContext& ctx, const TRenderSettings& s) {
    if (s.ShowDates) {
        InsertDates(ctx.GetTitleDates(), ctx.GetEvents(true));
        InsertDates(ctx.GetBodyDates(), ctx.GetEvents(false));
    }

    TEventStorage& st = ctx.GetEvents(false);

    {
        TUtf16String b = u"<div class=headerspan ";
        TUtf16String e = u"<!--/headerspan--></div>";
        InsertSpans(st, ctx.GetHeaderSpans(), "HEADER", b, e, "");
    }

    {
        TUtf16String b = u"<div class=strictheaderspan ";
        TUtf16String e = u"<!--/strictheaderspan--></div>";
        InsertSpans(st, ctx.GetStrictHeaderSpans(), "STRICT_HEADER", b, e, "");
    }

    {
        TUtf16String b = u"<div class=mainheaderspan ";
        TUtf16String e = u"<!--/mainheaderspan--></div>";
        InsertSpans(st, ctx.GetMainHeaderSpans(), "MAINHEADER", b, e, "");
    }

    InsertSegments(ctx.GetSegmentSpans(), st);

    {
        TUtf16String b = u"<div class=articlezone ";
        TUtf16String e = u"<!--/articlezone--></div>";
        InsertSpans(st, ctx.GetArticleSpans(), "ARTICLE", b, e, "<span class=zoneinfo>Article</span>");
    }
}


struct TCtx {
    TRenderSettings Settings;
    IOutputStream& Out;

    bool EmptyParagraph;
    bool Title;

    TCtx(const TRenderSettings& s, IOutputStream& out, bool title)
        : Settings(s)
        , Out(out)
        , EmptyParagraph(true)
        , Title(title)
    {}

    void OnEvent(const TSegEvent* ev) {
        TUtf16String text = ToWtring(ev->Text);
        TString rawattr = ToString(ev->Attr);
        EscapeHtmlChars<false>(text);

        switch (ev->Type) {
        default:
            break;
        case SET_BLOCK_OPEN:
            if ("pre" == ev->Attr)
                Out << "<pre>";
            break;
        case SET_BLOCK_CLOSE:
            if ("pre" == ev->Attr)
                Out << "</pre>";
            break;
        case SET_PARABRK:
            Out << text;

            if (!EmptyParagraph && !Title) {
                Out << "<p>";
                EmptyParagraph = true;
            }
            break;
        case SET_SPACE:
        case SET_SENTBRK:
            if (!text)
                Out << ' ';
            else
                Out << text;
            EmptyParagraph &= IsSpace(ev->Text.data(), ev->Text.size());
            break;
        case SET_TOKEN:
            Out << "<span class=token title=\"" << ev->Pos.ToString() << "\">" << text << "<!--/token--></span>";
            EmptyParagraph = false;
            break;
        case SET_IMAGE_EXT:
            if (Settings.ShowImages) {
                EmptyParagraph = false;
                Out << "<img src=\"" << RenderLinkUrl(rawattr, true, true) << "\" alt=\"" << text << "\" >";
            }
            break;
        case SET_IMAGE_INT:
            if (Settings.ShowImages) {
                EmptyParagraph = false;
                Out << "<img src=\"" << RenderLinkUrl(rawattr, true, true) << "\" alt=\"" << text << "\" >";
            }
            break;
        case SET_LINKEXT_OPEN:
            if (Settings.ShowLinks) {
                EmptyParagraph = false;
                Out << "<a class=linkext target=_blank href=\"" << RenderLinkUrl(rawattr, true, true) << "\">";
            }
            break;
        case SET_LINKINT_OPEN:
            if (Settings.ShowLinks) {
                EmptyParagraph = false;
                Out << "<a class=linkint target=_blank href=\"" << RenderLinkUrl(rawattr, true, true) << "\">";
            }
            break;
        case SET_LINK_CLOSE:
            if (Settings.ShowLinks) {
                EmptyParagraph = false;
                Out << "</a>";
            }
            break;
        case SET_SPAN_BEGIN:
        case SET_SPAN_END:
            Out << ev->Text;
            EmptyParagraph = ev->Attr != "DATE";
        }
    }
};

void RenderEvents(const TRenderSettings& sets, bool title, const TEventStorage& events, IOutputStream& out) {
    TCtx ctx(sets, out, title);
    for (TEventStorage::const_iterator it = events.begin(); it != events.end(); ++it)
        ctx.OnEvent(it);
}

class TSegRequest: public TRequestBase {
    TDaterContext Context;
    TRenderSettings Settings;

public:
    TSegRequest(const TString& configDir)
        : TRequestBase(configDir)
        , Context(ParserContext)
    {
    }

    TString RenderBackScripts() const override {
        TString s = TRequestBase::RenderBackScripts();
        s += TString("$('.segmentfull')") + (GetShowFeatures() ? ".show()\n" : ".hide()\n");
        s += "$('.segmentbrief').click(function(){\n"
                        "   $(this).parent().filter('.segmentfull').toggle();\n"
                        "})\n";
        s += "$('.dateregion .token, .linkint .token, .linkext .token').removeAttr('title');\n";
        s += "$('.featuresswitch').click(function(){"
                        "   $(this).siblings().filter('.segmentfeatures').toggle();\n"
                        "});\n";
        return s;
    }

    TString RenderStyle() const override {
        return TRequestBase::RenderStyle().append(
                        ".dateregion, .dateregion a {\n"
                        "   font-family: courier;\n"
                        "   color: blue;\n"
                        "   white-space: nowrap;\n"
                        "}\n"
                        ".linkext {\n"
                        "   color: red;\n"
                        "   text-decoration: none;\n"
                        "   border-bottom: 1px dashed red;\n"
                        "}\n"
                        ".linkint {\n"
                        "   color: green;\n"
                        "   text-decoration: none;\n"
                        "   border-bottom: 1px dashed green;\n"
                        "}\n"
                        ".segmentinfo, .zoneinfo, .segmentfeatures td {\n"
                        "   vertical-align: top;\n"
                        "   font-size: small;\n "
                        "   padding: 2px 3px;\n"
                        "   background-color: #eee;\n "
                        "   border: 1px solid #ccc;\n"
                        "   width: 100px;\n"
                        "}\n"
                        ".segmentspan-main .segmentinfo {\n"
                        "   font-weight: bold;\n"
                        "   background-color: #f7f7f7\n"
                        "}\n"
                        ".segmentfeatures td {\n"
                        "   font-size: 10px;\n "
                        "   white-space:nowrap;\n"
                        "}\n"
                        ".linkext img, .linkint img {\n"
                        "   padding: 1px;\n "
                        "   border: 1px dashed;\n "
                        "}\n"
                        ".headerspan {\n "
                        "   font-weight: bold;\n "
                        "}\n"
                        ".strictheaderspan {\n "
                        "   font-weight: bold;\n "
                        "   color: red;\n "
                        "}\n"
                        ".mainheaderspan {\n "
                        "   font-weight: bold;\n "
                        "   font-style:italic;\n "
                        "}\n"
                        "#segments {\n"
                        "   margin-top:10px;\n"
                        "}\n"
                        ".segmentspan, .segmentspan-main {\n"
                        "   margin-top:3px;\n"
                        "   margin-bottom:3px;\n"
                        "}\n"
                        ".featuresswitch {\n"
                        "   color: green;\n"
                        "   text-decoration:none;\n"
                        "   border-bottom: 1px dashed green;\n"
                        "}\n"
                        ".articlezone {"
                        "   margin-top:6px;\n"
                        "   margin-bottom:6px;\n "
                        "   margin-left:-4px;\n"
                        "   padding:2px 3px;\n"
                        "   border:1px solid #999;\n"
                        "}\n"
                        ".zoneinfo {\n"
                        "   margin-left:-4px;\n"
                        "}\n"

                        ).append(GetShowFeatures() ? ".segmentfeatures {}" : ".segmentfeatures {display:none}");
    }

    TDaterDate GetNowDate() const {
        TDaterDate d = ScanDateSimple(RD.CgiParam.Get("nowdate"));
        return d ? d : TDaterDate::Now();
    }

    ui32 GetShowDates() const {
        const TString& sd = Strip(RD.CgiParam.Get("showdates"));
        return !sd ? 0 : FromString<ui32>(sd);
    }

    bool GetShowLinks() const {
        return !!Strip(RD.CgiParam.Get("showlinks"));
    }

    bool GetShowImages() const {
        return !!Strip(RD.CgiParam.Get("showimages"));
    }

    bool GetShowFeatures() const {
        return !!Strip(RD.CgiParam.Get("showfeatures"));
    }

    int GetDater2Mode() const {
        TString c = Strip(RD.CgiParam.Get("dater2mode"));
        return IsNumber(c) ? FromString<int> (c) : 0;
    }

    ui32 GetCoarse() const {
        TString c = Strip(RD.CgiParam.Get("coarse"));
        return IsNumber(c) ? FromString<ui32> (c) : 0;
    }

    void InitPage() override {
        Settings.ShowDates = GetShowDates();
        Settings.ShowFeatures = GetShowFeatures();
        Settings.ShowImages = GetShowImages();
        Settings.ShowLinks = GetShowLinks();
        Settings.Dater2Mode = GetDater2Mode();

        TRequestBase::InitPage();
        RawDoc.Time.SetDate(GetNowDate());

        try {
            Context.SetUseDater2(2 == Settings.ShowDates, (ND2::EDaterMode)Settings.Dater2Mode);
            Context.SetDoc(RawDoc);
            Context.NumerateDoc();

            MarkSpans(Context, Settings);
        } catch (const yexception& e) {
            Errors.push_back(Sprintf("Experienced problems with processing '%s': %s",
                    RawDoc.Url.c_str(), e.what()).c_str());
            return;
        }
    }

    TString RenderSubForm() const override {
        return TString("<tr><td colspan=2 nowrap class=field2>")
                        .append(RenderRadio("Hide dates", "showdates", 0, 0 == Settings.ShowDates))
                        .append(RenderRadio("Show dater", "showdates", 1, 1 == Settings.ShowDates))
                        .append(RenderRadio("Show dater2", "showdates", 2, 2 == Settings.ShowDates))
                        .append(" | ")
                        .append(RenderSelect("Dater2 mode", "dater2mode",
                                             "Main dates", ND2::DM_MAIN_DATES,
                                             "Main dates and ranges", ND2::DM_MAIN_DATES_MAIN_RANGES,
                                             "All dates", ND2::DM_ALL_DATES,
                                             "All dates and ranges", ND2::DM_ALL_DATES_ALL_RANGES,
                                             "All dates and main ranges", ND2::DM_ALL_DATES_MAIN_RANGES,
                                             Settings.Dater2Mode))
                        .append("<tr><td colspan=2 nowrap class=field2>")
                        .append(RenderCheckbox("Show links", "showlinks", Settings.ShowLinks))
                        .append(" | ")
                        .append(RenderCheckbox("Show images", "showimages", Settings.ShowImages))
                        .append(" | ")
                        .append(RenderCheckbox("Show segment features", "showfeatures", Settings.ShowFeatures))
                        .append(" | ")
                        .append(RenderInput("Current date", "nowdate", GetNowDate().ToString("%d/%m/%Y")))
                        .append("</tr>");
    }


    TString RenderStats() const override {
        TString stats = TRequestBase::RenderStats();

        if (!HasLocation())
            return stats;

        stats.append("\n<tr><td class=key>Title<td>") .append(Render(true));
        if (Context.GetBestDate())
            stats.append("\n<tr><td class=key>Best date<td>").append(RenderDate(Context.GetBestDate()));
        if (!Context.GetDaterStats().Empty()) {
            stats.append("\n<tr><td class=key>Dater stats DM<td>").append(Context.GetDaterStats().ToStringDaysMonths());
            stats.append("\n<tr><td class=key>Dater stats MY<td>").append(Context.GetDaterStats().ToStringMonthsYears());
            stats.append("\n<tr><td class=key>Dater stats Y<td>").append(Context.GetDaterStats().ToStringYears());
        }
        if (!Context.GetTopDates().empty())
            stats.append("\n<tr><td class=key>Top dates<td>").append(SaveDatesList(Context.GetTopDates()));
        if (!Context.GetDaterStatsFiltered().Empty()) {
            stats.append("\n<tr><td class=key>Dater stats DM filtered<td>").append(Context.GetDaterStatsFiltered().ToStringDaysMonths());
            stats.append("\n<tr><td class=key>Dater stats MY filtered<td>").append(Context.GetDaterStatsFiltered().ToStringMonthsYears());
            stats.append("\n<tr><td class=key>Dater stats Y filtered<td>").append(Context.GetDaterStatsFiltered().ToStringYears());
        }
        stats.append("\n<tr><td class=key>Header spans<td>").append(ToString<size_t> (Context.GetHeaderSpans().size()));
        stats.append("\n<tr><td class=key>Strict header spans<td>").append(ToString<size_t> (Context.GetStrictHeaderSpans().size()));
        stats.append("\n<tr><td class=key>Main header spans<td>").append(ToString<size_t> (Context.GetMainHeaderSpans().size()));
        stats.append("\n<tr><td class=key>Segment spans<td>").append(ToString<size_t> (Context.GetSegmentSpans().size()));
        stats.append("\n<tr><td class=key>Article spans<td>").append(ToString<size_t> (Context.GetArticleSpans().size()));
        stats.append("\n<tr><td class=key>Main content spans<td>").append(ToString<size_t> (Context.GetMainContentSpans().size()));
        stats.append("\n<tr><td class=key>Stored segmarkup ver.<td>").append(ToString((ui32)NSegm::CurrentStoredSegSpanVersion));

        stats.append("\n<tr><td class=key>Legend<td>");

        if (GetShowLinks())
            stats.append("<span class=linkint>internal link</span>, <span class=linkext>external link</span>, ");

        if (GetShowDates())
            stats.append("<span class=dateregion>date</span>, ");

        stats.append("<span class=headerspan>header</span>, <span class=mainheaderspan>main header</span>, <span class=strictheaderspan>strict header</span>");

        return stats;
    }

    TString Render(bool title) const {
        TString res;
        TStringOutput sout(res);
        RenderEvents(Settings, title, Context.GetEvents(title), sout);
        return res;
    }

    TString RenderContent() const override {
        if (!IsReady())
            return "";

        return "\n<div id=segments>\n" + Render(false) + "\n</div>\n";
    }

};

}

int main(int argc, const char** argv) {
    using namespace NSegutils;
    Cerr << "Welcome to the segviewer" << Endl;

    StartServer<TSegRequest> (argc, argv, 6060, 8, ".");
}
