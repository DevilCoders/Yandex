#include <library/cpp/string_utils/quote/quote.h>

#include "segserverscommon.h"

namespace NSegutils {

TString Itoa(int i) {
    return ToString<int> (i);
}

TString TRequestBase::RenderFrontScripts() const {
    return Sprintf("function SelectMode() {\n"
    "   $('#%s, #%s').hide().contents().filter(':input').attr('disabled', 'disabled');\n"
    "   $('#' + $(this).val()).show().contents().filter(':input').removeAttr('disabled');\n"
    "}\n", UrlForm, FileForm) +
    "\n" +
    Sprintf("function GetRandomUrl() {\n"
    "   $('input[name=%s]').val($.ajax({\n"
    "           url: window.location.protocol + '//' + window.location.host + window.location.pathname + '?randurl=true',\n"
    "           dataType: \"text\",\n"
    "           async:false\n"
    "       }\n"
    "   ).responseText);"
    "}\n", WebUrl);
    ;
}

TString TRequestBase::RenderBackScripts() const {
    return Sprintf("$('select[name=%s]').change(SelectMode).change();\n"
                   "$('#random_url').click(GetRandomUrl);\n", FileMode);
}


TString RenderFileModeSelect(const TRequestBase& req) {
    TString s("Data source\n");
    s.append("\n<select name=").append(FileMode).append(">\n");

        s.append("\n<option value=").append(UrlForm).append(UrlForm == req.GetFileMode() ? " selected>\n" : ">\n")
                .append("From web\n</option>\n");
    if (!req.IsDisabled(FileForm))
        s.append("\n<option value=").append(FileForm).append(FileForm == req.GetFileMode() ? " selected>\n" : ">\n")
                .append("\nFrom file\n</option>\n");
    s.append("\n</select>\n");
    return s;
}

TString RenderWebUrlMode(const TRequestBase& req) {
    return Sprintf("\n<tbody id=%s>\n", UrlForm)
                    .append("\n<tr><td class=label>Url<td nowrap class=field>\n")
                    .append("\n<button type=button id=random_url>Random Url</button>\n")
                    .append(RenderInput(WebUrl, req.GetWebUrl()))
                    .append("\n</tbody>\n");
}

TString RenderFileMode(const TRequestBase& req) {
    return Sprintf("\n<tbody id=%s>\n", FileForm)
                    .append("\n<tr><td class=label>File location<td class=field>\n")
                    .append(RenderInput(FileLocation, req.GetFileLocation()))
                    .append("\n<tr><td class=label>Document url<td class=field>\n")
                    .append(RenderInput(FileUrl, req.GetFileUrl()))
                    .append("\n</tbody>\n");
}

static const char START_MARKER[] = "<a class=\"b-serp-item__title-link\" href=\"";
static const char END_MARKER[] = "\"";

struct TYandexSerpLinkIter {
    TStringBuf Html;
    size_t BegPos;
    size_t EndPos;
    ui32 Counter;

    TYandexSerpLinkIter(TStringBuf html, size_t pos = 0)
        : Html(html)
        , BegPos(pos)
        , EndPos(pos)
        , Counter()
    {}

    bool Next() {
        if (TStringBuf::npos == BegPos || TStringBuf::npos == EndPos)
            return false;

        BegPos = Html.find(START_MARKER, EndPos);

        if (TStringBuf::npos == BegPos)
            return false;

        BegPos += TStringBuf(START_MARKER).size();
        EndPos = Html.find(END_MARKER, BegPos);

        if (TStringBuf::npos == EndPos)
            return false;

        if (EndPos < BegPos)
            return false;

        return true;
    }

    TStringBuf GetCurrent() const {
        return EndPos - BegPos < 4096 ? Html.SubStr(BegPos, EndPos - BegPos) : TStringBuf();
    }
};

TString TRequestBase::RenderRandomUrl() const {
    static const TString marker = "<a class=\"b-serp-item__title-link\" href=\"";
    static const TString endmarker = "\"";
    static const TString yandex =
                    Sprintf("http://yandex.ru/yandsearch?random=%u&randomtext=1&lr=213&numdoc=1&i-m-a-hacker=1&p=%u",
                            RandomNumber<ui32>(), RandomNumber<ui32> () % 30);

    THtmlDocument res = Fetch(yandex);

    if (!res.Errors.empty())
        return Sprintf("Experienced error%s: %s", res.Errors.size() == 1 ? "" : "s", JoinStrings(
                res.Errors, "; ").c_str());

    TYandexSerpLinkIter iter(res.Html);

    while (iter.Next()) {
        if (!iter.GetCurrent().StartsWith("http://yabs."))
            return TString{iter.GetCurrent()};
    }

    return Sprintf("could not find start marker (%s) in %s", START_MARKER, yandex.data());
}

TString TRequestBase::RenderSvnVersionInfo() const {
    TString res("\n<a id=svn_info_button onclick=\"$('#svn_info_div').toggle('fast');\" href='#'>Segviewer svn info</a>\n");
    res.append("<div id='svn_info_div'>");
    res.append(GetProgramSvnVersion());
    res.append("</div>");
    return res;
}

TString TRequestBase::RenderForm() const {
    TString form("\n<form id=main_form >\n<table>\n");

    form.append("\n<tr><td colspan=2 nowrap class=field2>\n").append(RenderFileModeSelect(*this))
         .append(" | ")
         .append(RenderCheckbox("Disregard noindex", IgnoreNoindex, GetIgnoreNoindex()))
         .append(" | ")
         .append(RenderInput("Charset", Charset, GetCharset()));

    form.append(RenderWebUrlMode(*this));

    if (!IsDisabled(FileForm))
        form.append(RenderFileMode(*this));

    return form.append(RenderSubForm()).append("\n<tr><td><input type=submit value=Submit>").append(
            "\n</table>\n</form>\n");
}

TString TRequestBase::RenderTitle() const {
    return EncodeHtmlPcdata(RawDoc.Url.data());
}

TString TRequestBase::RenderErrors() const {
    TString res;
    for (TVector<TString>::const_iterator it = Errors.begin(); it != Errors.end(); ++it) {
        res += EncodeHtmlPcdata(it->c_str());
        res += "<br>\n";
    }
    return res;
}

TString TRequestBase::RenderStats() const {
    if (Location.empty()) {
        return "";
    }

    TString s;
    s.append("\n<tr><td class=key>URL<td><a target=_blank href='")
                    .append(RenderLinkUrl(RawDoc.Url, true, true)).append("' >")
                    .append(RenderLinkUrl(RawDoc.Url, true, false)).append("</a>");

    if (GetFileMode() != UrlForm) {
        s.append("\n<tr><td class=key>Реальное местоположение\n<td>")
        .append("<a target=_blank href=\"").append(EncodeHtmlPcdata((RD.GetCurPage()))).append("&amp;")
        .append(ShowHtml).append("=true\">").append(EncodeHtmlPcdata(Location.data())).append("</a>");
    }

    return s;
}

TString TRequestBase::RenderStyle() const {
    return TString("* { font-size:14px;}"
            "#wrapper {margin:30px;}\n"
            "#svn_info_button {color:#999; text-decoration: none; border-bottom: 1px dotted #999;}\n"
            "#svn_info_div {width:300px; display:none}\n"
            "table {empty-cells:show;}\n"
            "td {padding-right:5px;}\n"
            ".field input {width:800px;}\n"
            "#charset {width:50px;}\n"
            ".label, .key, .field2 {text-align:right;}\n"
            ".key {white-space:nowrap;width:50px;}\n"
            "#stats table {width:100%;}\n"
            "#stats td {font-size:small; padding:2px 3px; background-color: #eee; border: 1px solid #ccc;}\n"
            "#content, #stats {margin-left:30px;}\n"
            "#errors {color:red;}\n");
}

TString TRequestBase::RenderPage() const {
    TString page;

    page.append("\n<html>\n<head>\n");
    page.append("\n<style type=text/css>\n").append(RenderStyle()).append("\n</style>\n");
    page.append("\n<script type=text/javascript src=\"").append("//yandex.st/jquery/1.5.1/jquery.min.js").append("\" ></script>\n");
    page.append("\n<script type=text/javascript >\n").append(RenderFrontScripts()).append("\n</script>\n");
    page.append("\n<title>\n").append(RenderTitle()).append("\n</title>\n");
    page.append("\n</head>\n<body>\n").append("\n<div id=wrapper>\n");
    page.append(RenderSvnVersionInfo());
    page.append("\n<div id=form>\n").append(RenderForm()).append("\n</div>\n");
    page.append("\n<div id=stats>\n<table>\n").append(RenderStats()).append("\n</table>\n</div>\n");

    if (HasErrors()) {
        page.append("\n<div id=errors>\n").append(RenderErrors()).append("\n</div>\n");
    }

    page.append("\n<div id=content>\n").append(RenderContent()).append("\n</div>\n");
    page.append("\n</div>\n");
    page.append("\n<script type=text/javascript >\n").append(RenderBackScripts()).append("\n</script>\n");
    page.append("\n</body>\n</html>\n");

    return page;
}

void TRequestBase::InitDoc() {
    if (!!RawDoc.Url || HasLocation())
        ythrow yexception (); //it shouldn't be initialized by this point yet

    try {
        Location = "";

        if (GetFileMode() == UrlForm) {
            Location = GetWebUrl();
            RawDoc = Fetch(GetWebUrl());
        } else if (GetFileMode() == FileForm) {
            Location = GetFileLocation();

            if (Location.StartsWith("http://") || Location.StartsWith("https://")) {
                RawDoc = Fetch(Location);
            } else if (GetValidFileName(Location)) {
                RawDoc = THtmlFileReader(!!GetFileUrl() ? THtmlFileReader::MDM_None : THtmlFileReader::MDM_FirstLine)
                                .Read(Location);
            }

            RawDoc.Url = AddSchemePrefix(!!GetFileUrl() ? GetFileUrl() : RawDoc.Url);

        } else
            ythrow yexception ();

    } catch (const yexception& e) {
        Errors.push_back(e.what());
    }

    RawDoc.ForcedCharset = CharsetByName(GetCharset().data());
    if (CODES_UNKNOWN == RawDoc.ForcedCharset && !GetCharset().empty()) {
        Errors.push_back(Sprintf("Do not know charset '%s'", GetCharset().c_str()));
    }

    for (TVector<TString>::const_iterator it = RawDoc.Errors.begin(); it != RawDoc.Errors.end(); ++it) {
        Errors.push_back(
                Sprintf("Experienced problem with '%s': %s", Location.c_str(), it->c_str()));
    }

    RawDoc.Errors.clear();
}
}
