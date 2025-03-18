#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>

#include <library/cpp/http/server/http_ex.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>
#include <util/folder/path.h>

namespace NHtmlStats
{

struct THtmlStatsData
{
    TFsPath CachePath;
    NHtmlStats::THashGroups HashGroups;
    NHtmlStats::TUrlGrouper UrlGrouper;
    THtmlParser Parser;
    TParserOptions ParserOptions;

    THtmlStatsData(const TString& statsFileName, const TString& cachePath)
        : CachePath(cachePath)
        , ParserOptions(TParserOptions().DoReplaceNumbers())
    {
        TFileInput inf(statsFileName);
        LoadHashes(inf, HashGroups);
        UrlGrouper.LoadGroups("mined_groups.txt");
    }
};

using THtmlStatsDataPtr = TAtomicSharedPtr<THtmlStatsData>;

class TFetchRequest: public THttpClientRequestEx
{
    THtmlStatsDataPtr StatsData;

public:
    TFetchRequest(THtmlStatsDataPtr statsData)
        : StatsData(statsData)
    {
    }

    void ReplyError(const TString& code, const TString& expl)
    {
        Cerr << code << expl << Endl;
        Output() << "HTTP/1.0 " << code << "\r\nContent-Type: text/plain\r\n\r\n" << expl;
        return;
    }

    bool CheckValidFileName(const TString& scriptName) {
        if (scriptName.length() != 64) {
            return false;
        }
        for (int i = 0; i < 64; ++i) {
            char c = scriptName[i];
            if ( ! ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) ) {
                return false;
            }
        }
        return true;
    }

    void Reply(const TString& scriptName)
    {
        TString encName = RD.CgiParam.Get("encoding", 0);
        TString url = RD.CgiParam.Get("url", 0);
        ECharset encoding = CODES_UNKNOWN;
        int tmp;
        if (TryFromString(encName, tmp)) {
            encoding = (ECharset)tmp;
        }

        if (encoding == CODES_UNKNOWN || encoding == CODES_UNSUPPORTED) {
            ReplyError("400 Bad Request", "Unsupported encoding");
            return;
        }

        TFsPath path = StatsData->CachePath / scriptName;

        if (/*!CheckValidFileName(scriptName) || */!path.Exists()) {
            ReplyError("404 Not Found", "Cache file not found");
            return;
        }

        TString html;
        {
            TUnbufferedFileInput inf(path.GetPath());
            html = inf.ReadAll();
        }

        TString group = StatsData->UrlGrouper.GetGroupName(url);
        THashGroups::const_iterator groupIter = StatsData->HashGroups.find(group);
        if (groupIter == StatsData->HashGroups.end()) {
            ReplyError("400 Bad Request", "Unknown URL host");
            return;
        }

        THashGroup* hashes = groupIter->second.Get();
        TSentenceEvaluator evaluator(*hashes);
        StatsData->Parser.Parse(url, html, encoding, &evaluator, StatsData->ParserOptions);

        Output() << "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\n";
        for (const auto& sent : evaluator.Sentences) {
            Output() << sent.Freq << "\t" << sent.Hash << "\t" << WideToUTF8(sent.Sent) << Endl;
        }
    }

    bool Reply(void*) override
    {
        try {
            ProcessHeaders();
            RD.Scan();
            TStringBuf scriptName = RD.ScriptName();
            Cerr << "Incoming request: " << scriptName << Endl;
            if (scriptName.StartsWith('/')) {
                scriptName = scriptName.substr(1);
            }
            Reply(TString{scriptName});
            Output().Finish();
        }
        catch (const yexception& ye) {
            Output() << "HTTP/1.1 503 Internal error\r\n\r\n" << ye.what();
            Output().Finish();
            Cerr << "Exception: " << ye.what() << Endl;
        }

        return true;
    }
};

class TServer: public THttpServer, public THttpServer::ICallBack
{
    THtmlStatsDataPtr StatsData;

public:
    TServer(THttpServer::TOptions& httpOpts, THtmlStatsDataPtr statsData)
        : THttpServer(this, httpOpts)
        , StatsData(statsData)
    {
    }

protected:
    TClientRequest* CreateClient() override {
        return new TFetchRequest(StatsData);
    }
};

}

int main(int argc, const char* argv[])
{
    using namespace NLastGetopt;

    int port;
    TString cachedir;

    TOpts opt;
    opt.AddLongOption('p', "port", "port number").StoreResult(&port).DefaultValue("45011");
    opt.AddLongOption('c', "cachedir", "path to hashed file storage").StoreResult(&cachedir).DefaultValue(".cache");
    opt.SetFreeArgsMin(1);
    opt.SetFreeArgsMax(1);
    opt.SetFreeArgTitle(0, "stats-file", "Fragment popularity database");
    TOptsParseResult o(&opt, argc, argv);

    if (port <= 0 || port > 65535) {
        ythrow yexception() << "Illegal port number: " << port;
    }

    const TString hashFile = o.GetFreeArgs().at(0);

    Cerr << "Loading stats data" << Endl;
    TAtomicSharedPtr<NHtmlStats::THtmlStatsData> statsData;
    statsData.Reset(new NHtmlStats::THtmlStatsData(hashFile, cachedir));

    THttpServer::TOptions httpOpts;
    httpOpts.AddBindAddress("0.0.0.0", port);
    httpOpts.AddBindAddress("::0", port);
    NHtmlStats::TServer server(httpOpts, statsData);
    server.Start();
    Cerr << "Listening on port " << port << Endl;
    server.Wait();
}
