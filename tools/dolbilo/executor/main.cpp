#include "dns.h"
#include "echomode.h"
#include "slb.h"

#include <tools/dolbilo/libs/rps_schedule/rpsschedule.h>
#include <tools/dolbilo/libs/rps_schedule/rpslogger.h>

#include <library/cpp/coroutine/engine/condvar.h>
#include <library/cpp/coroutine/engine/events.h>
#include <library/cpp/coroutine/engine/impl.h>
#include <library/cpp/coroutine/engine/mutex.h>
#include <library/cpp/coroutine/engine/network.h>
#include <library/cpp/dolbilo/plan.h>
#include <library/cpp/dolbilo/stat.h>
#include <library/cpp/dolbilo/execmode.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/io/stream.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/deque.h>
#include <util/generic/intrlist.h>
#include <util/generic/refcount.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/thread/pool.h>
#include <util/system/guard.h>
#include <util/system/thread.h>

#include <cmath>

#define TaskRequestQueueShift 17

ui32 NextNumber(ui32 cur) {
    return 1664525 * cur + 1013904223;
}

struct TOpts {
    inline TOpts(int argc, char** argv) {
        NLastGetopt::TOpts opts;
        opts.AddHelpOption('?');
        TString modeStr;
        TString echoStr;

        opts.SetTitle("===========================================================\n"
                      "See https://wiki.yandex-team.ru/JandeksPoisk/Sepe/Dolbilka/\n"
                      "===========================================================\n");
        opts.AddLongOption("plans-help", "describe execution modes")
            .Optional()
            .NoArgument()
            .Handler0([]{
                Cerr << GetExecModeHelp();
                exit(0);
            });
        opts.AddLongOption('p', "plan-file", "Read plan from FILE instead of stdin.")
            .RequiredArgument("FILE")
            .Optional()
            .StoreResult(&FName);
        opts.AddLongOption('P', "replace-port", "Replace port in all requests.")
            .RequiredArgument("PORT")
            .Optional()
            .StoreResult(&ReplacePort);
        opts.AddLongOption('H', "replace-host", "Replace host in all requests.")
            .RequiredArgument("HOST")
            .Optional()
            .StoreResult(&ReplaceHost);
        opts.AddLongOption('o', "output",
            "Write output to FILE instead of stdout.")
            .RequiredArgument("FILE")
            .Optional()
            .StoreResult(&OName);
        opts.AddLongOption('B', "buffer", "Buffer size for output file")
            .RequiredArgument("BSIZE")
            .Optional()
            .StoreResult(&OBufferSize);
        opts.AddLongOption('n', "phantom", "Output results in phantom format (to be used with Lunapark).")
            .Optional().NoArgument()
            .StoreValue(&OutPhantom, true);
        opts.AddLongOption("reqid-as-tag", "Output reqid as tag in phantom format (default mode is enumeration)")
            .Optional().NoArgument()
            .StoreValue(&ReqIdAsAmmo, false);
        opts.AddLongOption("output-lenval32", "Output responses in lenval32 format")
            .Optional().NoArgument()
            .StoreValue(&OutLenVal32, true);
        opts.AddLongOption("output-base64-body", "Output responses base64 encoded body")
            .Optional().NoArgument()
            .StoreValue(&OutBase64Body, true);
        opts.AddLongOption('t', "timeout", "Timeout for one shot in milliseconds.")
            .RequiredArgument("TIME")
            .Optional()
            .StoreResult(&Timeout);
        opts.AddLongOption('s', "simultaneous", "Max simultaneous requests.")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&SimultaneousReqs);
        opts.AddLongOption('M', "multiplier", "Multiplier for delay between requests.")
            .RequiredArgument("FNUM")
            .Optional()
            .StoreResult(&Multiplier);
        opts.AddLongOption('a', "rps-min", "Minimum rps (for binary mode).")
            .RequiredArgument("FNUM")
            .Optional()
            .StoreResult(&MinRps);
        opts.AddLongOption('b', "rps-max", "Maximum rps (for binary mode).")
            .RequiredArgument("FNUM")
            .Optional()
            .StoreResult(&MaxRps);
        opts.AddLongOption('U', "min-success-rate", "Minimum success rate (for binary mode).")
            .RequiredArgument("FNUM")
            .Optional()
            .StoreResult(&MinSuccessRate);
        opts.AddLongOption('R', "rps-fixed", "Fixed rps (for plan mode).")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&FixedRps);
        opts.AddLongOption('\0', "rps-schedule", "Use yandex-tank rps schedule.")
            .RequiredArgument("SCHEDULE")
            .Optional()
            .StoreResult(&RpsSchedule);
        opts.AddLongOption('\0', "rps-log", "Dump to specified file rps scheduler information")
            .RequiredArgument("FNAME")
            .Optional()
            .StoreResult(&RpsLogName);
        opts.AddLongOption('\0', "battery", "Battery size (send only every NUM-th request in rps-schedule mode).")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&BatterySize);
        opts.AddLongOption('\0', "guncrew", "Guncrew serial within `battery` (start with NUM-th request).")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&GuncrewNum);
        opts.AddLongOption('w', "warm-time", "Warm time.")
            .RequiredArgument("SECONDS")
            .Optional()
            .StoreResult(&WarmTime);
        opts.AddLongOption('Q', "queries-limit", "Limit request number.")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&QueryLimit);
        opts.AddLongOption('T', "time-limit", "Limit overall time.")
            .RequiredArgument("SECONDS")
            .Optional()
            .StoreResult(&TimeLimit);
        opts.AddLongOption('\0', "deadman-url", "Poll URL to stop execution if something goes wrong.")
            .RequiredArgument("URL")
            .Optional()
            .StoreResult(&DeadmanUrl);
        opts.AddLongOption('\0', "deadman-interval", "Poll deadman URL every SECONDS.")
            .RequiredArgument("SECONDS")
            .Optional()
            .StoreResult(&DeadmanInterval);
        opts.AddLongOption('\0', "deadman-timeout", "Stop if deadman URL does not respond within SECONDS timeout.")
            .RequiredArgument("SECONDS")
            .Optional()
            .StoreResult(&DeadmanTimeout);
        opts.AddLongOption('\0', "augmenturl", "Additional string to append to each request URL.")
            .RequiredArgument("AUGMENTURL")
            .Optional()
            .StoreResult(&AugmentUrl);
        opts.AddLongOption('r', "replace-limit", "Number of bytes to be replaced.")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&ReplaceLimit);
        opts.AddLongOption('i', "seed", "Initial random seed.")
            .RequiredArgument("NUM")
            .Optional()
            .StoreResult(&Seed);
        opts.AddLongOption('m', "mode", "Execution mode: one of " + GetEnumAllNames<EExecMode>())
            .RequiredArgument("MODE")
            .Optional()
            .StoreResult(&modeStr)
            .DefaultValue("plan");
        opts.AddLongOption('S', "slb", "File with slb options.")
            .RequiredArgument("FILE")
            .Optional()
            .StoreResult(&SlbName);
        opts.AddLongOption('D', "dump-shots", "Dump shots.")
            .NoArgument().Optional()
            .StoreValue(&DumpShots, true);
        opts.AddLongOption('d', "dump-data", "Dump data.")
            .NoArgument().Optional()
            .StoreValue(&DumpData, true);
        opts.AddLongOption('F', "plan-memory", "Load full plan in memory.")
            .NoArgument().Optional()
            .StoreValue(&LoadPlanInMemory, true);
        opts.AddLongOption('k', "keep-alive", "Set only keep-alive header (legacy).")
            .NoArgument().Optional()
            .StoreValue(&KeepAlive, true);
        opts.AddLongOption('K', "real-keep-alive", "Enable keep-alive (works only for single target host).")
            .NoArgument().Optional()
            .StoreValue(&RealKeepAlive, true);
        opts.AddLongOption('c', "circular", "Repeat shots in circle.")
            .NoArgument().Optional()
            .StoreValue(&Circular, true);
        opts.AddLongOption('E', "echo", "Dump all plan shots in one of formats: " + GetEnumAllNames<EEchoMode>())
            .OptionalArgument("FORMAT")
            .DefaultValue("null")
            .OptionalValue("dolbilo")
            .StoreResult(&echoStr);
        opts.AddLongOption('z', "compress", "Request compressed data from server.")
            .NoArgument().Optional()
            .StoreValue(&Compress, true);
        opts.AddLongOption('\0', "no-body-encoding", "Disable body encoding according to Content-Encoding and Transfer-Encoding headers.")
            .NoArgument().Optional()
            .StoreValue(&BodyEncoding, false);

        const NLastGetopt::TOptsParseResult optsres(&opts, argc, argv);

        try {
            Mode = FromString<EExecMode>(modeStr);
            Echo = FromString<EEchoMode>(echoStr);
            if (DeadmanUrl && !(TDuration::Zero() < DeadmanTimeout && DeadmanTimeout < DeadmanInterval)) {
                ythrow NLastGetopt::TUsageException() << "deadman-timeout must be less than deadman-interval, both should be positive";
            }
            if (!(/* 0 <= GuncrewNum && */ GuncrewNum < BatterySize)) {
                ythrow NLastGetopt::TUsageException() << "battery size must be positive, guncrew serial must be within [0; battery)";
            }
            if (SlbName && ReplaceHost) {
                ythrow NLastGetopt::TUsageException() << "--slb and --replace-host are mutually exclusive, both options rewrite traffic destination";
            }
            if (OutLenVal32 && !DumpData) {
                ythrow NLastGetopt::TUsageException() << "--output-lenval32 requires --dump-data option enabled";
            }
            if (OutBase64Body && !DumpData) {
                ythrow NLastGetopt::TUsageException() << "--output-base64-body requires --dump-data option enabled";
            }
        } catch (...) {
            optsres.HandleError();
        }
    }

    TString FName = "-";
    TString OName = "-";
    size_t OBufferSize = 1 << 20;
    bool OutPhantom = false;
    bool ReqIdAsAmmo = true;
    bool OutLenVal32 = false;
    bool OutBase64Body = false;
    TDuration Timeout = TDuration::Seconds(30);
    size_t SimultaneousReqs = 1000;
    bool DumpShots = false;
    bool DumpData = false;
    float Multiplier = 1.0f;
    float MinRps = 1.0f;
    float MaxRps = 1.0f;
    float MinSuccessRate = 0.99f;
    size_t FixedRps = 0;
    TRpsSchedule RpsSchedule;
    TString RpsLogName;
    unsigned int BatterySize = 1;
    unsigned int GuncrewNum = 0;
    EExecMode Mode = EM_BY_PLAN;
    bool KeepAlive = false;
    bool RealKeepAlive = false;
    bool LoadPlanInMemory = false;
    size_t WarmTime = 0;
    TString SlbName;
    ui16 ReplacePort = 0;
    TString ReplaceHost;
    i64 QueryLimit = Max<decltype(QueryLimit)>();
    size_t TimeLimit = 0;
    TString DeadmanUrl;
    TDuration DeadmanInterval;
    TDuration DeadmanTimeout;
    TString AugmentUrl;
    bool Circular = false;
    EEchoMode Echo = ECHO_NULL;
    ui32 ReplaceLimit = 0;
    ui32 Seed = 0;
    bool Compress = false;
    bool BodyEncoding = true;
};

class TMain;

struct TShot: public TSimpleRefCount<TShot>
            , public TDevastateItem
            , public TIntrusiveListItem<TShot> {
    public:
        inline TShot(const TDevastateItem& item, TMain* m);

        inline ui64 PlanIndex() const {
            return PlanIndex_;
        }

        inline const TNetworkAddress& Ip() const {
            if (!TranslatedHost_.Get() || !Ip_.Get()) {
                Resolve();
            }

            return *Ip_;
        }

        inline void OnBadHost() {
            if (TranslatedHost_.Get()) {
                TranslatedHost_->OnBadHost();
            }
        }

        inline TMain* Main() const noexcept {
            return Main_;
        }

        inline TString RealHost() const {
            if (TranslatedHost_.Get()) {
                return TranslatedHost_->Result();
            }

            return Host();
        }

        inline TString RealData() const {
            TString realData = Data();
            ui32 curSeed = Seed_;
            for (size_t i = 0; i < ReplaceLimit_; ++i) {
                ui32 pos = curSeed % Max(realData.size() - 4, size_t(1));
                curSeed = NextNumber(curSeed);

                char c = (char)curSeed;
                curSeed = NextNumber(curSeed);

                realData[pos] = c;
            }
            return realData;
        }

    private:
        inline void Resolve() const;

    private:
        mutable IDns::TAddrRef Ip_;
        mutable ISlb::TTranslatedHostRef TranslatedHost_;
        TMain* Main_;
        ui32 Seed_;
        ui32 ReplaceLimit_;
        ui64 PlanIndex_ = Max<ui64>();
};

typedef TIntrusivePtr<TShot> TShotRef;

class TContIOStream: public IInputStream, public IOutputStream {
    public:
        inline TContIOStream(SOCKET fd, TCont* cont, TInstant deadline, IOutputStream* saver)
            : Fd_(fd)
            , Cont_(cont)
            , DeadLine_(deadline)
            , Saver_(saver)
        {
        }

        void DoWrite(const void* buf, size_t len) override {
            NCoro::WriteD(Cont_, Fd_, buf, len, DeadLine_).Checked();
        }

        size_t DoRead(void* buf, size_t len) override {
            const size_t ret = NCoro::ReadD(Cont_, Fd_, buf, len, DeadLine_).Checked();

            if (Saver_) {
                Saver_->Write(buf, ret);
            }

            return ret;
        }

    private:
        SOCKET Fd_;
        TCont* Cont_;
        const TInstant DeadLine_;
        IOutputStream* Saver_;
};

class TLengthDumper: public IOutputStream {
    public:
        inline TLengthDumper(size_t& len)
            : Len_(len)
        {
        }

        void DoWrite(const void* /*buf*/, size_t len) override {
            Len_ += len;
        }

    private:
        size_t& Len_;
};

class TDataDumper: public IOutputStream {
    public:
        inline TDataDumper(TDevastateStatItem::TData& data, size_t& len)
            : Data_(data)
            , Len_(len)
        {
        }

        void DoWrite(const void* buf, size_t len) override {
            Data_.insert(Data_.end(), (unsigned char*)buf, (unsigned char*)buf + len);
            Len_ += len;
        }

    private:
        TDevastateStatItem::TData& Data_;
        size_t& Len_;
};

class IShotIterator: public TSimpleRefCount<IShotIterator> {
    public:
        inline IShotIterator() noexcept {
        }

        virtual ~IShotIterator() {
        }

        virtual TShotRef Next() = 0;
};

typedef TIntrusivePtr<IShotIterator> IShotIteratorRef;

class TSimpleShotIterator: public IShotIterator {
    public:
        inline TSimpleShotIterator(TMain* m, TAutoPtr<IInputStream> f, size_t timeLimit)
            : Main_(m)
            , HasDeadline_(!!timeLimit)
            , Deadline_(Now() + TDuration::Seconds(timeLimit))
        {
            Cerr << "Loading plan..." << Endl;

            ForEachPlanItem(f.Get(), *this);

            Cerr << "Done loading plan (loaded " << Shots_.Size() << " items)" << Endl;
        }

        ~TSimpleShotIterator() override {
            while (!Shots_.Empty()) {
                delete Shots_.PopFront();
            }
        }

        TShotRef Next() override {
            if (Shots_.Empty() || HasDeadline_ && (Now() >= Deadline_)) {
                return nullptr;
            }

            return Shots_.PopFront();
        }

        inline EVerdict operator() (const TDevastateItem& item) {
            try {
                THolder<TShot> shot(new TShot(item, Main_));

                // prefetch DNS
                shot->Ip();
                AddShot(shot.Release());
            } catch (const std::exception& e) {
                Cerr << e.what() << Endl;
            }
            return V_CONTINUE;
        }

    private:
        inline void AddShot(TShotRef shot) {
            Shots_.PushBack(shot.Release());
        }

    private:
        TMain* Main_ = nullptr;
        TIntrusiveList<TShot> Shots_;
        const bool HasDeadline_;
        const TInstant Deadline_;
};

struct TCircularShotIterator: public IShotIterator {
    inline TCircularShotIterator(IShotIteratorRef slave, size_t timeLimit)
            : Slave_(slave)
            , HasDeadline_(!!timeLimit)
            , Deadline_(Now() + TDuration::Seconds(timeLimit))
        {
        }

        ~TCircularShotIterator() override {
        }

        TShotRef Next() override {
            TShotRef ret;
            if (!HasDeadline_ || (Now() < Deadline_)) {
                ret = DoNext();
                if (!!ret) {
                    Past_.push_back(ret);
                }
            }
            return ret;
        }

    private:
        inline TShotRef DoNext() {
            TShotRef ret = Slave_->Next();

            if (!ret && !Past_.empty()) {
                ret = Past_.front();
                Past_.pop_front();
            }

            return ret;
        }

    private:
        IShotIteratorRef Slave_;
        TDeque<TShotRef> Past_;
        const bool HasDeadline_;
        const TInstant Deadline_;
};

class TSmartShotIterator: public IShotIterator {
    public:
        inline TSmartShotIterator(TMain* m, TAutoPtr<IInputStream> f, TCont* cont, const ui64 limit, size_t timeLimit) noexcept
            : Main_(m)
            , F_(f)
            , Ex_(cont->Executor())
            , Name_(TString("shot_iterator(") + cont->Name() + ")")
            , Limit_(limit)
            , TimeLimit_(timeLimit)
        {
            Ex_->Create(*this, Name_.data());
        }

        ~TSmartShotIterator() override {
            // F_->Skip leads to `can not load pod array`, that's why escaper
            // flag is used to terminate plan-loader coroutine.
            Verdict_ = V_BREAK;

            try {
                while (Next().Get()) {}
            } catch (...) {
            }
        }

        TShotRef Next() override {
            TShotRef ret;

            Mutex_.LockI(C());

            while (!Eof_ && !Cur_) {
                Consumed_.Signal();
                Produced_.WaitI(C(), &Mutex_);
            }

            ret = Cur_;
            Cur_ = nullptr;

            Mutex_.UnLock();

            return ret;
        }

        inline void operator() (TCont* /*c*/) {
            try {
                ForEachPlanItem(F_.Get(), *this, Limit_, TimeLimit_);
            } catch (const std::exception& e) {
                Cerr << e.what() << Endl;
            }

            Yield(nullptr);
        }

        inline EVerdict operator() (const TDevastateItem& item) {
            try {
                TShotRef shot(new TShot(item, Main_));

                // prefetch DNS
                shot->Ip();
                Yield(shot);
            } catch (const std::exception& e) {
                Cerr << e.what() << Endl;
            }
            return Verdict_;
        }

    private:
        inline void Yield(TShotRef cur) {
            Mutex_.LockI(C());

            if (Cur_.Get()) {
                Consumed_.WaitI(C(), &Mutex_);
            }

            Cur_ = cur;
            Eof_ = !Cur_;

            Produced_.BroadCast();

            Mutex_.UnLock();
        }

        inline void WaitForNext() {
        }

        inline TCont* C() noexcept {
            return Ex_->Running();
        }

    private:
        TMain* Main_ = nullptr;
        THolder<IInputStream> F_;
        bool Eof_ = false;
        TShotRef Cur_;
        TContExecutor* Ex_ = nullptr;
        TContMutex Mutex_;
        TContCondVar Consumed_;
        TContCondVar Produced_;
        EVerdict Verdict_ = V_CONTINUE;
        TString Name_;
        const i64 Limit_;
        const size_t TimeLimit_;
};

struct TAugmentedShotIterator: public IShotIterator {
        inline TAugmentedShotIterator(IShotIteratorRef slave, const TString& augmentUrl, ui32 seed)
            : Slave_(slave)
            , AugmentUrl_(augmentUrl)
            , Random_(seed)
        {
        }

        ~TAugmentedShotIterator() override {
        }

        TShotRef Next() override {
            const TString randui32("{randui32}");
            TShotRef ret = Slave_->Next();
            if (!ret)
                return ret;

            TMemoryInput mi(ret->Data().data(), ret->Data().size());
            const TString req = mi.ReadLine();
            // faster than Split
            TStringBuf reqBuf(req);
            TStringBuf first = reqBuf.NextTok(' ');
            TStringBuf second = reqBuf.NextTok(' ');
            TStringBuf third = reqBuf.NextTok(' ');
            if (reqBuf.size() > 0)
                ythrow yexception() << "incorrect request(" << req.Quote() << ")";
            TString result;
            TStringOutput out(result);
            out << first << ' ' << second;
            TStringBuf augment(AugmentUrl_);
            size_t pos = 0;
            while ((pos = augment.find(randui32)) != TStringBuf::npos) {
                out << augment.Head(pos) << NextNumber();
                augment.Skip(pos + randui32.size());
            }
            out << augment << ' ' << third << "\r\n" << mi.ReadAll();
            return new TShot(
                TDevastateItem(ret->ToWait(), ret->Host(), ret->Port(), result, ret->PlanIndex()), ret->Main());
        }

    private:
        ui32 NextNumber() {
            Random_ = ::NextNumber(Random_);
            return Random_;
        }

        IShotIteratorRef Slave_;
        const TString& AugmentUrl_;
        ui32 Random_ = 0;
};

class TIteratorMultiplier {
        class TIterator: public TIntrusiveListItem<TIterator>, public IShotIterator {
                friend class TIteratorMultiplier;
            public:
                inline TIterator(TIteratorMultiplier* parent, ui32 pos)
                    : Parent_(parent)
                    , Pos_(pos)
                {
                    Parent_->Peers_.PushBack(this);
                }

                ~TIterator() override {
                }

                TShotRef Next() override {
                    if (!Parent_ || (Parent_->Drained_ && Size_ >= Parent_->Shots_.size())) {
                        return nullptr;
                    }
                    while (!Parent_->Drained_ && Pos_ >= Parent_->Shots_.size()) {
                        Parent_->Next();
                    }
                    if (!Parent_->Shots_.size()) {
                        return nullptr;
                    }
                    Pos_ %= Parent_->Shots_.size();
                    TShotRef ret = Parent_->Shots_[Pos_];
                    ++Pos_;
                    ++Size_;
                    return ret;
                }

            private:
                TIteratorMultiplier* Parent_;
                ui32 Pos_ = 0;
                ui32 Size_ = 0;
        };

    public:
        inline TIteratorMultiplier(IShotIteratorRef slave)
            : Slave_(slave)
            , Shift_(TaskRequestQueueShift)
            , Drained_(false)
        {
        }

        inline ~TIteratorMultiplier() {
            while (!Peers_.Empty()) {
                Peers_.PopBack()->Parent_ = nullptr;
            }
        }

        inline IShotIteratorRef Create() {
            return new TIterator(this, Peers_.Size() * Shift_);
        }

    private:
        void Next() {
            TShotRef next = Slave_->Next();
            if (!!next) {
                Shots_.push_back(next);
            } else {
                Drained_ = true;
            }
        }

        IShotIteratorRef Slave_;
        typedef TIntrusiveList<TIterator> TPeers;
        TPeers Peers_;
        TVector<TShotRef> Shots_;
        ui32 Shift_;
        bool Drained_;
};

class TMain {
        class TStatJob: public IObjectInQueue {
            public:
                inline TStatJob(TAutoPtr<TDevastateStatItem> item, TMain* This)
                    : Item_(item)
                    , This_(This)
                {
                }

                void Process(void* /*ThreadSpecificResource*/) override {
                    THolder<TStatJob> This(this);

                    This_->ProcessStatItemImpl(Item_.Get());
                }

            private:
                THolder<TDevastateStatItem> Item_;
                TMain* This_;
        };

    public:
        inline TMain(const TOpts* opts)
            : Opts_(opts)
            , Out_(OpenOutput(Opts_->OName, ECompression::DEFAULT, Opts_->OBufferSize))
            , Resolver_(&SyncDns_)
            , Shots_(nullptr)
            , Cnt_(0)
            , Slb_(new TFakeSlb())
            , Limit_(opts->QueryLimit)
            , Multiplier_(opts->Multiplier)
            , TotalQueries_(0)
            , GoodQueries_(0)
            , ReplaceLimit_(opts->ReplaceLimit)
            , Seed_(opts->Seed)
        {
            if (Opts_->SlbName) {
                Slb_.Reset(new TSimpleSlb(OpenInput(Opts_->SlbName).Get()));
            } else if (!Opts_->ReplaceHost.empty()) {
                Slb_.Reset(new TReplaceSlb(Opts_->ReplaceHost));
            }

            if (Opts_->RpsLogName) {
                RpsLogger_ = MakeHolder<TRpsLogger>(Opts_->RpsLogName);
            }
        }

        inline ~TMain() {
        }

        inline void operator() (TCont* c) {
            Cerr << "Process shots..." << Endl;

            if (Opts_->DumpShots) {
                Shots_ = new TSmartShotIterator(this, OpenInput(Opts_->FName), c, Limit_, 0);
                if (!Opts_->AugmentUrl.empty())
                    Shots_ = new TAugmentedShotIterator(Shots_, Opts_->AugmentUrl, Opts_->Seed);

                TShotRef shot;
                while ((shot = Shots_->Next()).Get() && Limit_-- > 0) {
                    Cout << shot->RealData() << Endl;
                }

                return;
            }

            if (Opts_->LoadPlanInMemory) {
                Shots_ = new TSimpleShotIterator(this, OpenInput(Opts_->FName), Opts_->TimeLimit);
            } else {
                Shots_ = new TSmartShotIterator(this, OpenInput(Opts_->FName), c, Limit_, Opts_->TimeLimit);
            }

            if (Opts_->Circular || (Opts_->Mode == EM_BY_BINARY_SEARCH)) {
                Shots_ = new TCircularShotIterator(Shots_, Opts_->TimeLimit);
            }
            if (!Opts_->AugmentUrl.empty()) {
                Shots_ = new TAugmentedShotIterator(Shots_, Opts_->AugmentUrl, Opts_->Seed);
            }

            try {
                TAsyncDns dns(c->Executor());
                TCachedDns::TSlaveHolder holder(&Resolver_, &dns);

                switch (Opts_->Mode) {
                    case EM_BY_PLAN:
                        MainByPlan(c, Opts_->FixedRps ? TDuration::Seconds(1) / Opts_->FixedRps : TDuration::Zero());
                        break;

                    case EM_DEVASTATE:
                    case EM_FINGER:
                        MainDevastate(c);
                        break;

                    case EM_BY_BINARY_SEARCH:
                        MainByBinarySearch(c);
                        break;

                    default:
                        Cerr << "Incorrect mode" << Endl;
                        abort();
                }
            } catch (const std::exception& e) {
                Cerr << e.what() << Endl;
            }

            /*
             * destroy shots
             */
            MultiIterator_.Destroy();
            Shots_ = nullptr;

            Cerr << "done" << Endl;
        }

        inline IDns* Resolver() noexcept {
            return &Resolver_;
        }

        inline ui16 ReplacePort() noexcept {
            return Opts_->ReplacePort;
        }

        inline ISlb* Slb() noexcept {
            return Slb_.Get();
        }

        ui32 GetSeed() noexcept {
            return Seed_;
        }

        void SetSeed(ui32 seed) noexcept {
            Seed_ = seed;
        }

        ui32 ReplaceLimit() noexcept {
            return ReplaceLimit_;
        }

    private:
        static inline double WarmMultiplier(TInstant start, TDuration delta, TInstant now) {
            if (now >= start + delta) {
                return 1.0;
            }

            const TDuration off = now - start;
            const double koeff = (double)off.MicroSeconds() / (double)delta.MicroSeconds();

            assert(koeff <= 1.0);

            return 1.0 + log(1.0 / Max<double>(koeff, 1.0 / delta.MicroSeconds()));
        }

        inline void MainByBinarySearch(TCont* c) {
            const double minSuccessRate = Opts_->MinSuccessRate;
            double maxRps = Opts_->MaxRps;
            double minRps = Opts_->MinRps;
            double curRps = 1.;
            i64 baseLimit = Limit_;

            Y_ASSERT(minRps <= maxRps);

            for (size_t i = 0; i < 10; ++i) {
                curRps = (maxRps + minRps) / 2;
                for (size_t j = 0; j < 3; ++j) {
                    TotalQueries_ = 0;
                    GoodQueries_ = 0;
                    Limit_ = baseLimit * curRps * 2 / (Opts_->MinRps + Opts_->MaxRps);

                    MainByPlan(c, TDuration::Seconds(1) / curRps);

                    if (GoodQueries_ / float(TotalQueries_) > minSuccessRate) {
                        minRps = curRps;
                        break;
                    }
                }
                if (minRps != curRps) {
                    maxRps = curRps;
                }

            }

            // last run with calculated rps value
            Out_.Reset(nullptr);
            Out_.Reset(OpenOutput(Opts_->OName, ECompression::DEFAULT, Opts_->OBufferSize).Release());

            Limit_ = baseLimit * curRps * 2 / (Opts_->MinRps + Opts_->MaxRps);
            MainByPlan(c, TDuration::Seconds(1) / curRps);
        }

        inline void MainByPlan(TCont* c, TDuration nextDelay = TDuration::Zero()) {
            Queue_.Start(1);

            const TInstant start = Now();
            const TDuration warm = TDuration::Seconds(Opts_->WarmTime);

            TRpsScheduleIterator schedIt(Opts_->RpsSchedule, start);

            TInstant last = start;
            if (Opts_->RpsSchedule) {
                if (Opts_->RpsSchedule.begin()->Mode == SM_AT_UNIX) {
                    // synchronize start
                    last = schedIt.NextShot(last);
                }

                // synchronize delay
                last = schedIt.NextShot(last, Opts_->GuncrewNum);
                c->SleepD(last);
            }

            TShotRef shot;

            while ((shot = Shots_->Next()).Get() && Limit_-- > 0) {
                TInstant next;

                if (nextDelay.GetValue()) { // rps-fixed
                    next = last + nextDelay;
                } else if (Opts_->RpsSchedule) {
                    next = schedIt.NextShot(last, Opts_->BatterySize);
                    if (RpsLogger_) {
                        RpsLogger_->DumpStats(schedIt);
                    }
                } else {
                    const double wmult = WarmMultiplier(start, warm, last);
                    next = last + shot->ToWait() * Multiplier_ * wmult;
                }

                c->Executor()->Create(CoSlave, shot.Get(), shot->Host().data());
                if (next != TInstant::Zero()) {
                    c->SleepD(next);
                    last = next;
                }
                else {
                    c->Yield(); // let `Slave' be scheduled to increment `Cnt_'
                    break;
                }
            }

            if (Opts_->RpsSchedule && Opts_->BatterySize > 1) {
                // synchronize finish
                c->SleepD(schedIt.GetFinish());
            }

            while (Cnt_.Val()) {
                c->SleepT(TDuration::Seconds(1));
            }

            Queue_.Stop();
        }

        inline void MainDevastate(TCont* c) {
            Queue_.Start(1);

            {
                const size_t count = Opts_->SimultaneousReqs;

                Cerr << "Spawn " << count << " coroutines..." << Endl;

                for (volatile size_t i = 0; i < count; ++i) {
                    c->Executor()->Create(CoDevastateSlave, this, "devastate_slave");
                }

                Cerr << "done spawn" << Endl;
            }

            c->Yield();

            while (Cnt_.Val()) {
                c->SleepT(TDuration::Seconds(1));
            }

            Queue_.Stop();
        }

        inline void SlaveLoop(TCont* c) {
            TShotRef shot;
            IShotIteratorRef shots = CreateIterator();
            auto guard = Guard(Cnt_);
            const size_t cnt = Cnt_.Val();

            c->Yield();

            Cerr << "Executor " << cnt << " up and running" << Endl;

            while ((shot = shots->Next()).Get() && Limit_-- > 0) {
                try {
                    Slave(c, shot.Get());
                } catch (const std::exception& e) {
                    Cerr << e.what() << Endl;
                }

                c->Yield();
            }

            Cerr << "Executor " << cnt << " finished" << Endl;

        }

        inline void SendRequest(IOutputStream* slave, TShot* shot, TDevastateStatItem::TTimeStatItem& tstats) {
            THttpOutput out(slave);

            out.EnableKeepAlive(Opts_->KeepAlive || Opts_->RealKeepAlive);
            out.EnableCompression(Opts_->Compress);
            out.EnableBodyEncoding(Opts_->BodyEncoding);

            TString realData = shot->RealData();
            out.Write(realData.data(), realData.size());
            out.Finish();
            tstats.Send = Now();
        }

        TString ReadResponse(IInputStream* slave, TDevastateStatItem::TTimeStatItem& tstats) {
            TTempBuf tmp;
            ui64 totalRead = 0;
            size_t chunkSize = 0;
            char* buf = tmp.Data();
            size_t sz = tmp.Size();
            THttpInput in(slave);

            while ((chunkSize = in.Read(buf, sz)) != 0) {
                if (!totalRead) {
                    tstats.Response = Now();
                }
            }
            return in.FirstLine();
        }

        inline void Slave(TCont* c, TShot* shot) {
            TDevastateStatItem::TTimeStatItem tstats;
            tstats.Start = Now();

            TShotRef This(shot);
            size_t datalen = 0;
            auto guard = Guard(Cnt_);

            try {

                const TInstant deadline = tstats.Start + Opts_->Timeout;

                TSocketHolder s;

                if (Opts_->RealKeepAlive && !SocketPool_.empty()) {
                    s.Swap(SocketPool_.back());
                    SocketPool_.pop_back();
                } else {
                    const int ret = NCoro::ConnectD(c, s, shot->Ip(), deadline);

                    if (ret) {
                        shot->OnBadHost();

                        ythrow TSystemError(ret) << "Cannot connect (" << shot->RealHost() << ", "
                            << shot->Ip() << ")";
                    }

                    SetZeroLinger(s);
                    SetNoDelay(s, true);
                }

                TDevastateStatItem::TData data;
                datalen = 0;
                tstats.Handshake = Now();

                THolder<IOutputStream> slave(Opts_->DumpData
                    ? (IOutputStream*)new TDataDumper(data, datalen)
                    : (IOutputStream*)new TLengthDumper(datalen));

                TContIOStream stream(s, c, deadline, slave.Get());

                SendRequest(&stream, shot, tstats);

                TString httpret = ReadResponse(&stream, tstats);

                THolder<TDevastateStatItem> item(new TDevastateStatItem(shot->GenerateUrl(),
                        tstats, shot->PlanIndex(), shot->RealData().length(), ParseHttpRetCode(httpret), data, datalen));
                ProcessStatItem(item);

                if (Opts_->RealKeepAlive) {
                    SocketPool_.emplace_back(std::move(s));
                }
            } catch (int err) {
                THolder<TDevastateStatItem> item(new TDevastateStatItem(shot->GenerateUrl(), tstats, shot->PlanIndex(), err));
                ProcessStatItem(item);
            } catch (const TSystemError& e) {
                Cerr << TInstant::Now() << " " << e.what() << ", url " << shot->GenerateUrl() << Endl;

                THolder<TDevastateStatItem> item(new TDevastateStatItem(shot->GenerateUrl(),
                        tstats, shot->PlanIndex(), e.Status()));

                ProcessStatItem(item);
            } catch (...) {
                Cerr << TInstant::Now() << " " << CurrentExceptionMessage()
                     << ", readed " << datalen << " bytes"
                     << ", url " << shot->GenerateUrl()
                     << Endl;

                THolder<TDevastateStatItem> item(new TDevastateStatItem(shot->GenerateUrl(),
                        tstats, shot->PlanIndex(), TDevastateStatItem::PROTOCOL_ERROR));
                ProcessStatItem(item);
            }
        }

        inline void ProcessStatItem(TAutoPtr<TDevastateStatItem> item) {
            THolder<TStatJob> job(new TStatJob(item, this));

            if (!Queue_.Add(job.Get())) {
                abort();
            }

            Y_UNUSED(job.Release());
        }

        inline void ProcessStatItemImpl(TDevastateStatItem* item) {
            if (Opts_->OutPhantom) {
                item->DumpPhantom(Out_.Get(), Opts_->ReqIdAsAmmo);
            } else if (Opts_->OutLenVal32 || Opts_->OutBase64Body) {
                size_t planIndex = static_cast<size_t>(item->PlanIndex());
                if (planIndex >= ResponsesState_.size()) {
                    ResponsesState_.resize(planIndex + 1, false);
                    Responses_.resize(planIndex + 1);
                }

                // Sequential iteration over responses
                // Keep order according to d-executor plan
                ResponsesState_[planIndex] = true;
                Responses_[planIndex] = std::move(item->Data());
                while (ResponsesPosition_ < ResponsesState_.size() && ResponsesState_[ResponsesPosition_]) {
                    TVector<unsigned char> &data = Responses_[ResponsesPosition_];

                    if (Opts_->OutLenVal32) {
                        // Dumping current response in lenval32 format
                        size_t dataLength = data.size();
                        Y_ENSURE(dataLength < Max<ui32>(), "Too long response to dump in lenval32 format. ");
                        const char* lengthPtr = reinterpret_cast<const char*>(&dataLength);
                        Out_.Get()->Write(lengthPtr, 4);
                        const char* dataPtr = reinterpret_cast<const char*>(data.data());
                        Out_.Get()->Write(dataPtr, dataLength);
                    } else {
                        // Dumping current response base64 encoded body
                        TString stringData{reinterpret_cast<const char*>(data.data()), data.size()};
                        TStringInput stringInput{stringData};
                        THttpInput httpInput{&stringInput};
                        TString base64encodedBody = Base64Encode(httpInput.ReadAll()) + "\n";
                        Out_.Get()->Write(base64encodedBody.data(), base64encodedBody.size());
                    }

                    ++ResponsesPosition_;
                }
            } else {
                item->Save(Out_.Get());
            }

            if (Opts_->Mode == EM_BY_BINARY_SEARCH) {
                ++TotalQueries_;
                if (item->ErrCode() == 0 && item->HttpCode() == HTTP_OK) {
                    ++GoodQueries_;
                }
            }
        }

        static void CoSlave(TCont* c, void* arg) {
            TShotRef shot((TShot*)arg);

            shot->Main()->Slave(c, shot.Get());
        }

        static void CoDevastateSlave(TCont* c, void* arg) {
            TMain* self = (TMain*)arg;

            self->SlaveLoop(c);
        }

        inline IShotIteratorRef CreateIterator() {
            if (Opts_->Mode == EM_DEVASTATE) {
                if (!MultiIterator_) {
                    MultiIterator_.Reset(new TIteratorMultiplier(Shots_));
                }

                return MultiIterator_->Create();
            }

            return Shots_;
        }

    private:
        const TOpts* Opts_;
        TThreadPool Queue_;
        TVector<TSocketHolder> SocketPool_;
        THolder<IOutputStream> Out_;
        THolder<TRpsLogger> RpsLogger_;
        TSyncDns SyncDns_;
        TCachedDns Resolver_;
        IShotIteratorRef Shots_;
        TExplicitSimpleCounter Cnt_;
        THolder<TIteratorMultiplier> MultiIterator_;
        THolder<ISlb> Slb_;
        i64 Limit_;

        float Multiplier_;
        ui64 TotalQueries_;
        ui64 GoodQueries_;
        ui32 ReplaceLimit_;
        ui32 Seed_;

        TVector<bool> ResponsesState_;
        TVector<TVector<unsigned char>> Responses_;
        size_t ResponsesPosition_ = 0;
};

inline TShot::TShot(const TDevastateItem& item, TMain* m)
    : TDevastateItem(item)
    , Ip_(nullptr)
    , Main_(m)
    , Seed_(Main_->GetSeed())
    , ReplaceLimit_(Main_->ReplaceLimit())
    , PlanIndex_(item.PlanIndex())
{
    Main_->SetSeed(NextNumber(Main_->GetSeed()));
}

inline void TShot::Resolve() const {
    TranslatedHost_ = Main()->Slb()->Translate(Host());

    ui16 port = Main()->ReplacePort();
    if (!port) {
        port = Port();
    }

    Ip_ = Main()->Resolver()->Resolve(TranslatedHost_->Result(), port);
    if (!Ip_) {
        ythrow yexception() << "cannot resolve " <<  RealHost().data();
    }
}

#define CONT_BASE_STACK_SIZE (16 * 1024)

#if defined(_32_)
    #define CONT_STACK_SIZE (CONT_BASE_STACK_SIZE)
#else
    #define CONT_STACK_SIZE (CONT_BASE_STACK_SIZE * 2)
#endif

class TEchoPlanFunctor {
    public:
        inline EVerdict operator() (const TDevastateItem& item) {
            Cout << item.Host() << ":" << item.Port() << "/" << item.Data() << Endl;
            return V_CONTINUE;
        }
};

class TEchoPhantomAmmoFunctor {
    public:
        inline EVerdict operator() (const TDevastateItem& item) {
            Cout << item.Data().size() << Endl
                 << item.Data() << Endl;
            return V_CONTINUE;
        }
};

class TEchoPhantomStpdFunctor {
    public:
        inline EVerdict operator() (const TDevastateItem& item) {
            if (Timestamp_.MilliSeconds() * 1000 != Timestamp_.MicroSeconds()) {
                static bool log;
                if (!log) {
                    Cerr << "`phantom-stpd' truncates timestamps to MilliSeconds and `plan` has MilliSeconds precision" << Endl;
                    log = true; // A bit too lazy to move `SimpleCallOnce` to util
                }
            }
            Cout << item.Data().size() << " " << Timestamp_.MilliSeconds() << Endl
                 << item.Data() << Endl;
            Timestamp_ += item.ToWait(); // corresponds to d-executor reality
            return V_CONTINUE;
        }
    private:
        TDuration Timestamp_;
};

class TDeadmanBreak : public ISimpleThread {
    public:
        TDeadmanBreak(const TString& url, const TDuration& interval, const TDuration& timeout)
            : HostHeader_(GetHostAndPort(url))
            , Path_(GetPathAndQuery(url))
            , Interval_(interval)
            , Timeout_(timeout)
        {
            if (GetSchemePrefix(url) != "http://") {
                ythrow yexception() << "deadman-url supports only http:// URLs";
            }

            TString host(GetHost(url));
            ui16 port(80);

            size_t pos = HostHeader_.rfind(':');
            if (pos != TString::npos) {
                port = FromString<ui16>(HostHeader_.substr(pos + 1));
            }

            Addr_.Reset(new TNetworkAddress(host, port));
        }

        void* ThreadProc() noexcept override;
        void operator() (TCont* c);

    private:
        const TString HostHeader_;
        const TString Path_;
        THolder<TNetworkAddress> Addr_;
        const TDuration Interval_;
        const TDuration Timeout_;
};

void* TDeadmanBreak::ThreadProc() noexcept {
    Detach();
    // there is no alarm() safety net as alarm() can't be mixed with sleep and
    // I can't easily verify that TContExecutor does not use sleep() at all.
    try {
        THolder<TContExecutor> executor(new TContExecutor(CONT_STACK_SIZE));
        executor->Execute(*this);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
    raise(SIGALRM);
    abort();
    return nullptr;
}

void TDeadmanBreak::operator() (TCont* c) {
    try {
        TString req;
        req += "GET " + Path_ + " HTTP/1.1\r\n"
             "Host: " + HostHeader_ + "\r\n"
             "\r\n";

        TInstant last(TInstant::Now());
        TInstant deadline = last + Timeout_;
        TInstant next = last + Interval_;

        TSocketHolder s;
        const int ret = NCoro::ConnectD(c, s, *Addr_, deadline);
        if (ret) {
            ythrow TSystemError(ret) << "deadman-url connect(" << *Addr_ << ") error";
        }

        while (true) {
            TContIOStream stream(s, c, deadline, nullptr);
            { // send request
                THttpOutput out(&stream);
                out.EnableKeepAlive(true);
                out.Write(req.data(), req.size());
                out.Finish();
            }

            { // read response
                THttpInput in(&stream);
                TString firstline(in.FirstLine());
                if (ParseHttpRetCode(firstline) != 200) {
                    ythrow yexception() << "deadman-url HTTP error: " << firstline;
                }
                while (in.Skip(4096) != 0) {
                    ;
                }
            }

            NCoro::PollD(c, s, POLLIN, next);
            if (IsNotSocketClosedByOtherSide(s)) {
                c->SleepD(next);
            } else {
                ythrow yexception() << "deadman-url closed keep-alive connection";
            }

            last = next;
            deadline = last + Timeout_;
            next = last + Interval_;
        }
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
}

int main(int argc, char** argv) {
    try {
        THolder<TOpts> opts;

        opts.Reset(new TOpts(argc, argv));
        if (opts->Echo == ECHO_NULL) {
            THolder<TDeadmanBreak> deadmanbreak;
            if (opts->DeadmanUrl) {
                deadmanbreak.Reset(new TDeadmanBreak(opts->DeadmanUrl, opts->DeadmanInterval, opts->DeadmanTimeout));
            }
            THolder<TMain> f(new TMain(opts.Get()));

            if (deadmanbreak) {
                deadmanbreak->Start();
            }

            THolder<TContExecutor> executor(new TContExecutor(CONT_STACK_SIZE));

            executor->Execute(*f);
        } else if (opts->Echo == ECHO_DOLBILO) {
            TEchoPlanFunctor functor;
            ForEachPlanItem(OpenInput(opts->FName).Get(), functor);
        } else if (opts->Echo == ECHO_PHANTOM_AMMO) {
            TEchoPhantomAmmoFunctor functor;
            ForEachPlanItem(OpenInput(opts->FName).Get(), functor);
        } else if (opts->Echo == ECHO_PHANTOM_STPD) {
            TEchoPhantomStpdFunctor functor;
            ForEachPlanItem(OpenInput(opts->FName).Get(), functor);
        } else {
            Y_ASSERT(false);
        }

        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }

    return 1;
}
