#include <tools/snipmake/snipdat/askctx.h>
#include <tools/snipmake/snipdat/metahost.h>
#include <tools/snipmake/snipdat/usersessions.h>
#include <tools/snipmake/snipdat/xmlsearchin.h>
#include <tools/snipmake/reqrestr/reqrestr.h>

#include <search/idl/meta.pb.h>
#include <search/session/compression/report.h>

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/urlid/doc_handle.h>
#include <kernel/snippets/idl/snippets.pb.h>

#include <library/cpp/lcs/lcs_via_lis.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>

#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/guid.h>
#include <util/generic/hash_set.h>
#include <util/stream/output.h>
#include <util/network/socket.h>
#include <library/cpp/http/io/stream.h>
#include <util/stream/str.h>
#include <util/stream/mem.h>
#include <util/stream/zlib.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/base64/base64.h>

using namespace NSearchQuery;

namespace NSnippets
{
    void OutMerge(const TString& prefix, const TVector<TStringBuf>& urls) {
        size_t i = 0;
        while (i < urls.size()) {
            size_t j = i + 1;
            size_t delta = urls[i].size();
            while (j < urls.size() && prefix.size() + delta + urls[j].size() + sizeof(" ()") - 1 + (j - i) * (sizeof("url:\"\"") - 1) + (j - i - 1) * (sizeof(" | ") - 1) + 20 < 400) {
                delta += urls[j].size();
                ++j;
            }
            Cout << prefix << " (";
            for (size_t k = i; k != j; ++k) {
                if (k > i) {
                    Cout << " | ";
                }
                Cout << "url:\"" << urls[k] << "\"";
            }
            Cout << ")" << Endl;
            i = j;
        }
    }
    void OutUrlsRestr(IOutputStream& out, const TVector<TStringBuf>& urls) {
        out << "(";
        for (size_t i = 0; i < urls.size(); ++i) {
            if (i) {
                out << " | ";
            }
            out << "url:\"" << urls[i] << "\"";
        }
        out << ")";
    }
    TString GetUrlsRestr(const TVector<TStringBuf>& urls) {
        TString res;
        TStringOutput o(res);
        OutUrlsRestr(o, urls);
        return res;
    }
    void OutSplit(const TString& req, const TVector<TStringBuf>& urls, const TString& region, const TString& domRegion) {
        Cout << req << "\t";
        OutUrlsRestr(Cout, urls);
        Cout << "\t" << region << "\t" << domRegion << Endl;
    }
    void OutSplitSplit(const TString& req, const TVector<TStringBuf>& urls, const TString& region, const TString& domRegion) {
        for (size_t i = 0; i < urls.size(); ++i) {
            Cout << req << "\t";
            Cout << urls[i];
            Cout << "\t" << region << "\t" << domRegion << Endl;
        }
    }

    struct TSessionsMain {
        TString Service;
        TString UI;
        TSessionsMain(TStringBuf s) {
            if (s == "imgsessions") {
                Service = "images.yandex";
                UI = "images.yandex";
            } else if (s == "videosessions") {
                Service = "video.yandex";
                UI = "video.yandex";
            } else {
                Service = "www.yandex";
                UI = "www.yandex";
            }
        }
        int Run(int argc, char** argv) {
            if (argc != 1) {
                Cerr << "expected 1 arg: [merge / split / request-url / splitsplit]";
                return -1;
            }
            bool justreq = "request-url"sv == argv[0];
            bool merge = "merge"sv == argv[0];
            bool split = "split"sv == argv[0];
            bool splitsplit = "splitsplit"sv == argv[0];
            TSessionEntry e;
            TVector<TStringBuf> urls;
            while (ParseNextMREntry(e, &Cin)) {
                if (e.Type != "REQUEST" || !e.Query.size() || e.Res.empty() || e.Service != Service || e.UI != UI || e.TestBuckets.size()) {
                    continue;
                }
                urls.resize(e.Res.size());
                for (size_t i = 0; i < urls.size(); ++i) {
                    urls[i] = e.Res[i].Url;
                }
                if (merge)
                    OutMerge(e.Query, urls);
                else if (split)
                    OutSplit(e.Query, urls, e.UserRegion, e.DomRegion);
                else if (splitsplit)
                    OutSplitSplit(e.Query, urls, e.UserRegion, e.DomRegion);
                else if (justreq)
                    Cout << e.FullRequest << Endl;
                else
                    Cout << e.Query << Endl;
            }
            return 0;
        }
    };
    struct THamsterFestMain {
        THamsterFestMain() {
        }
        int Run(int argc, char** argv) {
            bool beHamster = true;
            bool redirs = false;
            bool info = false;
            bool wantComplete = true;
            for (int i = 0; i < argc; ++i) {
                if ("--nohamster"sv == argv[i]) {
                    beHamster = false;
                    continue;
                }
                if ("--redirbeta"sv == argv[i]) {
                    redirs = true;
                    continue;
                }
                if ("--viainfo"sv == argv[i]) {
                    info = true;
                    continue;
                }
                if ("--incomplete"sv == argv[i]) {
                    wantComplete = false;
                    continue;
                }
                Cerr << "unknown toggle: " << argv[i] << Endl;
                return 1;
            }
            TString line;
            TString ans;
            while (Cin.ReadLine(line)) {
                try {
                    THost mhost;
                    TStringBuf mpath;
                    TStringBuf mcgi;
                    if (!ParseUrl(line, mhost, mpath, mcgi)) {
                        Cerr << "fail: can't parse url " << line << Endl;
                        continue;
                    }
                    if (beHamster && !BecomeHamster(mhost)) {
                        Cerr << "fail: can't become hamster for url " << line << Endl;
                        continue;
                    }
                    TCgiParameters cgi(mcgi);
                    ResetReportDumps(cgi);
                    AskReportEventlog(cgi);
                    AskReportFullDocids(cgi);
                    ans = mhost.Fetch(TString(mpath) + cgi.Print(), redirs ? 10 : 0);
                    NJson::TJsonValue v;
                    TMemoryInput mi(ans.data(), ans.size());
                    if (!NJson::ReadJsonTree(&mi, &v)) {
                        Cerr << "fail: can't get json_dump for " << line << Endl;
                        Cerr << "response is like: " << ans.substr(0, 100) << Endl;
                        continue;
                    }
                    TVector<TString> docids;
                    ExtractReportFullDocids(v, docids);
                    THashSet<TString> want_srcs;
                    THashSet<TString> want_docs;
                    THashSet<TString> dedup;
                    TVector<TString> vres;
                    THashMap<TString, TString> mres;
                    for (size_t i = 0; i < docids.size(); ++i) {
                        size_t t = docids[i].find('-');
                        if (t == TString::npos) {
                            Cerr << "note: no srcid in docid " << docids[i] << Endl;
                            continue;
                        }
                        want_docs.insert(docids[i]);
                        want_srcs.insert(TString(docids[i].data(), docids[i].data() + t));
                    }
                    TVector<TSubreq> subreqs;
                    if (!ExtractReportSubreqs(v, subreqs)) {
                        Cerr << "can't get eventlog subreqs: " << line << Endl;
                        continue;
                    }
                    typedef std::pair<TString, TString> TDocCtx;
                    TVector<TDocCtx> answers;
                    for (size_t i = 0; i < subreqs.size(); ++i) {
                        TString srcid = subreqs[i].first;
                        TString req = subreqs[i].second;
                        if (want_srcs.find(TString(srcid.data(), srcid.data() + srcid.size())) == want_srcs.end()) {
                            continue;
                        }
                        THost nhost;
                        TStringBuf npath;
                        TStringBuf ncgi;
                        if (!ParseUrl(req, nhost, npath, ncgi)) {
                            continue;
                        }
                        TCgiParameters ccgi(ncgi);
                        FixMultipleSnip(ccgi);
                        if (info) {
                            for (size_t i = 0; i < docids.size(); ++i) {
                                if (!docids[i].StartsWith(srcid + "-")) {
                                    continue;
                                }
                                TCgiParameters icgi = ccgi;
                                AskInfoSnippetsCtx(icgi, docids[i].substr(srcid.size() + 1));
                                TString nans = nhost.Fetch(TString(npath) + icgi.Print());
                                TString ctx = FindInfoCtx(nans);
                                if (!ctx.size()) {
                                    continue;
                                }
                                answers.push_back(TDocCtx(docids[i], ctx));
                            }
                        } else {
                            AskSnippetsCtx(ccgi);
                            TString nans = nhost.Fetch(TString(npath) + ccgi.Print());

                            NMetaProtocol::TReport report;
                            if (!report.ParseFromString(nans)) {
                                continue;
                            }
                            NMetaProtocol::Decompress(report); // decompresses only if needed
                            for (size_t i = 0; i < report.GroupingSize(); ++i) {
                                const NMetaProtocol::TGrouping& grouping = report.GetGrouping(i);
                                for (size_t j = 0; j < grouping.GroupSize(); ++j) {
                                    const NMetaProtocol::TGroup& group = grouping.GetGroup(j);
                                    for (size_t k = 0; k < group.DocumentSize(); ++k) {
                                        const NMetaProtocol::TDocument& doc = group.GetDocument(k);
                                        TString fullId = srcid + TDocRoute::Separator;
                                        if (doc.HasDocId()) {
                                            fullId += doc.GetDocId();
                                        } else if (doc.HasDocHash()) {
                                            TDocHandle dh(doc.GetDocHash(), doc.GetRoute());
                                            fullId += dh.ToString();
                                        } else {
                                            continue;
                                        }
                                        if (!doc.HasArchiveInfo()) {
                                            continue;
                                        }
                                        const NMetaProtocol::TArchiveInfo& ar = doc.GetArchiveInfo();
                                        TString ctx = TString(FindCtx(ar));
                                        if (!ctx.size()) {
                                            continue;
                                        }
                                        answers.push_back(TDocCtx(fullId, ctx));
                                    }
                                }
                            }
                        }
                    }
                    for (size_t i = 0; i < answers.size(); ++i) {
                        TString fullId = answers[i].first;
                        TString ctx = answers[i].second;
                        if (want_docs.find(fullId) == want_docs.end()) {
                            continue;
                        }
                        if (dedup.find(ctx) != dedup.end()) {
                            continue;
                        }
                        dedup.insert(ctx);
                        vres.push_back(ctx);
                        mres[fullId] = ctx;
                    }
                    if (vres.size() != want_docs.size()) {
                        Cerr << "fail: " << line << " got: " << vres.size() << " wanted: " << want_docs.size() << Endl;
                        if (wantComplete) {
                            continue;
                        } else {
                            Cerr << "ok, will use these " << vres.size() << Endl;
                        }
                    }
                    TString requestId = CreateGuidAsString();
                    for (size_t i = 0; i < docids.size(); ++i) {
                        if (mres.find(docids[i]) == mres.end()) {
                            continue;
                        }
                        const TString& context = mres[docids[i]];
                        NProto::TSnippetsCtx ctx;
                        if (ctx.ParseFromString(Base64Decode(context))) {
                            ctx.MutableLog()->SetRequestId(requestId);
                            size_t regionId;
                            if (TryFromString<size_t>(cgi.Get("lr"), regionId)) {
                                ctx.MutableLog()->SetRegionId(regionId);
                            }
                            TString data;
                            Y_PROTOBUF_SUPPRESS_NODISCARD ctx.SerializeToString(&data);
                            data = Base64Encode(data);
                            Cout << data << Endl;
                        }
                    }
                } catch (...) {
                }
            }
            return 0;
        }
    };
    struct THamsterMain {
        TString Source;
        TString Type;
        THamsterMain(TStringBuf h) {
            if (h == "imghamster") {
                Source = "IMAGES";
                Type = "pictures";
            } else if (h == "videohamster") {
                Source = "VIDEO";
                Type = "video";
            } else {
                Source = "WEB";
                Type = "";
            }
        }
        int Run(int argc, char** argv) {
            TDomHosts hosts(argc == 1 ? argv[0] : "");
            TString line;
            TString tmp;
            TString ans;
            NXmlSearchIn::TRequest res;
            while (Cin.ReadLine(line)) {
                TVector<TStringBuf> v;
                {
                    TCharDelimiter<const char> d('\t');
                    TContainerConsumer< TVector<TStringBuf> > c(&v);
                    SplitString(line.data(), line.data() + line.size(), d, c);
                }
                if (v.empty()) {
                    continue;
                }
                TStringBuf req = v[0];
                TStringBuf restr = v.size() >= 2 ? v[1] : "";
                TStringBuf region = v.size() >= 3 ? v[2] : "";
                TStringBuf domRegion = v.size() >= 4 ? v[3] : "ru";
                const THost& dhost = hosts.Get(TString(domRegion.data(), domRegion.size()));
                if (!dhost.CanFetch()) {
                    continue;
                }
                try {
                    if (!dhost.XmlSearch(res, TString(req.data(), req.size()), Source, Type, TString(region.data(), region.size()))) {
                        continue;
                    }
                    THost mhost;
                    TStringBuf mpath;
                    TStringBuf mcgi;
                    if (!ParseUrl(res.FullQuery, mhost, mpath, mcgi)) {
                        continue;
                    }
                    TCgiParameters cgi(mcgi);
                    FixMultipleSnip(cgi);
                    if (restr.size()) {
                        AddRestrToQtrees(cgi, TString(restr.data(), restr.size()));
                    }
                    AskSnippetsCtx(cgi);
                    ans = mhost.Fetch(TString(mpath) + cgi.Print());
                } catch (...) {
                }
                Cout << res.QueryUTF8 << '\t' << res.Qtree << '\t' << Base64Encode(ans) << Endl;
            }
            return 0;
        }
    };
    struct TSessionsHamsterCtxMain {
        TDomHosts Hosts;
        TSessionsHamsterCtxMain() {
        }
        struct TPack {
            struct TDoc {
                TStringBuf Url;
                TString Ctx;

                TDoc() {
                }
                TDoc(TStringBuf url)
                  : Url(url)
                {
                }
            };

            TVector<TDoc> Docs;
        };
        TStringBuf Suggest(TStringBuf x, TVector<TStringBuf> v) {
            TStringBuf res;
            size_t resVal = 0;
            for (size_t i = 0; i < v.size(); ++i) {
                size_t cur = NLCS::MeasureLCS<char>(x, v[i]);
                if (cur > resVal) {
                    res = v[i];
                    resVal = cur;
                }
            }
            return res;
        }
        void FetchPack(TPack& pack, const TString& query, const TString& region, const TString& domRegion, const TString& source) {
            const THost& dhost = Hosts.Get(domRegion);
            if (!dhost.CanFetch()) {
                return;
            }
            for (size_t t = 0; t < 2; ++t) {
                TVector<TStringBuf> allUrls;
                THashMap<TString, TString*> urlToCtx;
                for (size_t i = 0; i < pack.Docs.size(); ++i) {
                    if (pack.Docs[i].Ctx.size()) {
                        continue;
                    }
                    allUrls.push_back(pack.Docs[i].Url);
                    urlToCtx[AddSchemePrefix(TString(pack.Docs[i].Url.data(), pack.Docs[i].Url.size()))] = &pack.Docs[i].Ctx;
                }
                if (!allUrls.size()) {
                    return;
                }

                for (size_t z = 0; z < allUrls.size(); z += 10) {
                    size_t w = Min(z + 10, allUrls.size());
                    TVector<TStringBuf> urls(allUrls.begin() + z, allUrls.begin() + w);
                    TString restr = GetUrlsRestr(urls);

                    try {
                        NXmlSearchIn::TRequest res;
                        if (!dhost.XmlSearch(res, query, source, "", region)) {
                            continue;
                        }
                        THost mhost;
                        TStringBuf mpath;
                        TStringBuf mcgi;
                        if (!ParseUrl(res.FullQuery, mhost, mpath, mcgi)) {
                            continue;
                        }
                        TCgiParameters cgi(mcgi);
                        FixMultipleSnip(cgi);
                        cgi.EraseAll("g");
                        cgi.EraseAll("ag");
                        cgi.EraseAll("ag0");
                        cgi.InsertUnescaped("g", "0..10");
                        AddRestrToQtrees(cgi, restr);
                        AskSnippetsCtx(cgi);
                        TString ans = mhost.Fetch(TString(mpath) + cgi.Print());

                        NMetaProtocol::TReport report;
                        if (!report.ParseFromString(ans)) {
                            continue;
                        }
                        NMetaProtocol::Decompress(report); // decompresses only if needed
                        for (size_t i = 0; i < report.GroupingSize(); ++i) {
                            const NMetaProtocol::TGrouping& grouping = report.GetGrouping(i);
                            for (size_t j = 0; j < grouping.GroupSize(); ++j) {
                                const NMetaProtocol::TGroup& group = grouping.GetGroup(j);
                                for (size_t k = 0; k < group.DocumentSize(); ++k) {
                                    const NMetaProtocol::TDocument& doc = group.GetDocument(k);
                                    if (!doc.HasArchiveInfo()) {
                                        continue;
                                    }
                                    const NMetaProtocol::TArchiveInfo& ar = doc.GetArchiveInfo();
                                    if (!ar.HasUrl()) {
                                        continue;
                                    }
                                    TString url = AddSchemePrefix(ar.GetUrl());
                                    if (urlToCtx.find(url) == urlToCtx.end()) {
                                        if (t != 1) {
                                            continue;
                                        }
                                        bool hacked = false;
                                        for (size_t l = 0; l < allUrls.size(); ++l) {
                                            TString nurl;
                                            if (url.size() + 1 == allUrls[l].size() && url + '/' == allUrls[l]
                                                || url.size() == allUrls[l].size() + 1 && url.StartsWith(allUrls[l]) && url[url.size() - 1] == '/') {

                                                nurl.assign(allUrls[l].data(), allUrls[l].size());
                                            } else {
                                                continue;
                                            }
                                            if (urlToCtx[nurl]->size()) {
                                                continue;
                                            }
                                            hacked = true;
                                            Cerr << "UNASK: " << query << " " << url << " " << source << " domain:" << domRegion << " lr=" << region << Endl;
                                            Cerr << "HACKD: " << query << " " << nurl << Endl;
                                            url = nurl;
                                            break;
                                        }
                                        if (!hacked) {
                                            Cerr << "UNASKED: " << query << " " << url << " " << source << " domain:" << domRegion << " lr=" << region << Endl;
                                            Cerr << "ASKED??: " << query << " " << Suggest(url, allUrls) << Endl;
                                            continue;
                                        }
                                    } else if (urlToCtx[url]->size()) {
                                        continue;
                                    }
                                    TStringBuf ctx = FindCtx(ar);
                                    if (!ctx.size()) {
                                        continue;
                                    }
                                    urlToCtx[url]->assign(ctx.data(), ctx.data() + ctx.size());
                                }
                            }
                        }
                    } catch (...) {
                        continue;
                    }
                }
            }
        }
        int Run(int, char**) {
            TSessionEntry e;
            typedef THashMap<TString, TPack> TPacks;
            TPacks packs;
            size_t total = 0, fail = 0;
            while (ParseNextMREntry(e, &Cin)) {
                if (e.Type != "REQUEST" || !e.Query.size() || e.Res.empty() || e.Service != "www.yandex" || e.UI != "www.yandex" || e.TestBuckets.size()) {
                    continue;
                }
                packs.clear();
                for (size_t i = 0; i < e.Res.size(); ++i) {
                    packs[e.Res[i].Source].Docs.push_back(TPack::TDoc(AddSchemePrefix(e.Res[i].Url)));
                }
                for (TPacks::iterator it = packs.begin(); it != packs.end(); ++it) {
                    FetchPack(it->second, e.Query, e.UserRegion, e.DomRegion, it->first);
                    for (size_t i = 0; i < it->second.Docs.size(); ++i) {
                        ++total;
                        if (it->second.Docs[i].Ctx.size()) {
                            Cout << it->second.Docs[i].Ctx << Endl;
                        } else {
                            ++fail;
                            Cerr << "FAIL: " << e.Query << " " << it->second.Docs[i].Url << " " << it->first << " domain:" << e.DomRegion << " lr=" << e.UserRegion << Endl;
                            Cerr << fail << " of " << total << " failed (" << fail * 1.0 / total << "%)" << Endl;
                        }
                    }
                }
            }
            return 0;
        }
    };
}

int main(int argc, char** argv) {
    --argc, ++argv;
    if (argc >= 1 && (!strcmp(argv[0], "sessions") || !strcmp(argv[0], "imgsessions") || !strcmp(argv[0], "videosessions"))) {
        TStringBuf s = argv[0];
        --argc, ++argv;
        return NSnippets::TSessionsMain(s).Run(argc, argv);
    } else if (argc >= 1 && (!strcmp(argv[0], "hamster") || !strcmp(argv[0], "imghamster") || !strcmp(argv[0], "videohamster"))) {
        TStringBuf h = argv[0];;
        --argc, ++argv;
        return NSnippets::THamsterMain(h).Run(argc, argv);
    } else if (argc >= 1 && !strcmp(argv[0], "sessions-hamster-ctx")) {
        --argc, ++argv;
        return NSnippets::TSessionsHamsterCtxMain().Run(argc, argv);
    } else if (argc >= 1 && !strcmp(argv[0], "hamsterfest")) {
        --argc, ++argv;
        return NSnippets::THamsterFestMain().Run(argc, argv);
    } else {
        Cerr << "unknown mode" << Endl;
        return 1;
    }
}
