#include <tools/snipmake/metasnip/jobqueue/jobqueue.h>
#include <tools/snipmake/metasnip/jobqueue/mtptsr.h>

#include <library/cpp/getopt/opt.h>

#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/network/socket.h>
#include <util/stream/file.h>
#include <library/cpp/http/io/stream.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/split.h>

namespace NSnippets
{

    struct TParams {
        TString Host;
        ui16 Port;
        ui16 Threads;
        ui16 MaxQueue;
        TString InputFName;
        TParams()
          : Host()
          , Port(0)
          , Threads(1)
          , MaxQueue(1)
        {
        }
    };
    struct TJob;
    struct TWorker {
        TSocket SearchSocket;
        void Process(TJob* job);
    };
    struct TThreadDataManager : IThreadSpecificResourceManager {
        const TParams& Params;
        TThreadDataManager(const TParams& params)
          : Params(params)
        {
        }
        void* CreateThreadSpecificResource() override {
            THolder<TWorker> h(new TWorker());
            TNetworkAddress addr(Params.Host, Params.Port);
            TSocket s(addr);
            s.SetNoDelay(true);
            h->SearchSocket = s;
            return h.Release();
        }
        void DestroyThreadSpecificResource(void* tsr) override {
            delete ((TWorker*)tsr);
        }
    };
    struct TJob : IObjectInQueue {
        TParams* Params;
        TString Tmp;
        TString Req;
        TString Qtree;
        TBlob Report;
        TJobReport Account;
        TJob(TParams* params, TString tmp)
          : Params(params)
          , Tmp(tmp)
          , Report()
          , Account()
        {
        }
        void Clear() {
            Tmp.clear();
            Req.clear();
            Qtree.clear();
            Report = TBlob();
        }
        void Process(void* tsr) override {
            ((TWorker*)tsr)->Process(this);
        }
    };
    void TWorker::Process(TJob* job) {
        TReportGuard g(&job->Account);

        TCgiParameters cgip(job->Tmp.data());
        job->Req = cgip.Get("user_request");
        job->Qtree = cgip.Get("qtree");

        {
            TSocketOutput so(SearchSocket);
            THttpOutput o(&so);
            o.EnableCompression(true);
            o.EnableKeepAlive(true);
            TString r;
            r += "GET /yandsearch?" + job->Tmp + " HTTP/1.1\r\n";
            r += "Host: " + job->Params->Host + ":" + ToString(job->Params->Port) + "\r\n\r\n";
            o.Write(r.data(), r.size());
            o.Finish();
        }
        {
            TSocketInput si(SearchSocket);
            THttpInput i(&si);
            job->Report = TBlob::FromString(i.ReadAll());
        }
    }
    struct TMain {
        TParams Params;
        void DoRun(IInputStream* in) {
            TString tmp;
            TThreadDataManager thrData(Params);
            TMtpQueueTsr qImpl(&thrData);
            TJobQueue q(&qImpl);
            q.Start(Params.Threads, Params.MaxQueue);
            TAtomicSharedPtr<IObjectInQueue> oldJob;

            while (in->ReadLine(tmp)) {
                TAtomicSharedPtr<TJob> newJob(new TJob(&Params, tmp));
                while (!q.Add(newJob, &newJob->Account)) {
                    oldJob = q.CompleteFront();
                    ProcessReport(*(TJob*)oldJob.Get());
                    ((TJob*)oldJob.Get())->Clear();
                }
            }
            for (; !!(oldJob = q.CompleteFront()).Get(); ) {
                ProcessReport(*(TJob*)oldJob.Get());
                ((TJob*)oldJob.Get())->Clear();
            }

            q.Stop();
        }
        void ProcessReport(const TJob& job) {
            const TBlob& report = job.Report;
            Cout << job.Req << '\t' << job.Qtree << '\t' << Base64Encode(TStringBuf((const char*)report.Data(), report.Size())) << Endl;
            return;
        }

        void PrintUsage() {
            Cout << "metasnip -h basehost -p baseport [-i input] [-j nthreads] [-q maxqueue]" << Endl;
            Cout << "  specify maxqueue=0 for unlimited input (pre)load" << Endl;
        }

        int Run(int argc, char** argv) {
            Opt opt(argc, argv, "h:p:i:j:q:");
            int optlet;
            while ((optlet = opt.Get()) != EOF) {
                switch (optlet) {
                    case 'h':
                        Params.Host = opt.Arg;
                        break;
                    case 'p':
                        Params.Port = FromString<ui16>(opt.Arg);
                        break;
                    case 'i':
                        Params.InputFName = opt.Arg;
                        break;
                    case 'j':
                        Params.Threads = FromString<ui16>(opt.Arg);
                        break;
                    case 'q':
                        Params.MaxQueue = FromString<ui16>(opt.Arg);
                        break;

                    case '?':
                    default:
                        PrintUsage();
                        return 0;
                        break;

                }
            }
            if (Params.MaxQueue && Params.MaxQueue < Params.Threads) {
                Params.MaxQueue = Params.Threads;
            }

            if (Params.Host.empty() || Params.Port == 0) {
                PrintUsage();
                return 1;
            }

            THolder<TFileInput> inf;
            IInputStream* qIn = &Cin;
            if (!Params.InputFName.empty()) {
                inf.Reset(new TFileInput(Params.InputFName));
                qIn = inf.Get();
            }
            DoRun(qIn);
            return 0;
        }
    };

}

int main(int argc, char** argv) {
    //--argc, ++argv;
    NSnippets::TMain main;
    return main.Run(argc, argv);
}
