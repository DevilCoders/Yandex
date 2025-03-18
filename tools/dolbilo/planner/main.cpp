#include <library/cpp/dolbilo/planner/accesslog.h>
#include <library/cpp/dolbilo/planner/eventlog.h>
#include <library/cpp/dolbilo/planner/fullreqs.h>
#include <library/cpp/dolbilo/planner/phantom.h>
#include <library/cpp/dolbilo/planner/loader.h>
#include <library/cpp/dolbilo/planner/loadlog.h>
#include <library/cpp/dolbilo/planner/pcapdump.h>
#include <library/cpp/dolbilo/planner/plain.h>
#include <library/cpp/dolbilo/planner/planlog.h>

#include <library/cpp/coroutine/engine/events.h>
#include <library/cpp/coroutine/engine/impl.h>
#include <library/cpp/dolbilo/plan.h>
#include <library/cpp/getopt/ygetopt.h>

#include <library/cpp/containers/intrusive_avl_tree/avltree.h>
#include <util/generic/hash.h>
#include <util/generic/yexception.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/stream/output.h>
#include <util/stream/zlib.h>
#include <util/string/cast.h>

class TLoaderFactory {
        typedef TSimpleSharedPtr<IReqsLoader> TLoader;
        typedef THashMap<TString, TLoader> TLoaders;
    public:
        static inline TLoaderFactory& Instance() {
            static TLoaderFactory ret;

            return ret;
        }

        inline TSimpleSharedPtr<IReqsLoader> Find(const TString& name) {
            TLoaders::const_iterator it = Loaders_.find(name);

            if (it == Loaders_.end()) {
                ythrow yexception() << "unknown loader name (" << name.Quote() << ")";
            }

            return TSimpleSharedPtr<IReqsLoader>(it->second->Clone());
        }

        inline TString Names() const {
            TString ret;

            for (TLoaders::const_iterator it = Loaders_.begin(); it != Loaders_.end(); ++it) {
                if (ret.size()) {
                    ret += ", ";
                }

                ret += it->first;
            }

            return ret;
        }

    private:
        inline TLoaderFactory() {
            Add("plain", TLoader(new TPlainLoader));
            Add("loadlog", TLoader(new TLoadLogLoader));
            Add("planlog", TLoader(new TPlanLoader));
            Add("accesslog", TLoader(new TAccessLogLoader));
            Add("eventlog", TLoader(new TEventLogLoader));
            Add("pcap", TLoader(new TPcapDumpLoader));
            Add("fullreqs", TLoader(new TFullReqsLoader));
            Add("phantom", TLoader(new TPhantomLoader));
        }

        inline ~TLoaderFactory() {
        }

        inline void Add(const char* name, TLoader loader) {
            Loaders_[name] = loader;
        }

    private:
        TLoaders Loaders_;
};

static inline void Usage(const char* prog) {
    Cerr << "===========================================================\n"
            "See https://wiki.yandex-team.ru/JandeksPoisk/Sepe/Dolbilka/\n"
            "===========================================================\n" << Endl;
    Cerr << "Usage: " << prog << " [-l filename] [-o filename] [-t type ("
         << TLoaderFactory::Instance().Names() << ")] [-H] [mode-specific options]" << Endl
         << "   or: " << prog << " -t type -H for 'type' loader command line options" << Endl
         << "   -l filename with queries plan [input]" << Endl
         << "   -o filename for execution plan [output]" << Endl
         << "   -t loader type" << Endl
         << "   -D debug output instead of plan-file" << Endl
         << "   -P plan items in one portion" << Endl
         << "   -S split output by S slices" << Endl << Endl;

}

class TDevastateItemWithAbsTime {
    public:
        inline TDevastateItemWithAbsTime(const TDevastateItem& item, TInstant abstime)
            : Item_(item)
            , AbsTime_(abstime)
        {
        }

        inline ~TDevastateItemWithAbsTime() {
        }

        inline TDevastateItem& Item() noexcept {
            return Item_;
        }

        inline TInstant AbsTime() const noexcept {
            return AbsTime_;
        }

    private:
        TDevastateItem Item_;
        TInstant AbsTime_;
};

class TSimpleOutputter {
    protected:
        class TItem {
            public:
                inline TItem() noexcept {
                }

                virtual ~TItem() {
                }

                inline void Add(TDevastateItemWithAbsTime& item) {
                    DoAdd(item);
                }

                inline void Finish() {
                    DoFinish();
                    Out_->Finish();
                }

            private:
                virtual void DoAdd(TDevastateItemWithAbsTime& item) = 0;
                virtual void DoFinish() = 0;

            public:
                TSimpleSharedPtr<IOutputStream> Out_;
        };

        typedef TSimpleSharedPtr<TItem> TItemRef;
        typedef TVector<TItemRef> TItems;

    public:
        inline TSimpleOutputter() noexcept
            : N_(0)
        {
        }

        virtual ~TSimpleOutputter() {
        }

        inline void AddOutput(TAutoPtr<IOutputStream> out) {
            TItemRef item = CreateItem();

            item->Out_ = out;
            Items_.push_back(item);
        }

        inline void Add(TDevastateItemWithAbsTime& item) {
            assert(Items_.size());

            /*
             * round robin for items...
             */
            Items_[(N_++) % Items_.size()]->Add(item);
        }

        inline void Finish() {
            for (TItems::iterator it = Items_.begin(); it != Items_.end(); ++it) {
                (*it)->Finish();
            }
        }

        virtual TItemRef CreateItem() = 0;

    private:
        ui64 N_;
        TItems Items_;
};

class TOutputterAdaptor: public IReqsLoader::IOutputter {
    public:
        inline TOutputterAdaptor(TSimpleOutputter* slave) noexcept
            : Cur_(TInstant::MicroSeconds(0))
            , Slave_(slave)
        {
        }

        ~TOutputterAdaptor() override {
        }

        void Add(const TDevastateItem& it) override {
            Cur_ += it.ToWait(); // correctly account the very first item
            TDevastateItemWithAbsTime item(it, Cur_);
            Slave_->Add(item);
        }

    private:
        TInstant Cur_;
        TSimpleOutputter* Slave_;
};

class TOutputter: public TSimpleOutputter {
        class TMyItem: public TItem {
            public:
                inline TMyItem(size_t plen)
                    : PortionLength_(plen)
                    , Cur_(TInstant::MicroSeconds(0))
                {
                }

                ~TMyItem() override {
                }

            private:
                void DoAdd(TDevastateItemWithAbsTime& it) override {
                    TDevastateItem& item = it.Item();

                    if (it.AbsTime() < Cur_) {
                        abort();
                    }

                    item.ToWait(it.AbsTime() - Cur_);
                    Cur_ = it.AbsTime();

                    Plan_.Add(item);

                    if (Plan_.Size() >= PortionLength_) {
                        Flush();
                    }
                }

                void DoFinish() override {
                    Flush();
                }

                inline void Flush() {
                    Plan_.Save(Out_.Get());
                    Plan_.Clear();
                }

            private:
                TDevastatePlan Plan_;
                size_t PortionLength_;
                TInstant Cur_;
        };

    public:
        inline TOutputter(size_t plen)
            : PortionLength_(plen)
        {
        }

        ~TOutputter() override {
        }

        TItemRef CreateItem() override {
            return new TMyItem(PortionLength_);
        }

    private:
        size_t PortionLength_;
};

class TDumpOutputter: public TSimpleOutputter {
        class TMyItem: public TItem {
            public:
                inline TMyItem() noexcept
                    : Cur_(TInstant::MicroSeconds(0))
                {
                }

                ~TMyItem() override {
                }

            private:
                void DoAdd(TDevastateItemWithAbsTime& it) override {
                    TDevastateItem& item = it.Item();

                    if (it.AbsTime() < Cur_) {
                        abort();
                    }

                    item.ToWait(it.AbsTime() - Cur_);
                    Cur_ = it.AbsTime();

                    *Out_ << item.Port() << '\n'
                          << item.Host() << '\n'
                          << item.ToWait() << '\n'
                          << item.Data() << '\n';
                }

                void DoFinish() override {
                }

            private:
                TInstant Cur_;
        };

    public:
        TItemRef CreateItem() override {
            return new TMyItem();
        }
};

class TMultiLoader {
        class TSlave;

        struct TCompare {
            template <class T>
            static inline bool Compare(const T& l, const T& r) noexcept {
                return l < r;
            }
        };

        class TOneItem: public TAvlTreeItem<TOneItem, TCompare> {
            public:
                inline TOneItem(const TSlave* parent, const TDevastateItem& item, TCont* cont)
                    : Par_(parent)
                    , Item_(item)
                    , Event_(cont)
                {
                }

                inline bool operator< (const TOneItem& r) const noexcept {
                    return (AbsTime() < r.AbsTime()) || (AbsTime() == r.AbsTime() && this < &r);
                }

                inline TInstant AbsTime() const noexcept;

                inline const TDevastateItem& Item() const noexcept {
                    return Item_;
                }

                inline void Wake() noexcept {
                    Event_.Wake();
                }

                inline void Wait() noexcept {
                    Event_.WaitI();
                }

            private:
                const TSlave* Par_;
                const TDevastateItem& Item_;
                TContEvent Event_;
        };

        typedef TAvlTree<TOneItem, TCompare> TTree;

        class TSlave: public IReqsLoader::IOutputter {
            public:
                inline TSlave(TTree* tree)
                    : Tree_(tree)
                    , C_(nullptr)
                {
                }

                ~TSlave() override {
                }

                inline void operator() (TCont* c) {
                    C_ = c;

                    try {
                        IReqsLoader::TParams params(In_.Get(), this);

                        Loader()->Process(&params);
                    } catch (...) {
                        Cerr << CurrentExceptionMessage() << Endl;
                    }
                }

                inline IInputStream* File() const noexcept {
                    return In_.Get();
                }

                inline void SetFile(const TString& fname) {
                    FileName_ = fname;
                    In_ = OpenInput(FileName_);
                }

                inline IReqsLoader* Loader() const noexcept {
                    return Loader_.Get();
                }

                inline void SetLoader(TSimpleSharedPtr<IReqsLoader> loader) noexcept {
                    Loader_ = loader;
                }

                inline TInstant AbsTime() const noexcept {
                    return AbsTime_;
                }

                void Add(const TDevastateItem& item) override {
                    TOneItem one_item(this, item, C_);

                    Tree_->Insert(&one_item);
                    one_item.Wait();
                    AbsTime_ += item.ToWait();
                }

                inline const char* Name() const noexcept {
                    return FileName_.data();
                }

            private:
                TTree* Tree_;
                TSimpleSharedPtr<IReqsLoader> Loader_;
                TString FileName_;
                TSimpleSharedPtr<IInputStream> In_;
                TCont* C_;
                TInstant AbsTime_;
        };

        typedef TVector<TSlave> TSlaves;
    public:
        inline TMultiLoader(int argc, char const* const* argv)
            : Outputter_(nullptr)
            , OutFileName_("-")
            , PortionLength_(5000)
            , Slices_(1)
            , Dump_(false)
        {
            Add();
            TString opts = "P:l:o:t:HS:?D";

            {
                TGetOpt opt(argc, argv, opts);

                for (TGetOpt::TIterator it = opt.Begin(); it != opt.End(); ++it) {
                    if (it->Key() == 't') {
                        if (!it->HaveArg()) {
                            ythrow yexception() << "-t expect argument";
                        }

                        opts += TLoaderFactory::Instance().Find(it->Arg())->Opts();
                        break;
                    }
                }
            }

            {
                TGetOpt opt(argc, argv, opts);

                for (TGetOpt::TIterator it = opt.Begin(); it != opt.End(); ++it) {
                    switch (it->Key()) {
                        case 'l':
                            if (!it->HaveArg()) {
                                ythrow yexception() << "-l expect argument (input file name)";
                            }

                            OnFileName(it->Arg());
                            break;

                        case 'o':
                            if (!it->HaveArg()) {
                                ythrow yexception() << "-o expect argument (output file name)";
                            }

                            OutFileName_ = it->Arg();
                            break;

                        case 't':
                            if (!it->HaveArg()) {
                                ythrow yexception() << "-t expect argument (loader type)";
                            }

                            OnLoader(it->Arg());
                            break;

                        case 'D':
                            Dump_ = true;
                            break;

                        case 'P':
                            if (!it->HaveArg()) {
                                ythrow yexception() << "-P expect argument (plan items in one portion)";
                            }

                            PortionLength_ = FromString<size_t>(it->Arg());
                            break;

                        case '?':
                        case 'H':
                            ::Usage(argv[0]);
                            Cerr << "Mode-specific options:" << Endl;
                            Usage();
                            exit(0);

                            break;

                        case 'S':
                            Slices_ = FromString<size_t>(it->Arg());

                            if (Slices_ < 1) {
                                ythrow yexception() << "zero slices";
                            }

                            break;

                        default: {
                            IReqsLoader::TOption option = {it->Key(), it->Arg()};

                            if (!HandleOpt(&option)) {
                                ythrow yexception() << "unknown cmd option";
                            }
                        }
                    }
                }
            }
        }

        inline void Process() {
            CheckLast();

            THolder<TOutputter> outputter(Dump_
                ? (TOutputter*)new TDumpOutputter
                : (TOutputter*)new TOutputter(PortionLength_)
            );

            if (Slices_ < 2) {
                outputter->AddOutput(OpenOutput(OutFileName_));
            } else {
                for (size_t i = 0; i < Slices_; ++i) {
                    outputter->AddOutput(OpenOutput(OutFileName_ + "." + ToString(i)));
                }
            }

            Outputter_ = outputter.Get();

            if (Slaves_.size() == 1) {
                TOutputterAdaptor adaptor(Outputter_);
                IReqsLoader::TParams params(Last()->File(), &adaptor);

                Last()->Loader()->Process(&params);
            } else {
                TContExecutor executor(1000000);

                executor.Execute(*this);
            }

            outputter->Finish();
        }

        inline void operator() (TCont* c) {
            for (TSlaves::iterator it = Slaves_.begin(); it != Slaves_.end(); ++it) {
                c->Executor()->Create(*it, it->Name());
            }

            c->Yield();

            while (!Tree_.Empty()) {
                TTree::iterator it = Tree_.Begin();

                try {
                    TDevastateItemWithAbsTime item(it->Item(), it->AbsTime());

                    Outputter_->Add(item);
                } catch (const std::exception& e) {
                    Cerr << e.what() << Endl;
                }

                it->Unlink();
                it->Wake();
            }
        }

    private:
        inline void OnLoader(const TString& name) {
            if (Last()->Loader()) {
                CheckLast(); Add();
            }

            Last()->SetLoader(TLoaderFactory::Instance().Find(name));
        }

        inline void OnFileName(const TString& name) {
            if (Last()->File()) {
                CheckLast(); Add();
            }

            Last()->SetFile(name);
        }

        inline bool HandleOpt(const IReqsLoader::TOption* option) {
            if (Last()->Loader()) {
                return Last()->Loader()->HandleOpt(option);
            }

            return false;
        }

        inline void Usage() {
            CheckLast();

            for (TSlaves::iterator it = Slaves_.begin(); it != Slaves_.end(); ++it) {
                it->Loader()->Usage();
            }
        }

        inline void Add() {
            Slaves_.push_back(TSlave(&Tree_));
        }

        inline void CheckLast() {
            if (!Last()->Loader()) {
                Last()->SetLoader(TLoaderFactory::Instance().Find("plain"));
            }

            if (!Last()->File()) {
                Last()->SetFile("-");
            }
        }

        inline TSlave* Last() noexcept {
            return &Slaves_.back();
        }

    public:
        TSlaves Slaves_;
        TTree Tree_;
        TSimpleOutputter* Outputter_;
        TString OutFileName_;
        size_t PortionLength_;
        size_t Slices_;
        bool Dump_;
};

inline TInstant TMultiLoader::TOneItem::AbsTime() const noexcept {
    return Par_->AbsTime();
}

int main(int argc, char** argv) {
    try {
        THolder<TMultiLoader> loader;

        try {
            loader.Reset(new TMultiLoader(argc, argv));
        } catch (...) {
            Usage(argv[0]);

            throw;
        }

        loader->Process();

        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }

    return 1;
}
