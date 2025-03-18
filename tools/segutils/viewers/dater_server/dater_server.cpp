#include <tools/segutils/segcommon/data_utils.h>

#include <kernel/segutils/numerator_utils.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/http/server/http_ex.h>

#include <util/generic/hash.h>
#include <util/random/shuffle.h>
#include <util/stream/tee.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/system/hp_timer.h>
#include <util/system/rusage.h>
#include <util/system/thread.h>
#include <util/thread/pool.h>

using namespace NDater;

namespace NSegutils {

struct TDaterServerOpts {
    TString ConfigDir;
    TString Format;
    TDaterDate Now;
    ui32 Port;
    ui32 NThreads;
    ui32 NDloadThreads;
    bool Offline;
public:
    TDaterServerOpts()
        : ConfigDir(".")
        , Format("%G")
        , Now(TDaterDate::Now())
        , Port(18080)
        , NThreads(2)
        , NDloadThreads(20)
        , Offline()
    {
    }
};

typedef THashMap<TString, TDaterDate> TDatingResult;

class TDaterTask: public IObjectInQueue {
    TString Url;
    TDaterDate Now;
    TDaterDate Date;
public:
    TDaterTask(TString url, TDaterDate now)
        : Url(url)
        , Now(now)
        , Date()
    {
        if(!now)
            ythrow yexception();
    }

    bool Has(const char * s) {
        return Url.find(s) != TString::npos;
    }

    void GetDate(THtmlDocument data, TParserContext* ctx) {
        TDaterContext dctx(*ctx);
        data.Time.SetDate(Now);
        dctx.SetDoc(data);
        dctx.NumerateDoc();
        TDatePosition best = dctx.GetBestDate();

        if (best.IsMoreTrusted()) {
            Date = best;
            return;
        }
    }

    void PrintDownloadMessage() const {
        Clog << TThread::CurrentThreadId() << " downloading " << Url << Endl;
    }

    void PrintStartMessage() const {
        Clog << TThread::CurrentThreadId() << Sprintf(" started processing (RSS=%uM) ",
                                                      (ui32)(TRusage::Get().MaxRss >> 20))
                                    << Url << Endl;
    }

    void PrintFinishMessage(NHPTimer::STime start) const {
        Clog << TThread::CurrentThreadId() << Sprintf(" finished processing (RSS=%uM, took %gsec) ",
                                                      (ui32)(TRusage::Get().MaxRss >> 20) , NHPTimer::GetTimePassed(&start))
                                    << Url << Endl;
    }

    void Process(void* res) override {
        try {

            PrintDownloadMessage();

            THtmlDocument data = Fetch(Url);

            if (MIME_HTML != data.HttpMime)
                return;

            NHPTimer::STime start = 0;
            NHPTimer::GetTime(&start);
            PrintStartMessage();
            GetDate(data, (TParserContext*) res);
            PrintFinishMessage(start);

        } catch (const yexception& e) {
            Clog << e.what() << Endl;
            return;
        } catch (const std::exception& ex) {
            Clog << ex.what() << Endl;
            return;
        }
    }

    TString GetUrl() const {
        return Url;
    }

    TDaterDate GetDate() const {
        return Date;
    }
};

typedef TList<TDaterTask> TDaterTasks;

class TDaterQueue: public TThreadPool {
    TString ConfigDir;
public:
    TDaterQueue(TString confdir) :
        ConfigDir(confdir) {
    }

    void* CreateThreadSpecificResource() override {
        return new TParserContext(ConfigDir);
    }

    void DestroyThreadSpecificResource(void* resource) override {
        delete (TParserContext*) resource;
    }
};

class TDaterRequest: public THttpClientRequestEx {
    TDaterServerOpts Opts;
    TVector<TString> Urls;
    TDatingResult Results;
public:
    TDaterRequest(TDaterServerOpts opts)
        : Opts(opts)
    {
    }

    void SetUrls(const TVector<TString>& urls) {
        Urls = urls;
    }

    bool Reply(void*) override {
        if (!ProcessHeaders()) {
            Clog << "bad request" << Endl;
            return true;
        }

        TVector<TString> urls;
        TContainerConsumer<TVector<TString>> c(&urls);
        TSkipEmptyTokens<TContainerConsumer<TVector<TString>> > cc(&c);
        SplitString(Buf.AsCharPtr(), Buf.AsCharPtr() + Buf.Size(), TCharDelimiter<const char>('\n'), cc);

        for (TVector<TString>::iterator it = urls.begin(); it != urls.end(); ++it) {
            StripInPlace(*it);
            if (!it->empty())
                Urls.push_back(AddSchemePrefix(*it));
        }

        RD.Scan();
        TDaterDate now = TDaterDate::FromString(RD.CgiParam.Get("now"));
        GetDates(now);
        PrintDates(now);

        return true;
    }

    void PrintDates(TDaterDate now) {
        TString buff;

        for (TVector<TString>::const_iterator it = Urls.begin(); it != Urls.end(); ++it) {
            TString url = *it;
            if (!Results.contains(url))
                continue;

            TDaterDate d = Results[url];

            buff.append(url).append('\t').append(d.ToString(Opts.Format, now));
            buff.append('\n');
        }

        if (Opts.Offline) {
            Cout << buff;
        } else {
            TTeeOutput output(&Output(), &Clog);
            output << "HTTP/1.0 200 OK\r\n" << "Content-Type: text/plain; charset=utf8\r\n"
                    << "Content-Length: " << buff.size() << "\r\n\r\n" << buff;
        }

        Clog << Results.size() << " urls of " << Urls.size() << " dated" << Endl;
    }

    void GetDates(TDaterDate now) {
        TVector<TString> urls = Urls;

        Shuffle(urls.begin(), urls.end());

        TDaterQueue queue(Opts.ConfigDir);
        TDaterTasks tasks;

        for (TVector<TString>::const_iterator it = urls.begin(); it != urls.end(); ++it) {
            tasks.push_back(TDaterTask(*it, now));
        }

        Clog << "Starting queue" << Endl;
        queue.Start(Opts.NDloadThreads);
        Clog << "Started queue" << Endl;

        for (TDaterTasks::iterator it = tasks.begin(); it != tasks.end(); ++it) {
            if (!queue.Add(&*it))
                ythrow yexception ();
        }

        Clog << "Stopping queue" << Endl;
        queue.Stop();
        Clog << "Stopped queue" << Endl;

        for (TDaterTasks::const_iterator it = tasks.begin(); it != tasks.end(); ++it) {
            if (it->GetDate()) {
                Results[it->GetUrl()] = it->GetDate();
            }
        }
    }
};

class TDaterServer: public THttpServer::ICallBack, public THttpServer {
    THttpServer::TOptions CreateOptions(ui16 port, ui16 nThreads) {
        THttpServer::TOptions options;
        options.Port = port;
        options.nThreads = nThreads;
        return options;
    }

    TDaterServerOpts Opts;
public:
    TDaterServer(TDaterServerOpts opts)
        : THttpServer(this, CreateOptions(opts.Port, opts.NThreads))
        , Opts(opts)
    {
    }

    void AddLog(const TString& l) {
        Clog << l << Endl;
    }
protected:
    TClientRequest* CreateClient() override {
        return new TDaterRequest(Opts);
    }
};

void Usage(const char * me) {
    TDaterServerOpts opts;
    Cerr << "A service to download and date documents. "
        "The documents can be supplied either by stdin or by HTTP POST body." << Endl;
    Cerr << me
            << " --help | --format-help | [-p port] [-c configDir] [-n nThreads] [-d nDloadThreads] [-f format] [-o] [-q refdate]"
            << Endl;
    Cerr << "\t" << "-p port: port for handling requests [" << opts.Port << "]" << Endl;
    Cerr << "\t" << "-c configDir: dir with htparser.ini, dict.dict and 2ld.list ["
            << opts.ConfigDir << "]" << Endl;
    Cerr << "\t" << "-n nThreads: n threads to run [" << opts.NThreads << "]" << Endl;
    Cerr << "\t" << "-d nDloadThreads: n threads to use to download web documents ["
            << opts.NDloadThreads << "]" << Endl;
    Cerr << "\t" << "-f format : format in which to output dates [" << opts.Format << "], see also --format-help" << Endl;
    Cerr << "\t" << "-o : offline mode, take urls from stdin" << Endl;
    Cerr << "\t" << "-q refdate : reference date from which to calculate the difference, works only for the offline mode. For the server mode use HTTP request parameter now=" << Endl;
}

void GetOpts(int argc, const char** argv, TDaterServerOpts& dsopts) {

    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        Usage(argv[0]);
        exit(0);
    }

    if (argc > 1 && strcmp(argv[1], "--format-help") == 0) {
        Clog << TDaterDate::FormatHelp();
        exit(0);
    }

    Opt opts(argc, argv, "c:d:f:n:p:q:or");
    int optlet;

    while (EOF != (optlet = opts.Get())) {
        switch (optlet) {
        case 'c':
            dsopts.ConfigDir = opts.GetArg();
            break;
        case 'd':
            dsopts.NDloadThreads = FromString<int> (opts.GetArg());
            break;
        case 'f':
            dsopts.Format = opts.GetArg();
            break;
        case 'n':
            dsopts.NThreads = FromString<int> (opts.GetArg());
            break;
        case 'p':
            dsopts.Port = FromString<int> (opts.GetArg());
            break;
        case 'q':
            dsopts.Now = TDaterDate::FromString(opts.GetArg());
            break;
        case 'o':
            dsopts.Offline = true;
            break;
        default:
            Usage(argv[0]);
            exit(0);
        }
    }
}

}

int main(int argc, const char**argv) {
    using namespace NSegutils;
    TDaterServerOpts opts;

    GetOpts(argc, argv, opts);

    if (opts.Offline) {
        TString l;
        TVector<TString> urls;
        while (Cin.ReadLine(l)) {
            StripInPlace(l);

            if (l.empty())
                continue;

            urls.push_back(AddSchemePrefix(l));
        }

        if (!urls.empty()) {
            Clog << "Taking data from the stdin, no server will start." << Endl;
            TDaterRequest r(opts);
            r.SetUrls(urls);
            Clog << "Downloading documents." << Endl;
            TDaterDate now = opts.Now ? opts.Now : TDaterDate::Now();
            r.GetDates(now);
            Clog << "Printing results." << Endl;
            r.PrintDates(now);
        }
    } else {
        signal(SIGPIPE, SIG_IGN);

        Clog << "Creating server" << Endl;

        TDaterServer serv(opts);

        Clog << "Port: " << opts.Port << " NThreads: " << opts.NThreads << " NDloadThreads: "
                << opts.NDloadThreads << " ConfigDir: " << opts.ConfigDir << " Format: "
                << opts.Format << Endl;

        serv.AddLog("Starting server");
        serv.Start();
        serv.AddLog("Waiting for requests");

        serv.Wait();

        serv.AddLog("Turning off");
        sleep(1);
    }
}
