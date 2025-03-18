#include <library/cpp/dolbilo/plan.h>
#include <library/cpp/dolbilo/stat.h>
#include <library/cpp/dolbilo/execmode.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/neh/factory.h>
#include <library/cpp/uri/http_url.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/http/misc/parsed_request.h>

#include <util/stream/buffered.h>
#include <util/stream/file.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/event.h>
#include <util/thread/lfqueue.h>
#include <util/thread/factory.h>

using namespace NNeh;
using namespace NLastGetopt;

namespace {
    class TSemaphore {
    public:
        inline TSemaphore(size_t val)
            : Val_(val)
            , E_(TSystemEvent::rAuto)
        {
        }

        inline void Acquire() noexcept {
            while (AtomicDecrement(Val_) < 0) {
                AtomicIncrement(Val_);
                E_.Wait();
            }
        }

        inline void Release() noexcept {
            AtomicIncrement(Val_);
            E_.Signal();
        }

    private:
        TAtomic Val_;
        TSystemEvent E_;
    };

    struct TLockOps {
        static inline void Acquire(TSemaphore* s) noexcept {
            if (s) {
                s->Acquire();
            }
        }

        static inline void Release(TSemaphore* s) noexcept {
            if (s) {
                s->Release();
            }
        }
    };

    typedef TGuard<TSemaphore, TLockOps> TSemaphoreGuard;

    template <class T>
    class TQueueType {
    public:
        typedef NNeh::TAutoLockFreeQueue<T> TImpl;
        typedef typename TImpl::TRef TRef;

        inline TQueueType()
            : E_(TSystemEvent::rAuto)
        {
        }

        inline void Stop() {
            Enqueue(nullptr);
        }

        inline void Enqueue(TRef t) {
            Q_.Enqueue(t);
            E_.Signal();
        }

        inline bool Dequeue(TRef& t) {
            while (!Q_.Dequeue(&t)) {
                E_.Wait();
            }

            return !!t;
        }

    private:
        TImpl Q_;
        TSystemEvent E_;
    };

    class TItemCb: public TAtomicCounter, public IThreadFactory::IThreadAble {
        struct TProcessor: public IOnRecv, public TSemaphoreGuard {
            inline TProcessor(TItemCb* parent)
                : TSemaphoreGuard(parent->InFly.Get())
                , Parent(parent)
            {
                TStats.Start = TInstant::Now();
                Parent->Inc();
            }

            inline ~TProcessor() override {
                Parent->Dec();
            }

            void OnRecv(THandle& hnd) override {
                THolder<TProcessor> self(this);

                {
                    TStats.Response = TInstant::Now();
                    Result = hnd.Get();
                    Parent->Schedule(self);
                }
            }

            TDevastateStatItem::TTimeStatItem TStats;
            TResponseRef Result;
            TItemCb* Parent;
        };

    public:
        inline TItemCb()
            : Protocol("full")
            , Port(0)
            , Time(TInstant::Now())
            , Out(&Cout)
            , Thr(SystemThreadFactory()->Run(this))
            , QLimit(Max<i64>())
            , Proto(nullptr)
            , NativeNeh(false)
            , TimeLimit(0)
        {
        }

        ~TItemCb() override {
        }

        inline EVerdict operator() (const TDevastateItem& item) {
            TMessage msg;

            if (NativeNeh) {
                PrepareNehMessage(msg, item);
            } else {
                msg.Addr = ConstructAddr(item);
                msg.Data = item.Data();
            }
            ProcessAdditionalData(msg);

            if (!InFly) {
                Time += item.ToWait();
                SleepUntil(Time);
            }

            THolder<TProcessor> proc(new TProcessor(this));
            TServiceStatRef noStat;
            try {
                Proto->ScheduleRequest(msg, proc.Get(), noStat);
                Y_UNUSED(proc.Release());
            } catch (...) {
                proc->TStats.Response = TInstant::Now();
                ScheduleSave(new TDevastateStatItem(TString(), proc->TStats, 0, 503, CurrentExceptionMessage()));
            }
            return V_CONTINUE;
        }

        static inline TString SynthRequestUrl(const TMessage& req) {
            TString url(req.Addr);
            url += '?';
            url += req.Data;
            return url;
        }

        inline void Schedule(TAutoPtr<TProcessor> result) {
            if (!result->Result) {
                ScheduleSave(new TDevastateStatItem(TString(), result->TStats, 0, 503));
            } else if (result->Result->IsError()) {
                ScheduleSave(new TDevastateStatItem(SynthRequestUrl(result->Result->Request), result->TStats, 0, 503, result->Result->GetErrorText()));
            } else {
                TDevastateStatItem::TData d;

                ScheduleSave(new TDevastateStatItem(SynthRequestUrl(result->Result->Request), result->TStats, 0, 0, 200, d, result->Result->Data.size()));
            }
        }

        inline void ScheduleSave(TAutoPtr<TDevastateStatItem> item) {
            SQ.Enqueue(item);
        }

        inline TString ConstructAddr(const TDevastateItem& item) {
            const ui16 port = Port ? Port : item.Port();
            const TString* host = &Host;

            if (host->empty()) {
                host = &item.Host();
            }

            TStringStream ss;

            ss << Protocol << "://"sv << *host << ":"sv << port;

            return ss.Str();
        }

        //in item.Data() we have HTTP GET request - parse it for using data in neh request
        inline void PrepareNehMessage(TMessage& msg, const TDevastateItem& item) {
            msg.Addr = ConstructAddr(item);

            TStringInput in(item.Data());
            THttpInput hin(&in);
            TParsedHttpRequest req(hin.FirstLine());

            if (req.Method != "GET") {
                throw yexception() << "now only GET requests can be converted to neh, but receive: " << req.Method;
            }

            THttpURL url;
            THttpURL::EParsed res = url.Parse(req.Request, THttpURL::FeaturesDefault, TStringBuf(), 1000000);

            if (res != THttpURL::ParsedOK) {
                throw yexception() << "can't parse url: error=" << res << ", url=" << req.Request;
            }

            if (!url.IsNull(THttpURL::FlagPath)) {
                msg.Addr += url.GetField(THttpURL::FieldPath);
            }
            if (!url.IsNull(THttpURL::FlagQuery)) {
                msg.Data = url.GetField(THttpURL::FieldQuery);
            }
        }

        inline void ProcessAdditionalData(TMessage& msg) {
            if (!AugmentUrl.empty()) {
                msg.Data += AugmentUrl;
            }
        }

        inline void Wait() {
            float tout = 0.01;

            while (Val()) {
                Sleep(TDuration::Seconds(tout));

                tout = Min(tout * 1.2f, 0.2f);
            }

            SQ.Stop();
            Thr->Join();
        }

        void DoExecute() override {
            TSQ::TRef t;
            ui64 cnt = 0;
            ui64 nxt = 100;

            while (SQ.Dequeue(t)) {
                ::Save(Out, *t);
                t.Destroy();
                ++cnt;

                if (cnt == nxt) {
                    Out->Flush();
                    nxt *= 2;
                }
            }

            Out->Flush();
        }

        inline void Run(const TString& fileName) {
            THolder<IInputStream> in;
            Proto = ProtocolFactory()->Protocol(Protocol);
            NativeNeh = Protocol != "full";
            TInstant dl = Now() + TDuration::Seconds(TimeLimit);
            bool hasDeadline = !!TimeLimit;
            Y_VERIFY(!Circular || hasDeadline);
            do {
                in.Reset(OpenInput(fileName).Release());
                ForEachPlanItem(in.Get(), *this, QLimit, TimeLimit);
            } while (Circular && (!hasDeadline || (Now() < dl)));
        }

        THolder<TSemaphore> InFly;
        TString Protocol;
        TString Host;
        ui16 Port;
        TInstant Time;
        IOutputStream* Out;
        typedef TQueueType<TDevastateStatItem> TSQ;
        TSQ SQ;
        THolder<IThreadFactory::IThread> Thr;
        i64 QLimit;
        IProtocol* Proto;
        bool NativeNeh;
        TString AugmentUrl;
        size_t TimeLimit;
        bool Circular = false;
    };
}

int main(int argc, char** argv) {
#if !defined(_MSC_VER)
    signal(SIGTTOU, SIG_IGN);
#endif

    THolder<IOutputStream> out;
    TItemCb cb;
    TString iname("-");

    {
        TOpts opts;
        TString oname = "-";
        EExecMode mode = EM_BY_PLAN;
        size_t sreqs = 0;

        opts.SetTitle("===========================================================\n"
                      "See https://wiki.yandex-team.ru/JandeksPoisk/Sepe/Dolbilka/\n"
                      "===========================================================\n");

        opts.AddHelpOption('?');

        opts.AddLongOption('P', "replace-port", "Replace port in all requests.")
            .RequiredArgument("PORT")
            .Optional()
            .StoreResult(&cb.Port);

        opts.AddLongOption('H', "replace-host", "Replace host in allrequests.")
            .RequiredArgument("HOST")
            .Optional()
            .StoreResult(&cb.Host);

        opts.AddLongOption('N', "neh-protocol", "Use neh protocol.")
            .RequiredArgument("PROTOCOL")
            .Optional()
            .StoreResult(&cb.Protocol);

        opts.AddLongOption('o', "output",
            "Write output to FILE instead of stdout.")
            .RequiredArgument("FILE")
            .Optional()
            .StoreResult(&oname);

        opts.AddLongOption('p', "plan-file",
            "Read plan from FILE instead of stdin.")
            .RequiredArgument("FILE")
            .Optional()
            .StoreResult(&iname);

        opts.AddLongOption('m', "mode",
            "Mode of operation. Can be one of " + GetEnumAllNames<EExecMode>())
            .RequiredArgument("MODE")
            .Optional()
            .StoreResult(&mode);

        opts.AddLongOption('s', "simultaneous", "Max simultaneous requests.")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&sreqs);

        opts.AddLongOption('Q', "queries-limit", "Limit request number.")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&cb.QLimit);

        opts.AddLongOption('T', "time-limit", "Limit execution time.")
            .RequiredArgument("SECONDS")
            .Optional()
            .StoreResult(&cb.TimeLimit);

        opts.AddLongOption('\0', "augmenturl", "Additional string to append to each request query.")
            .RequiredArgument("AUGMENTURL")
            .Optional()
            .StoreResult(&cb.AugmentUrl);

        opts.AddLongOption('t', "timeout", "Timeout for one shot (IGNORED).")
            .RequiredArgument("SECONDS")
            .Optional();

        opts.AddLongOption('c', "circular", "Repeat shots in circle.")
            .NoArgument().Optional()
            .StoreValue(&cb.Circular, true);

        TOptsParseResult(&opts, argc, argv);

        out.Reset(OpenOutput(oname).Release());
        cb.Out = out.Get();

        if (
            sreqs ||
            mode == EM_DEVASTATE ||
            mode == EM_FINGER
        ) {
            if (!sreqs) {
                sreqs = 10;
            }

            cb.InFly.Reset(new TSemaphore(sreqs));
        }
    }

    cb.Run(iname);
    cb.Wait();
}
