#pragma once

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NMetaProtocol {
    class TArchiveInfo;
    class TReport;
}

namespace NJson {
    class TJsonValue;
}

namespace NSnippets {
    inline void FixMultipleSnip(TCgiParameters& cgi) {
        if (cgi.NumOfValues("snip") == 0) {
            return;
        }
        TString snip;
        for (size_t i = 0; i < cgi.NumOfValues("snip"); ++i) {
            TString s = cgi.Get("snip", i);
            if (s.size()) {
                if (snip.size()) {
                    snip += ";";
                }
                snip += s;
            }
        }
        cgi.EraseAll("snip");
        cgi.InsertUnescaped("snip", snip);
    }
    inline void AskSnippetsCtx(TCgiParameters& cgi) {
        if (!cgi.Has("gta", "_SnippetsCtx")) {
            cgi.InsertUnescaped("gta", "_SnippetsCtx");
        }
        TString snip = cgi.Get("snip");
        snip = snip + ";wantctx=da";
        cgi.ReplaceUnescaped("snip", snip);
    }
    inline void AskPreview(TCgiParameters& cgi, const TString& docid) {
        cgi.InsertUnescaped("snip", "fstann");
        cgi.InsertUnescaped("rearr", "Pindocs_on;pindocs=" + docid);
    }
    inline void AskInfoSnippetsCtx(TCgiParameters& cgi, const TString& docid) {
        cgi.EraseAll("info");
        cgi.InsertUnescaped("info", "snipdump:docid:" + docid + ";json");
    }

    inline void ResetReportDumps(TCgiParameters& cgi) {
        cgi.EraseAll("dump");
        cgi.EraseAll("json_dump");
    }

    inline void AskReportEventlog(TCgiParameters& cgi) {
        cgi.InsertUnescaped("json_dump", "eventlog");
    }

    inline void AskMetaEventlog(TCgiParameters& cgi) {
        cgi.InsertUnescaped("dump", "eventlog");
    }

    void AskReportSearchdata(TCgiParameters& cgi);
    void AskReportFullDocids(TCgiParameters& cgi);
    struct TExtraSnippetBlock {
        TString Url;
        TString DocId;
        TString PreviewLink;
        TString ExtSnippetLink;
        TString ServerDescr;
    };
    void AskReportPreviewLinks(TCgiParameters& cgi);
    void AskReportExtSnippetLinks(TCgiParameters& cgi);
    void ExtractReportFullDocids(const NJson::TJsonValue& root, TVector<TString>& v);
    void ExtractReportExtraSnippetBlocks(const NJson::TJsonValue& root, TVector<TExtraSnippetBlock>& v);
    typedef std::pair<TString, TString> TSubreq;
    bool ExtractReportSubreqs(const NJson::TJsonValue& root, TVector<TSubreq>& v);
    struct TUpperReq {
        TString Host;
        ui16 Port = 9080;
        TString Path = "/yandsearch";
        TString PostBody;
        TString ReqId;
    };
    bool ExtractReportUpperReq(const NJson::TJsonValue& root, TUpperReq& res);
    TStringBuf FindCtx(const NMetaProtocol::TArchiveInfo& ar);
    TString FindInfoCtx(const TString& info);
}
