#include "askctx.h"
#include "metahost.h"

#include <search/idl/meta.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>

#include <util/generic/string.h>
#include <util/stream/mem.h>
#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/split.h>

namespace NSnippets {

TStringBuf FindCtx(const NMetaProtocol::TArchiveInfo& ar) {
    for (size_t i = 0; i < ar.GtaRelatedAttributeSize(); ++i) {
        if ("_SnippetsCtx"sv == ar.GetGtaRelatedAttribute(i).GetKey()) {
            return ar.GetGtaRelatedAttribute(i).GetValue();
        }
    }
    return TStringBuf();
}

void AskReportSearchdata(TCgiParameters& cgi) {
    //java-report seems to not know searchdata option
    cgi.InsertUnescaped("json_dump", "1");
}
void AskReportFullDocids(TCgiParameters& cgi) {
    //could use paths from ExtractFullDocids below, but a.*.b.*.c works a bit weird, result is smth like like "a.*" : ...
    AskReportSearchdata(cgi);
}
void AskReportPreviewLinks(TCgiParameters& cgi) {
    //could use paths from ExtractFullDocids below, but a.*.b.*.c works a bit weird, result is smth like like "a.*" : ...
    AskReportSearchdata(cgi);
}
void AskReportExtSnippetLinks(TCgiParameters& cgi) {
    //could use paths from ExtractFullDocids below, but a.*.b.*.c works a bit weird, result is smth like like "a.*" : ...
    AskReportSearchdata(cgi);
}

static void ExtractByPath(const NJson::TJsonValue& root, TStringBuf path, TVector<TString>& res) {
    size_t t = path.find('.');
    TStringBuf key;
    if (t != TStringBuf::npos) {
        key = TStringBuf(path.data(), path.data() + t);
        path = TStringBuf(path.data() + t + 1, path.data() + path.size());
    } else {
        key = path;
        path = TStringBuf();
    }
    if (key == "") {
        if (root.IsString()) {
            res.push_back(root.GetString());
        }
        if (root.IsInteger()) {
            res.push_back(ToString(root.GetInteger()));
        }
        return;
    }
    if (key == "*") {
        if (root.IsArray()) {
            const NJson::TJsonValue::TArray& v = root.GetArray();
            for (size_t i = 0;  i < v.size(); ++i) {
                ExtractByPath(v[i], path, res);
            }
        }
        if (root.IsMap()) {
            const NJson::TJsonValue::TMapType& m = root.GetMap();
            for (NJson::TJsonValue::TMapType::const_iterator it = m.begin(); it != m.end(); ++it) {
                ExtractByPath(it->second, path, res);
            }
        }
        return;
    }
    NJson::TJsonValue e;
    if (root.IsMap() && root.GetValue(key, &e)) {
        ExtractByPath(e, path, res);
    }
}

static bool CleanExtraLink(TString& req) {
        TStringBuf scheme, hostport, path, cgi;
        if (ParseUrl(req, scheme, hostport, path, cgi)) {
            TCgiParameters cgip(cgi);
            ResetReportDumps(cgip);
            req = TString(scheme) + hostport + path + cgip.Print();
            return true;
        }
        return false;
}

void ExtractReportExtraSnippetBlocks(const NJson::TJsonValue& root, TVector<TExtraSnippetBlock>& v) {
    NJson::TJsonValue searchdata;
    if (!root.IsMap() || !root.GetValue("searchdata", &searchdata)) {
        NJson::TJsonValue tmpldata;
        if (!root.IsMap() || !root.GetValue("tmpl_data", &tmpldata) || !tmpldata.IsMap() || !tmpldata.GetValue("searchdata", &searchdata)) {
            return;
        }
    }
    NJson::TJsonValue docs;
    if (!searchdata.IsMap() || !searchdata.GetValue("docs", &docs)) {
        return;
    }
    if (!docs.IsArray()) {
        return;
    }
    for (size_t i = 0; i < docs.GetArray().size(); ++i) {
        const NJson::TJsonValue& d = docs.GetArray()[i];
        if (!d.IsMap()) {
            continue;
        }
        TExtraSnippetBlock res;
        NJson::TJsonValue docid, url, previewLink, extSnippetLink, serverDescr;
        if (d.GetValue("docid", &docid) && docid.IsString()) {
            res.DocId = docid.GetString();
        }
        if (d.GetValue("url", &url) && url.IsString()) {
            res.Url = url.GetString();
        }
        if (d.GetValue("preview_link", &previewLink) && previewLink.IsString()) {
            TString req = previewLink.GetString();
            if (CleanExtraLink(req)) {
                res.PreviewLink = req;
            }
        }
        if (d.GetValue("ext_snippet_link", &extSnippetLink) && extSnippetLink.IsString()) {
            TString req = extSnippetLink.GetString();
            if (CleanExtraLink(req)) {
                TVector<TString> extdw;
                ExtractByPath(d, "supplementary.generic.*.attrs.extdw", extdw);
                if (extdw.size() == 1 && extdw[0] && extdw[0] != "0") {
                    res.ExtSnippetLink = req;
                }
            }
        }
        if (d.GetValue("server_descr", &serverDescr) && serverDescr.IsString()) {
            res.ServerDescr = serverDescr.GetString();
        }
        v.push_back(res);
    }
}

void ExtractReportFullDocids(const NJson::TJsonValue& root, TVector<TString>& v) {
    TStringBuf keys[] = {
        "searchdata.docs.*.docid",
        "searchdata.images.*.docid",
        "searchdata.images.*.series.*.docid",
        "searchdata.clips.*.docid",
        "tmpl_data.yandexsearch.response.results.grouping.*.group.*.doc.*.id",
    };
    for (size_t i = 0; i < Y_ARRAY_SIZE(keys); ++i) {
        ExtractByPath(root, keys[i], v);
    }
    NJson::TJsonValue r;
    if (root.IsMap() && root.GetValue("tmpl_data", &r)) {
        ExtractReportFullDocids(r, v);
    }
}

static bool GetEvlog(const NJson::TJsonValue& root, TString& evlog) {
    if (!root.IsMap()) {
        return false;
    }
    NJson::TJsonValue e;
    if (!root.GetValue("eventlog", &e)) {
        return false;
    }
    if (!e.IsString()) {
        return false;
    }
    evlog = e.GetString();
    return true;
}

static void ParseEvlines(TStringBuf evlog, TVector<TStringBuf>& evlines) {
    TCharDelimiter<const char> d('\n');
    TContainerConsumer< TVector<TStringBuf> > c(&evlines);
    SplitString(evlog.data(), evlog.data() + evlog.size(), d, c);
}

static void ParseEvfields(TStringBuf evline, TVector<TStringBuf>& evfields) {
    TCharDelimiter<const char> d('\t');
    TContainerConsumer< TVector<TStringBuf> > c(&evfields);
    SplitString(evline.data(), evline.data() + evline.size(), d, c);
}

bool ExtractReportSubreqs(const NJson::TJsonValue& root, TVector<TSubreq>& v) {
    TString evlog;
    if (!GetEvlog(root, evlog)) {
        return false;
    }
    TVector<TStringBuf> evlines;
    ParseEvlines(evlog, evlines);
    for (const TStringBuf& evline : evlines) {
        TVector<TStringBuf> evfields;
        ParseEvfields(evline, evfields);
        if (evfields.size() < 3) {
            continue;
        }
        TStringBuf evtype = evfields[2];
        if (evtype == "SubSourceRequest") {
            if (evfields.size() < 8) {
                continue;
            }
            TStringBuf srcid = evfields[3];
            TStringBuf req = evfields[7];
            v.push_back(TSubreq(TString(srcid.data(), srcid.size()), TString(req.data(), req.size())));
        }
    }
    return true;
}

bool ExtractReportUpperReq(const NJson::TJsonValue& root, TUpperReq& res) {
    res = TUpperReq();
    TString evlog;
    if (!GetEvlog(root, evlog)) {
        return false;
    }
    TVector<TStringBuf> evlines;
    ParseEvlines(evlog, evlines);
    for (const TStringBuf& evline : evlines) {
        TVector<TStringBuf> evfields;
        ParseEvfields(evline, evfields);
        if (evfields.size() < 4) {
            continue;
        }
        TStringBuf evtype = evfields[2];
        if (evtype == "HostName") {
            res.Host = TString{evfields[3]};
        } else if (evtype == "ContextCreated" && evfields.size() >= 5) {
            res.ReqId = TString{evfields[4]};
        } else if (evtype == "PostBody") {
            res.PostBody = TString{evfields[3]};
        }
    }
    return res.Host && res.PostBody && res.ReqId;
}

TString FindInfoCtx(const TString& info) {
    TVector<TString> v;
    NJson::TJsonValue r;
    TMemoryInput mi(info.data(), info.size());
    if (!NJson::ReadJsonTree(&mi, &r)) {
        return TString();
    }
    ExtractByPath(r, "SnippetDump.rows.*.cells.*", v);
    if (v.size() != 1) {
        return TString();
    }
    return v[0];
}

}
