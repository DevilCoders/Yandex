#include "http.h"

#include "http_static.h"
#include "log.h"
#include "transf_file.h"
#include "worker.h"
#include "worker_target_graph.h"

#include <tools/clustermaster/common/async_jobs.h>
#include <tools/clustermaster/common/util.h>
#include <tools/clustermaster/communism/util/http_util.h>

#include <library/cpp/http/misc/httpdate.h>
#include <library/cpp/uri/encode.h>
#include <library/cpp/xml/encode/encodexml.h>

#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>
#include <util/network/sock.h>
#include <util/stream/pipe.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/util.h>
#include <util/system/fstat.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/system/pipe.h>

struct TTorrentInfo {
    unsigned Code;
    TString   Info;
    time_t   Modified;
};

class TTorrentJobs : public TAsyncJobs {
public:
    TTorrentJobs()
        : TAsyncJobs(10) {
        Start();
    }

    ~TTorrentJobs() noexcept {
        Stop();
    }

    const TTorrentInfo* GetTorrentInfo(const TString& path) const noexcept {
        const auto it = TorrentInfo.find(path);
        if (it == TorrentInfo.end()) {
            return nullptr;
        }
        return &it->second;
    }

    const TTorrentInfo& AddTorrentInfo(const TString& path, const TTorrentInfo& ti) {
        const auto result = TorrentInfo.emplace(path, ti);
        if (!result.second) {
            result.first->second = ti;
        }
        return result.first->second;
    }

    TMutex& GetMutex() noexcept {
        return Mutex;
    }

private:
    THashMap<TString, TTorrentInfo> TorrentInfo;
    TMutex Mutex;
};


TWorkerGraph& TWorkerHttpRequest::GetGraph() {
    return reinterpret_cast<TWorkerHttpServer*>(HttpServ())->GetGraph();
}

void TWorkerHttpRequest::ReplyUnsafe(IOutputStream& out) {
    TMaybe<THttpRequestContext> context = THttpRequestContext::Cons(RD, RequestString);

    if (context.Empty()) {
        NHttpUtil::ServeSimpleStatus(out, HTTP_BAD_REQUEST, "", "");
    }

    TVector<TString> url = context->Url;

    if (url.size() == 0) {
        Serve404(out);
        return;
    } else if (url.size() == 1 && url[0] == "style.css") {
        ServeStyleCss(out);
        return;
    } else if (url.size() >= 1 && url[0] == "log") {
        if (url.size() == 2) {
            LogLevel::Level() = FromString<ELogLevel>(url[1]);
            ServeSimpleStatus(out, HTTP_OK);
        } else {
            ServeLog(out);
        }
        return;
    } else if (url.size() == 1 && url[0] == "ps") {
        ServePS(out, "ps auxww | sort");
        return;
    } else if (url.size() == 1 && url[0] == "pstree") {
        ServePS(out, Sprintf("pstree -p %d", getpid()));
        return;
    } else if ((url.size() == 3 || url.size() == 4) && url[0] == "logs") {
        ServeLog(out, url[1], url[2], GetVersion(url));
        return;
    } else if ((url.size() == 3 || url.size() == 4) && url[0] == "fullogs") {
        ServeLog(out, url[1], url[2], GetVersion(url), true);
        return;
    } else if (url.size() == 3 && url[0] == "res") {
        ServeResourcesHint(out, url[1], url[2]);
        return;
    } else if (url[0] == "dump") {
        ServeDump(out);
        return;
    } else if (url.size() >= 3 && url[0] == "proxy_solver") {
        TString solverUrl = "";

        if (url.size() == 3)
            solverUrl = "/";
        else
            for (unsigned int i = 3; i < url.size(); ++i)
                solverUrl += "/" + url[i];

        ServeProxySolver(out, url[1], url[2], solverUrl);

        return;
    } else if (url.size() >= 3 && url[0] == "torrent") {
        TString path;
        TStringOutput outPath(path);
        NUri::TEncoder::Decode(outPath, url[2], 0);

        if (url[1] == "share") {
            ServeTorrentShare(out, TFsPath(path).RealPath());
            return;
        } else if (url.size() == 4 && url[1] =="load") {
            TString torrent;
            TStringOutput outTorrent(torrent);
            NUri::TEncoder::Decode(outTorrent, url[3], 0);

            ServeTorrentDownload(out, path, torrent);
            return;
        }
    }

    Serve404(out);
}

void TWorkerHttpRequest::ServeTorrentShare(IOutputStream& out, const TString& path) {
    TGuard<TMutex> guard(Singleton<TTorrentJobs>()->GetMutex());

    const TTorrentInfo* ti0 = Singleton<TTorrentJobs>()->GetTorrentInfo(path);
    if (ti0) {
        if (ti0->Code == 200 && ti0->Modified == TFileStat(path).MTime) {
            DEBUGLOG1(http, "Torrent sharing has been succeeded: " << ti0->Info);
            out << "HTTP/1.0 200 OK\r\n";
            out << "Cache-control: no-cache, max-age=0\r\n";
            out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
            out << "\r\n";
            out << ti0->Info;
            return;
        }
        else {
            time_t ifModifiedSince = 0;
            const TString *ims = RD.HeaderIn("If-Modified-Since");
            if (ims) {
                ParseHTTPDateTime(ims->c_str(), ifModifiedSince);
            }

            if (ti0->Code == 100 && ti0->Modified == ifModifiedSince) {
                DEBUGLOG1(http, "Waiting for torrent sharing: " << ti0->Info);
                out << "HTTP/1.0 202 Accepted\r\n";
                out << "Cache-control: no-cache, max-age=0\r\n";
                out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
                out << "\r\n";
                out << (ims ? ims->c_str() : "");
                return;
            }

            DEBUGLOG1(http, "Torrent sharing has been failed: " << ti0->Info);
            out << "HTTP/1.0 500 Internal Server Error\r\n";
            out << "Cache-control: no-cache, max-age=0\r\n";
            out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
            out << "\r\n";
            out << "Torrent sharing has been failed";
            return;
        }
    }

    DEBUGLOG1(http, "Queue torrent sharing job " << path);
    const TTorrentInfo& ti = Singleton<TTorrentJobs>()->AddTorrentInfo(path, { 100, "Sharing", time(0) });

    Singleton<TTorrentJobs>()->Add(MakeHolder<TSomeJob>([path]() {
        DEBUGLOG1(http, "Execute torrent sharing");
        try {
            TFsPath p(path);
            TString dir = p.Parent();
            TString name = p.GetName();
            if (p.IsDirectory() && TPathSplit::IsPathSep(path.back())) {
                dir = p;
                name = ".";
            }

            DEBUGLOG1(http, TString("sky share -d ") + dir + " " + name);
            TString torrent = TPipeInput(TString("sky share -d ") + dir + " " + name).ReadLine();
            Singleton<TTorrentJobs>()->AddTorrentInfo(path, { 200, torrent, TFileStat(path).MTime });
        }
        catch (const TSystemError&) {
            //pclose() returns -1 because of sigchld handler
        }
        catch (const yexception& e) {
            DEBUGLOG1(http, "Share error: " << e.what());
            Singleton<TTorrentJobs>()->AddTorrentInfo(path, { 500, e.what(), time(0) });
            return;
        }
    }));

    out << "HTTP/1.0 202 Accepted\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
    out << "\r\n";
    out << FormatHttpDate(ti.Modified);
}

void TWorkerHttpRequest::ServeTorrentDownload(IOutputStream& out, const TString& path, const TString& torrent) {
    TGuard<TMutex> guard(Singleton<TTorrentJobs>()->GetMutex());

    const TTorrentInfo* ti0 = Singleton<TTorrentJobs>()->GetTorrentInfo(torrent);
    if (ti0) {
        if (ti0->Code == 200) {
            DEBUGLOG1(http, "Torrent downloading has been succeeded: " << ti0->Info);
            out << "HTTP/1.0 200 OK\r\n";
            out << "Cache-control: no-cache, max-age=0\r\n";
            out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
            out << "\r\n";
            out << ti0->Info;
            return;
        }
        else {
            time_t ifModifiedSince = 0;
            const TString *ims = RD.HeaderIn("If-Modified-Since");
            if (ims) {
                ParseHTTPDateTime(ims->c_str(), ifModifiedSince);
            }

            if (ti0->Code == 100 && ti0->Modified == ifModifiedSince) {
                DEBUGLOG1(http, "Waiting for torrent downloading: " << torrent << " " << ti0->Info);
                out << "HTTP/1.0 202 Accepted\r\n";
                out << "Cache-control: no-cache, max-age=0\r\n";
                out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
                out << "\r\n";
                out << (ims ? ims->c_str() : "");
                return;
            }

            DEBUGLOG1(http, "Torrent downloading has been failed: " << torrent << " " << ti0->Info);
            out << "HTTP/1.0 500 Internal Server Error\r\n";
            out << "Cache-control: no-cache, max-age=0\r\n";
            out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
            out << "\r\n";
            out << "Torrent downloading has been failed";
            return;
        }
    }

    DEBUGLOG1(http, "Queue torrent downloading job " << torrent << " " << path);
    const TTorrentInfo& ti = Singleton<TTorrentJobs>()->AddTorrentInfo(torrent, { 100, "Downloading", time(0) });

    Singleton<TTorrentJobs>()->Add(MakeHolder<TSomeJob>([path, torrent]() {
        DEBUGLOG1(http, "Execute torrent downloading");
        try {
            DEBUGLOG1(http, TString("sky get -u -d ") + path + " " + torrent);
            TPipeInput(TString("sky get -u -d ") + path + " " + torrent);
            Singleton<TTorrentJobs>()->AddTorrentInfo(torrent, { 200, path, 0 });
        }
        catch (const TSystemError&) {
            //pclose() returns -1 because of sigchld handler
        }
        catch (const yexception& e) {
            DEBUGLOG1(http, "Download error: " << e.what());
            Singleton<TTorrentJobs>()->AddTorrentInfo(torrent, { 500, e.what(), time(0) });
        }
    }));

    out << "HTTP/1.0 202 Accepted\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
    out << "\r\n";
    out << FormatHttpDate(ti.Modified);
}

void TWorkerHttpRequest::ServeStyleCss(IOutputStream& out) {
    PrintSimpleHttpHeader(out, "text/css");

    GetStaticFile("/style.css", out);
}

void TWorkerHttpRequest::ServeLog(IOutputStream& out, const TString& target, const TString& task, ui8 version, bool fullogs) {
    DEBUGLOG1(http, RD.RemoteAddr() << ": Log requested for target " << target << " task " << task);

    if (!GetGraph().HasTarget(target)) {
        Serve404(out);
        return;
    }

    // Only return 204 if we cannot open logFile
    TString filename = Sprintf("%s/%s.%03d.log", GetGraph().WorkerGlobals->LogDirPath.data(), target.data(), FromString<unsigned>(task));
    if (version != 0) {
        filename += Sprintf(".%d", version);
    }
    TFILEPtr in(filename, "r");

    try {
        PrintSimpleHttpHeader(out, "text/plain");
        // UTF-BOM to force log opening in browser window
        out << "\xEF\xBB\xBF";

        if (fullogs)
            TransferDataLimited(in, out, 0, in.GetLength());
        else
            TransferDataLimited(in, out, 65536, 65536);
    } catch (const yexception& e) {
        ERRORLOG1(http, RD.RemoteAddr() << ": Error during transfering data: " << e.what());
    }
}

void TWorkerHttpRequest::ServeLog(IOutputStream& out) {
    DEBUGLOG1(http, RD.RemoteAddr() << ": Worker log requested");

    if (GetGraph().WorkerGlobals->LogFilePath.empty()) {
        DEBUGLOG1(http, RD.RemoteAddr() << ": Cannot serve worker log: log file is not set");
        Serve404(out);
        return;
    }

    TFILEPtr in(GetGraph().WorkerGlobals->LogFilePath.data(), "r");

    PrintSimpleHttpHeader(out, "text/plain");
    // UTF-BOM to force log opening in browser window
    out << "\xEF\xBB\xBF";

    TransferDataLimited(in, out, 0, 1024 * 1024);
}

void TWorkerHttpRequest::ServePS(IOutputStream& out, const TString& target) {
    DEBUGLOG1(http, RD.RemoteAddr() << ": PS requested");

    TPipeInput ps(target);

    PrintSimpleHttpHeader(out, "text/html");

    out << "<html><head><title>Processlist</title></head><body><pre>\n";

    try {
        TString line;
        // Looks like pclose() on TPipeInput::DoRead will always throw an exception at the end of reading.
        while (ps.ReadLine(line)) {
            if (line.StartsWith("USER"))
                out << "<span style=\"font-weight: bold\">";
            else if (line.StartsWith("aspam"))
                out << "<span style=\"color: #008000; font-weight: bold\">";
            else if (line.StartsWith("webbase"))
                out << "<span style=\"color: #0000C0; font-weight: bold\">";
            else
                out << "<span>";

            out << EncodeXMLString(line.data());

            out << "</span>\n";
        }
    } catch (const yexception& /*e*/) {}

    out << "</pre></body></html>";
}

void TWorkerHttpRequest::ServeDump(IOutputStream& out) {
    out << "HTTP/1.0 200 OK\r\n";
    out << "Connection: close\r\n";
    out << "Content-Type: text/plain\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
    out << "\r\n";

    GetGraph().DumpState(out);
}

void TWorkerHttpRequest::ServeResourcesHint(IOutputStream& out, const TString& target, const TString& taskNStr) {
    DEBUGLOG1(http, RD.RemoteAddr() << ": Resources state requested for target " << target << " task " << taskNStr);

    try {
        TString hint;
        TStringOutput hintOut(hint);
        GetGraph().FormatResourcesHint(target, taskNStr, hintOut);

        PrintSimpleHttpHeader(out, "text/plain");

        out << hint.data();
    } catch (const TWorkerGraph::TNoResourcesHint& e) {
        ERRORLOG1(http, RD.RemoteAddr() << ": No resources state for target " << target << " task " << taskNStr << ": " << e.what());
        return Serve404(out);
    }
}

void TWorkerHttpRequest::Serve404(IOutputStream& out) {
    out << "HTTP/1.0 404 Not found\r\n";
    out << "Connection: close\r\n";
    out << "Cache-control: no-cache, max-age=0\r\n";
    out << "Expires: Thu, 01 Jan 1970 00:00:01 GMT\r\n";
    out << "\r\n";
    out << "404\n";
}

void TWorkerHttpRequest::ServeSimpleStatus(IOutputStream& out, HttpCodes code, const TString& annotation) {
    TString title = "ClusterMaster worker: error " + ToString<int>(static_cast<int>(code));;
    NHttpUtil::ServeSimpleStatus(out, code, "/", title, annotation);
}

void TWorkerHttpRequest::ServeProxySolver(IOutputStream& out, const TString& targetName, const TString& taskN, const TString& solverUrl) {
    TAutoPtr<TNetworkAddress> solverHttpAddress;
    try {
        solverHttpAddress = GetGraph().GetSolverHttpAddressForTask(targetName, taskN);
    } catch (const yexception& e) {
        ServeSimpleStatus(out, HTTP_INTERNAL_SERVER_ERROR, e.what());
        return;
    }

    try {
        TSocket sock(*solverHttpAddress);

        sock.SetSocketTimeout(10, 0);

        TSocketOutput sockOut(sock);
        TSocketInput sockIn(sock);

        // Send request
        sockOut << TString("GET ") + solverUrl + " HTTP/1.1\r\n";

        // Send headers
        for (auto i = ParsedHeaders.begin(); i != ParsedHeaders.end(); ++i) {

            // name
            sockOut << i->first << ":";

            // value
            sockOut << i->second;

            sockOut << "\r\n";
        }
        sockOut << "\r\n";

        sockOut.Finish();

        out << sockIn.ReadAll();
    } catch (const yexception& e) {
        ServeSimpleStatus(out, HTTP_INTERNAL_SERVER_ERROR, TString("Error connecting to solver: ") + EncodeXMLString(e.what()));
        return;
    }
}

ui8 GetVersion(const TVector<TString>& url) {
    ui8 version = 0;
    if (url.size() == 4) {
        version = FromString<ui8>(url[3]);
    }
    return version;
}
