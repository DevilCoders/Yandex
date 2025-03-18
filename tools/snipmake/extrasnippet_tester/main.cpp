#include <tools/snipmake/snipdat/askctx.h>
#include <tools/snipmake/snipdat/metahost.h>

#include <tools/snipmake/common/nohtml.h>

#include <search/idl/meta.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>

#include <kernel/search_daemon_iface/dp.h>

#include <util/stream/input.h>
#include <util/stream/mem.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {
    bool MakePreviewBeta(THost& host, TStringBuf& scheme, TStringBuf& mhostport, TStringBuf& mpath, TCgiParameters& cgi) {
        if (mhostport.EndsWith(".com.tr")) {
            mhostport = "www.hamster.yandex.com.tr";
        } else if (mhostport.EndsWith(".ua")) {
            mhostport = "hamster.yandex.ua";
        } else if (mhostport.EndsWith(".kz")) {
            mhostport = "hamster.yandex.kz";
        } else if (mhostport.EndsWith(".by")) {
            mhostport = "hamster.yandex.by";
        } else if (mhostport.EndsWith(".ru")) {
            mhostport = "hamster.yandex.ru";
        } else {
            return false;
        }
        if (mpath != "/yandsearch?" && mpath != "/search/?" && mpath != "/touchsearch?") {
            return false;
        }
        if (!ParseHostport(scheme, mhostport, host)) {
            return false;
        }
        cgi.EraseAll("exp_flags");
        cgi.InsertUnescaped("exp_flags", "enable_serp3");
        cgi.InsertUnescaped("exp_flags", "content_preview");
        return true;
    }
    void Go(const TString& s) {
        Cout << "input" << '\t' << s << Endl;
        try {
            THost mhost;
            TStringBuf mscheme;
            TStringBuf mhostport;
            TStringBuf mpath;
            TStringBuf mcgi;
            if (!ParseUrl(s, mhostport, mscheme, mpath, mcgi)) {
                Cout << "error" << '\t' << "can't parse input url" << Endl;
                return;
            }
            TCgiParameters cgi(mcgi);
            if (!MakePreviewBeta(mhost, mscheme, mhostport, mpath, cgi)) {
                Cout << "error" << '\t' << "can't make preview beta url" << Endl;
                return;
            }
            Cout << "preview_input" << '\t' << mscheme << mhostport << mpath << cgi.Print() << Endl;

            ResetReportDumps(cgi);
            AskReportEventlog(cgi);
            AskReportFullDocids(cgi);
            AskReportPreviewLinks(cgi);
            TString ans = mhost.Fetch(TString(mpath) + cgi.Print(), 10);
            Cout << "report" << '\t' << (TString(mscheme) + mhostport + mpath + cgi.Print()) << '\t' << Base64Encode(ans) << Endl;
            NJson::TJsonValue v;
            TMemoryInput mi(ans.data(), ans.size());
            if (!NJson::ReadJsonTree(&mi, &v)) {
                Cout << "error" << '\t' << "can't parse report json" << Endl;
            }
            TVector<TSubreq> subreqs;
            ExtractReportSubreqs(v, subreqs);
            TVector<TExtraSnippetBlock> previewBlocks;
            ExtractReportExtraSnippetBlocks(v, previewBlocks);
            for (size_t i = 0; i < previewBlocks.size(); ++i) {
                Cout << "doc" << '\t' << previewBlocks[i].DocId << '\t' << previewBlocks[i].Url << Endl;
                for (size_t t = 0; t < 2; ++t) {
                    const bool preview = t == 1;
                    const TString prefix = preview ? "preview" : "extend";
                    if (!preview && previewBlocks[i].ServerDescr == "FAKE") {
                        continue;
                    }
                    THost phost;
                    TStringBuf pscheme;
                    TStringBuf phostport;
                    TStringBuf ppath;
                    TStringBuf pcgi;
                    const TString link = preview ? previewBlocks[i].PreviewLink : previewBlocks[i].ExtSnippetLink;
                    if (!link.size()) {
                        Cout << "no_" << prefix << "_link" << '\t' << previewBlocks[i].DocId << '\t' << previewBlocks[i].Url << Endl;
                    } else if (!ParseUrl(link, phost, pscheme, phostport, ppath, pcgi)) {
                        Cout << "bad_" << prefix << "_link" << '\t' << previewBlocks[i].DocId << '\t' << link << Endl;
                    } else {
                        TString pv = phost.Fetch(TString(ppath) + pcgi, 10);
                        Cout << prefix << '\t' << (TString(pscheme) + phostport + ppath + pcgi) << '\t' << Base64Encode(pv) << Endl;
                        TVector<TSubreq> pvSubreqs;
                        {
                            TCgiParameters pp(pcgi);
                            AskReportEventlog(pp);
                            TString pv2 = phost.Fetch(TString(ppath) + pp.Print(), 10);
                            Cout << prefix << "_eventlog" << '\t' << (TString(pscheme) + phostport + ppath + pp.Print()) << '\t' << Base64Encode(pv2) << Endl;
                            NJson::TJsonValue v;
                            TMemoryInput mi(pv2.data(), pv2.size());
                            if (NJson::ReadJsonTree(&mi, &v)) {
                                ExtractReportSubreqs(v, pvSubreqs);
                            }
                        }
                        for (size_t j = 0; j < pvSubreqs.size(); ++j) {
                            THost bhost;
                            TStringBuf bscheme;
                            TStringBuf bhostport;
                            TStringBuf bpath;
                            TStringBuf bcgi;
                            if (!ParseUrl(pvSubreqs[j].second, bhost, bscheme, bhostport, bpath, bcgi)) {
                                Cout << prefix << "_bad_event_link" << '\t' << pvSubreqs[j].second << Endl;
                                continue;
                            }
                            TString bpv = bhost.Fetch(TString(bpath) + bcgi, 10);
                            Cout << prefix << "_basereply" << '\t' << (TString(bscheme) + bhostport + ppath + bcgi) << '\t' << Base64Encode(bpv) << Endl;
                        }
                    }
                    for (size_t j = 0; j < subreqs.size(); ++j) {
                        const TString srcDash = subreqs[j].first + "-";
                        const TString& req = subreqs[j].second;
                        if (!previewBlocks[i].DocId.StartsWith(srcDash)) {
                            continue;
                        }
                        THost host;
                        TStringBuf scheme;
                        TStringBuf hostport;
                        TStringBuf path;
                        TStringBuf cgi;
                        if (!ParseUrl(req, host, scheme, hostport, path, cgi)) {
                            Cout << prefix << "_bad_subreq" << '\t' << req << Endl;
                            continue;
                        }
                        if (preview) {
                            TCgiParameters cgip(cgi);
                            AskPreview(cgip, previewBlocks[i].DocId);
                            TString bpv2 = host.Fetch(TString(path) + cgip.Print(), 10);
                            Cout << "metasearch_preview" << '\t' << (TString(scheme) + hostport + path + cgip.Print()) << '\t' << Base64Encode(bpv2) << Endl;
                        }
                        {
                            TCgiParameters ccgi(cgi);
                            FixMultipleSnip(ccgi);
                            AskInfoSnippetsCtx(ccgi, previewBlocks[i].DocId.substr(srcDash.size()));
                            TString nans = host.Fetch(TString(path) + ccgi.Print());
                            Cout << prefix << "_metasearch_infoctx" << '\t' << (TString(scheme) + hostport + path + ccgi.Print()) << '\t' << Base64Encode(nans) << Endl;
                            TString ctx = FindInfoCtx(nans);
                            if (ctx.size()) {
                                Cout << prefix << "_metasearch_ctx" << '\t' << (TString(scheme) + hostport + path + ccgi.Print()) << '\t' << ctx << Endl;
                            }
                        }
                    }
                }
            }
        } catch (yexception& e) {
            Cout << "fatal_error" << Endl;
            Cout << e.what() << Endl;
        }
    }
    void FetcherMain() {
        TString s;
        while (Cin.ReadLine(s)) {
            Go(s);
        }
    }
    struct TParserMain {
        struct TDoc {
            TString DocId;
            TString Url;
            TString ExtLink;
            TString Extended;
        };
        TDoc Doc;
        int Bad = 0;
        int Lost = 0;
        int Ok = 0;
        void Look() {
            if (Doc.Extended == "{\"error\":\"not_found\"}") {
                ++Lost;
            } else if (Doc.Extended == "{\"error\":\"bad_document\"}") {
                ++Bad;
            } else {
                ++Ok;
            }
            Cout << Doc.Url << '\t' << Doc.Extended << Endl;
        }
        void Step(const TStringBuf& s) {
            TVector<TStringBuf> v;
            TCharDelimiter<const char> d('\t');
            TContainerConsumer<TVector<TStringBuf>> c(&v);
            SplitString(s.data(), s.data() + s.size(), d, c);
            const TStringBuf key = v[0];
            if (key == "doc") {
                Doc = TDoc();
                Doc.DocId = v[1];
                Doc.Url = v[2];
            } else if (key == "extend") {
                Doc.ExtLink = v[1];
                Doc.Extended = Base64Decode(v[2]);
                Look();
            }
        }
        void Flush() {
        }
        void Finish () {
            int sum = Bad + Lost + Ok;
            Cout << "bad: " << Bad * 100. / sum << " lost: " << Lost * 100. / sum << " ok: " << Ok * 100. / sum << Endl;
        }
        void Run() {
            TString s;
            while (Cin.ReadLine(s)) {
                if (s.StartsWith("input\t")) {
                    Flush();
                }
                Step(s);
            }
            Finish();
        }
    };
    void ParserMain() {
        TParserMain().Run();
    }
}

int main(int argc, char** argv) {
    --argc, ++argv;
    if (!argc) {
        NSnippets::FetcherMain();
    } else if (argc && argv[0] == "parse"sv) {
        NSnippets::ParserMain();
    } else {
        Cerr << "unknown mode: " << argv[0] << Endl;
    }
    return 0;
}
