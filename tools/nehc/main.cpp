#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/neh/https.h>
#include <library/cpp/neh/jobqueue.h>
#include <library/cpp/neh/multi.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/neh/rpc.h>

#include <util/generic/typetraits.h>
#include <util/stream/input.h>
#include <util/system/shellcommand.h>

#include <stdio.h>

//fixed version for Cin
class TStdIn2: public IInputStream {
public:
    inline TStdIn2() noexcept
        : F_(stdin)
    {
    }

    ~TStdIn2() override {
    }

private:
    size_t DoRead(void* buf, size_t len) override {
        if (feof(F_))
            return 0;

        const size_t ret = fread(buf, 1, len, F_);

        if (ret < len && ferror(F_)) {
            ythrow yexception() << "can not read from stream(" << LastSystemErrorText() << ")";
        }

        return ret;
    }

private:
    FILE* F_;
};

using namespace NNeh;

struct TCommonOpts {
    ui32 Verbose;
};

struct TServerOpts : public TCommonOpts {
    TServerOpts()
        : SrvType(SrvEcho)
        , DelayTime(0)
    {}
    //server type
    enum ESrvType {
        SrvEcho
        , SrvDirectEcho
        , SrvExec
        , SrvOneShot
        , SrvCat
    } SrvType;
    size_t DelayTime; //response delay (recv/send)
    TString ExecCommand;
};

class TSrv {
public:
    TSrv(TServerOpts& opts)
        : O_(opts)
    {
    }

    inline void ServeRequest(const IRequestRef& req) {
        static const int quant = 10;
        TInstant timeStart = TInstant::Now();
        TInstant timeout = timeStart + TDuration::MilliSeconds(O_.DelayTime);

        while (TInstant::Now() < timeout) {
            if (req->Canceled()) {
                if (O_.Verbose) {
                    Cout << "Request canceled after " << (TInstant::Now() - timeStart).MilliSeconds() << " milliseconds\n";
                    Cout.Flush();
                }
                return;
            }
            Sleep(TDuration::MilliSeconds(quant));
        }

        TStringBuf data = req->Data();

        if (O_.SrvType == TServerOpts::SrvEcho) {
            if (O_.Verbose) {
                Cout << "send request data in reply (echo)\n";
                Cout.Flush();
            }
            TData res(data.data(), data.data() + data.size());
            req->SendReply(res);
        } else if (O_.SrvType == TServerOpts::SrvExec) {
            if (!O_.ExecCommand) {
                //FOOLPROOF
                if (O_.Verbose) {
                    Cout << "send empty data in reply\n";
                    Cout.Flush();
                }
                TData res;
                req->SendReply(res);
            }
            TShellCommandOptions shellOpts;
            TString sd(data);
            TStringInput ss(sd);

            shellOpts.SetInputStream(&ss);

            TShellCommand shell("/bin/sh", shellOpts);

            shell << "-c";
            shell << O_.ExecCommand;
            shell.Run().Wait();

            const TString &s = shell.GetOutput();
            TData res(s.c_str(), s.c_str() + s.size());

            req->SendReply(res);
        } else if (O_.SrvType == TServerOpts::SrvOneShot) {
            Cout.Write(data.data(), data.size());
            Cout.Finish();
            close(1);
            TStdIn2 Cin2;
            TString input(Cin2.ReadAll());
            TData res(input.c_str(), input.c_str() + input.size());
            req->SendReply(res);
            Event1.Signal();
        } else if (O_.SrvType == TServerOpts::SrvCat) {
            {
                TGuard<TMutex> guard(CatCoutMutex_);

                Cout.Write(data.data(), data.size());
            }
            Cout.Flush();
            TData res;
            req->SendReply(res);
        } else {
            Y_ASSERT(0);
        }
    }

    TSystemEvent Event1;
private:
    TServerOpts& O_;
    TMutex CatCoutMutex_;
};

struct TClientsOpts : public TCommonOpts {
    size_t Rep;
    size_t CancelTimeout;
    bool Silent;
};

int RunClient(const TMessage& msg, const TClientsOpts& opts) {
    NNeh::IMultiRequesterRef rr = CreateRequester();
    size_t rep = opts.Rep;
    while (rep--) {

        THandleRef hr = Request(msg);
        rr->Add(hr);

        //check request status
        THandleRef processed;
        bool sendingComplete = false;
        TInstant cancelRequestAfter = TInstant::Now() + TDuration::MilliSeconds(opts.CancelTimeout);
        while (true) {
            bool res = rr->Wait(processed, TDuration::MilliSeconds(5));

            if (!res && opts.Verbose >= 3) {
                Cout << "-";
                Cout.Flush();
            }

            if (!sendingComplete && hr->MessageSendedCompletely()) {
                sendingComplete = true;
                if (opts.Verbose >= 3) {
                    Cout << "[Complete sending message]";
                    Cout.Flush();
                }
            }

            if (TInstant::Now() > cancelRequestAfter) {
                hr->Cancel();
                if (opts.Verbose >= 3) {
                    Cout << "[Cancel request]\n";
                    Cout.Flush();
                }
                if (opts.Verbose) {
                    Cout << "Cancel request\n";
                }
                if (!rep) {
                    Sleep(TDuration::MilliSeconds(20));
                    return 4;
                }
                break;
            }
            if (processed.Get() == hr.Get()) {
                TResponseRef resp = processed->Get();
                if (opts.Verbose >= 3) {
                    Cout << Endl;
                }
                if (!resp) {
                    if (opts.Verbose)
                        Cout << "No response\n";
                    if (!rep)
                        return 2;
                } else {
                    if (resp->IsError()) {
                        if (opts.Verbose)
                            Cout << "Receive request error: " << resp->GetErrorText() << Endl;
                        if (!rep)
                            return 3;
                    } else {
                        if (opts.Verbose)
                            Cout << "Receive request response: ";
                        if (!opts.Silent)
                            Cout << resp->Data;
                        else if (opts.Verbose)
                            Cout << "<supressed>";
                        if (opts.Verbose)
                            Cout << Endl;
                    }
                }
                break;
            }
        }
    }
    return 0;
}

void WaitPressEnter(bool verbose) {
    if (verbose) {
        Cout << "Service started (for exit press Enter)\n";
        Cout.Flush();
    }
    try {
        TString line = Cin.ReadLine();
    } catch (...) {
    }
}

int main(int argc, char** argv) {
    using namespace NLastGetopt;

    TCommonOpts commonOpts;
    TClientsOpts clientOpts;
    size_t genData = 0;
    TServerOpts serverOpts;

    TOpts opts;
    //client options
    opts.AddCharOption('c', "cancel request timeout (millisec)").StoreResult(&clientOpts.CancelTimeout).RequiredArgument("TIMEOUT").DefaultValue("600000");
    opts.AddCharOption('r', "repeat request").StoreResult(&clientOpts.Rep).RequiredArgument("TIMES").DefaultValue("1");
    opts.AddCharOption('q', "suppress output response").NoArgument();
    opts.AddCharOption('g', "generate text blob as data: 'xxx...'").StoreResult(&genData).RequiredArgument("SIZE");
    //server options
    opts.AddCharOption('s', "single-shot server (print request data to stdout, get response data from stdin), use neh services addresses as app. arguments").NoArgument();
    opts.AddCharOption('e', "echo server (simple return data to client)").NoArgument();
    opts.AddCharOption('E', "direct echo server (not use separate thread for process request) (simple return data to client)").NoArgument();
    opts.AddCharOption('C', "cat server (print requests data to stdout, return empty response to client)").NoArgument();
    opts.AddCharOption('x', "execute server, executes specified command via /bin/sh (request data >>stdin, stdout>> response)").StoreResult(&serverOpts.ExecCommand).RequiredArgument("EXEC_CMD").DefaultValue("");
    opts.AddCharOption('i', "server response delay (recv/send, millisec)").StoreResult(&serverOpts.DelayTime).RequiredArgument("TIMEOUT").DefaultValue("0");
    //common options
    opts.AddCharOption('v', "verbose output [0..3]").StoreResult(&commonOpts.Verbose).RequiredArgument("LEVEL").DefaultValue("0");
    opts.AddHelpOption();

    opts.SetFreeArgsMin(1);

    opts.SetFreeArgTitle(0, "<addr(s)>", "Message address (or server address(es) for server)");
    opts.SetFreeArgTitle(1, "<msg-data>", "Message data (if used '-g', generated blob will be appended at tail), if leaved data and -g, read data from stdin");

    TAutoPtr<TOptsParseResult> resOptsRef;

    try {
        resOptsRef = new TOptsParseResult(&opts, argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        opts.PrintUsage(TStringBuf(argv[0]), Cerr);
        return 1;
    }

    TOptsParseResult& resOpts = *resOptsRef;
    TVector<TString> freeArgs = resOpts.GetFreeArgs();

    bool server = true;
    if (resOpts.Has('e')) {
        serverOpts.SrvType = TServerOpts::SrvEcho;
    } else if (resOpts.Has('E')) {
        serverOpts.SrvType = TServerOpts::SrvDirectEcho;
    } else if (resOpts.Has('x')) {
        serverOpts.SrvType = TServerOpts::SrvExec;
    } else if (resOpts.Has('s')) {
        serverOpts.SrvType = TServerOpts::SrvOneShot;
    } else if (resOpts.Has('C')) {
        serverOpts.SrvType = TServerOpts::SrvCat;
    } else {
        server = false;
    }
    if (server) {
        //server
        if (serverOpts.SrvType == TServerOpts::SrvDirectEcho) {
            //echo server without dedicated processing threads (send reply directly from callback)
            TListenAddrs addrs;
            for (size_t i = 0; i < freeArgs.size(); i++) {
                addrs.push_back(freeArgs[i]);
            }

            struct TOnRequest: public IOnRequest {
                void OnRequest(IRequestRef req) override {
                    TStringBuf data = req->Data();
                    TData res(data.data(), data.data() + data.size());
                    req->SendReply(res);
                }
            } callback;

            IRequesterRef srv = MultiRequester(addrs, &callback);
            WaitPressEnter(serverOpts.Verbose);
        } else {
            ((TCommonOpts&)serverOpts) = commonOpts;

            TSrv srv(serverOpts);

            IServicesRef svs = CreateLoop();
            for (size_t i = 0; i < freeArgs.size(); i++) {
                svs->Add(freeArgs[i], srv);
            }

            try {
                svs->ForkLoop(8);
            } catch (yexception& err) {
                Cerr << "can't run services: " << err.what() << '\n';
                return 2;
            }

            if (serverOpts.SrvType == TServerOpts::SrvOneShot) {
                srv.Event1.Wait();
                //give some time for flush last response data
                Sleep(TDuration::MilliSeconds(1000));
                return 0;
            }
            WaitPressEnter(serverOpts.Verbose);
        }
        return 0;
    } else {
        //client
        ((TCommonOpts&)clientOpts) = commonOpts;
        clientOpts.Silent = resOpts.Has('q');
        TMessage msg;
        if (freeArgs.empty()) {
            opts.PrintUsage(TStringBuf(argv[0]), Cerr);
            return 1;
        } if (freeArgs.size() == 1) {
            msg.Addr = freeArgs[0];
            if (genData) {
                while (genData--) {
                    msg.Data += "x";
                }
            } else {
                //read message data from console
                TStdIn2 Cin2;
                msg.Data = Cin2.ReadAll();
            }
        } else {
            msg.Addr = freeArgs[0];
            msg.Data = freeArgs[1];
            if (genData) {
                while (genData--) {
                    msg.Data += "x";
                }
            }
        }
        try {
            return RunClient(msg, clientOpts);
        } catch (...) {
            Cerr << CurrentExceptionMessage() << Endl;
            return 6;
        }
    }
    return 0;
}
