#pragma once

#include "http_logging.h"

#include <tools/clustermaster/communism/util/http_util.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/server/http.h>

#include <util/generic/noncopyable.h>

class TWorkerHttpServer;
class TWorkerGraph;

class TWorkerHttpRequest: public THttpClientRequestForHuman<TClustermasterHttpRequestLogger>, TNonCopyable {
public:
    TWorkerHttpRequest()
    {
    }
private:
    TWorkerGraph& GetGraph();

private:
    void ReplyUnsafe(IOutputStream&) override;

    void ServeTorrentShare(IOutputStream& out, const TString& path);
    void ServeTorrentDownload(IOutputStream& out, const TString& path, const TString& torrent);

    void ServeStyleCss(IOutputStream&);
    void Serve404(IOutputStream&);
    void ServeSimpleStatus(IOutputStream&, HttpCodes code, const TString& annotation = "");
    void ServeLog(IOutputStream&, const TString& target, const TString& task, ui8 version = 0, bool fullogs = false);
    void ServeLog(IOutputStream&);
    void ServePS(IOutputStream&, const TString& target);
    void ServeDump(IOutputStream&);
    void ServeResourcesHint(IOutputStream&, const TString& target, const TString& taskNStr);
    void ServeProxySolver(IOutputStream&, const TString& targetName, const TString& taskN, const TString& solverUrl);
};

class TWorkerHttpServer: public THttpServer, THttpServer::ICallBack {
public:
    TWorkerHttpServer(TWorkerGraph& tg, THttpServer::TOptions options)
        : THttpServer(this, options)
        , TargetGraph(tg)
    {
    }

    TWorkerGraph& GetGraph() {
        return TargetGraph;
    }

protected:
    TClientRequest* CreateClient() override {
        return new TWorkerHttpRequest();
    }

protected:
    TWorkerGraph& TargetGraph;
};

ui8 GetVersion(const TVector<TString>& url);
