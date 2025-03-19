#include "httpsearchclient.h"
#include "msfixedvector.h"

#include <util/random/random.h>
#include <util/system/mutex.h>
#include <util/system/tempfile.h>
#include <util/memory/pool.h>
#include <util/string/split.h>
#include <util/generic/hash.h>
#include <util/generic/cast.h>
#include <util/generic/ymath.h>
#include <util/string/ascii.h>

typedef TConnGroup::TConnData TConnData;
typedef const TConnData* TConnDataKey;

namespace NHttpSearchClient {
    size_t SmoothThreshold = 1000;
}

using NHttpSearchClient::TGroupData;

namespace {

    typedef std::pair<double, size_t> TClientWeight;

    class TClientWeightLess {
        public:
            inline bool operator ()(const TClientWeight& l, const TClientWeight& r) {
                return l.first < r.first;
            }

            inline bool operator ()(const TClientWeight& l, double r) {
                return l.first < r;
            }
    };

    template <typename T>
    static inline double Val(T& v) {
        return v;
    }

    template <>
    inline double Val<TClientWeight>(TClientWeight& v) {
        return v.first;
    }

    template <typename T>
    static inline void Scale(T& v, double mult) {
        v *= mult;
    }

    template <>
    inline void Scale<TClientWeight>(TClientWeight& v, double mult) {
        v.first *= mult;
    }

    template <class T>
    static inline void NormalizeWeights(T b, T e, const double mult) noexcept {
        while (b != e) {
            Scale(*b++, mult);
        }
    }

    template <class C>
    static inline void NormalizeWeights(C& c) noexcept {
        const double mult = (1 << 20) / Val(c.back());

        NormalizeWeights(c.begin(), c.end(), mult);
    }

    template <class It, class T>
    static inline bool Have(It b, It e, T c) noexcept {
        while (b != e) {
            if (*b == c) {
                return true;
            }

            ++b;
        }

        return false;
    }

}//namespace

namespace NHttpSearchClient {
    THolder<IConnIterator> THttpSearchClientBase::PossibleConnections(TRequestHash hash) const {
        return PossibleConnections(TSeed(hash));
    }

    THolder<IConnIterator> THttpSearchClientBase::PossibleConnections(TSeed seed) const {
        return THolder<IConnIterator>(CreateGenericIterator(seed).Release());
    }

    THolder<THttpSearchClientBase> CreateClient(const TClientOptions& opts, TConnGroup* parent, const TString& ip,
                                                TGroupSplitter groupSplitter, TClientCreator clientCreator = {});

    class TSimpleHttpSearchClient: public THttpSearchClientBase {
    public:
        class TSimpleIterator: public TGenericIterator {
        public:
            TSimpleIterator(const TConnData& connData)
                : TGenericIterator()
                , Data_(&connData)
            {
            }

            TSimpleIterator(THolder<TConnData> connData)
                : TGenericIterator()
                , Data_(connData.Get())
                , DataHolder_(connData.Release())
            {
            }

            ~TSimpleIterator() override {
            }

            void SetClientStatus(const TConnData* conn, const THostInfo& props, const TErrorDetails&) override {
                UpdateConnData(conn, props);
            }

            const TConnData* Next(size_t /*attempt*/) override {
                const TConnData* ret = nullptr;

                if (!!Data_) {
                    ret = Data_;
                    Data_ = nullptr;
                }

                return ret;
            }

            bool Eof() const override {
                return !Data_;
            }

        private:
            const TConnData* Data_ = nullptr;
            THolder<TConnData> DataHolder_;
        };

        TSimpleHttpSearchClient(const TClientOptions& opts, TConnGroup* parent, const TString& ip)
            : THttpSearchClientBase(*opts.Script, *opts.Options)
            , ConnData_(*opts.Script, Options_.EnableIpV6, Options_.EnableUnresolvedHosts
                        , Options_.EnableCachedResolve
                        , *opts.GroupId, opts.IsMain, parent, ip)
        {
            if (opts.IsMain) {
                ConnData_.Register();
            }
        }

        ~TSimpleHttpSearchClient() override {
        }

        TGenericIteratorRef CreateGenericIterator(TSeed /*seed*/) const override {
            return new TSimpleIterator(ConnData_);
        }

        void DoReportStats(IOutputStream& out) const override {
            out << "<host>" << ConnData_.Host() << "</host>";
            out << "<port>" << ConnData_.Port() << "</port>";
            out << "<ismain>" << ConnData_.Registered() << "</ismain>";
            out << "<indgen>" << ConnData_.IndexGeneration() << "</indgen>";
            out << "<src-ts>" << ConnData_.SourceTimestamp() << "</src-ts>";
            out << "<acc>" << ConnData_.Accessibility() << "</acc>";
            out << "<connsucc>" << ConnData_.Get(CDS_ConnSucceeded) << "</connsucc>";
            out << "<connfail>" << ConnData_.Get(CDS_ConnFailed) << "</connfail>";
            out << "<reqsucc>" << ConnData_.Get(CDS_ReqSucceeded) << "</reqsucc>";
            out << "<reqskip>" << ConnData_.Get(CDS_ReqSkipped) << "</reqskip>";
            out << "<reqfail>" << ConnData_.Get(CDS_ReqFailed) << "</reqfail>";
        }

    private:
        TConnData ConnData_;
    };

}

namespace {
    bool IsValidWeightChar(const char c) {
        return IsAsciiDigit(c) || c == '.';
    }

    bool IsGroupWeight(const TString& token) {
        return AllOf(token, IsValidWeightChar);
    }

    size_t GetWeightsStart(const TString& page) {
        size_t weightsStart = TString::npos;
        size_t pos = page.size();
        while (pos > 0) {
            --pos;
            if (page[pos] == '@') {
                weightsStart = pos;
            } else if (!IsValidWeightChar(page[pos])) {
                break;
            }
        }

        return weightsStart;
    }

    static void SplitToSimpleGroups(const TString& str, TVector<TGroupData>& groups) {
        TVector<TString> pages;

        StringSplitter(str).SplitBySet(" \t").SkipEmpty().Collect(&pages);

        for (const TString& page : pages) {
            const size_t weightsStart = GetWeightsStart(page);

            TGroupData data;

            data.Group = page.substr(0, weightsStart);

            if (weightsStart != TString::npos) {
                data.GroupWeights = page.substr(weightsStart + 1);
            } else {
                data.GroupWeights = "@1";
            }

            groups.push_back(data);
        }
    }

    static void SplitToBracedGroups(const TString& str, TVector<TGroupData>& groups, bool rawIpAddrs) {
        enum {
            SEARCH_OPEN_BRACE,
            SEARCH_CLOSE_BRACE,
            SEARCH_SPACE
        } state = SEARCH_OPEN_BRACE;
        TString::const_iterator groupStart = nullptr, weightsStart = nullptr, ch;
        size_t braceLevel = 0;
        TGroupData data;

        for (ch = str.begin(); ch != str.end(); ++ch) {
            switch (state) {
                case SEARCH_OPEN_BRACE:
                    if (*ch == '(') {
                        ++braceLevel;
                        groupStart = ch + 1;
                        state = SEARCH_CLOSE_BRACE;
                    } else if (*ch != ' ' && *ch != '\t') {
                        ythrow yexception() << "invalid search script";
                    }

                    break;

                case SEARCH_CLOSE_BRACE:
                    if (*ch == ')') {
                        --braceLevel;

                        if (braceLevel == 0) {
                            data.Group.append(groupStart, ch);
                            weightsStart = ch + 1;
                            state = SEARCH_SPACE;
                        }
                    } else if (*ch == '(') {
                        ++braceLevel;
                    } else if (!rawIpAddrs && (*ch == '[') && (braceLevel == 1)) {
                        TString::const_iterator optStart = ch + 1;

                        data.Group.append(groupStart, ch);

                        while (*ch != ']') {
                            ++ch;
                        }

                        groupStart = ch + 1;
                        data.GroupOptions = TString(optStart, ch - optStart);
                    }
                    break;

                case SEARCH_SPACE:
                    if (*ch == ' ' || *ch == '\t') {
                        data.GroupWeights = TString(weightsStart, ch - weightsStart);
                        groups.push_back(data);
                        data.Clear();
                        state = SEARCH_OPEN_BRACE;
                    }
                    break;
            }
        }

        if ((state == SEARCH_SPACE) && (ch - groupStart > 0)) {
            data.GroupWeights = TString(weightsStart, ch - weightsStart);
            groups.push_back(data);
            data.Clear();
        }
    }

    typedef NHttpSearchClient::THostInfo THostInfo;
    typedef TVector<TClientWeight> TClientWeights;
    typedef TVector<double> TWeights;
    typedef TVector<TWeights> TWeightsVector;

    static inline void PrepareWeights(TClientWeights& c) noexcept {
        Sort(c.begin(), c.end(), TClientWeightLess());
        double sum = 0;
        for (TClientWeights::iterator it = c.begin(); it != c.end(); ++it) {
            (*it).first = (sum += (*it).first);
        }

        NormalizeWeights(c);
    }

    class TWeightsContainer {
        public:
            TWeightsContainer(const TBalancingOptions& opts)
                : Options_(opts)
            {
            }

            template <class It>
            inline void AddWeights(It first, It last, size_t clientNum, size_t clientCount) {
                BaseWeights_.push_back(0);

                for (It it = first; it != last; ++it) {
                    const size_t i = it - first;

                    if ((i + 1) > Tables_.size()) {
                        Tables_.push_back(TWeights());
                        Tables_.back().resize(clientCount);
                    }

                    double weight = FromString<double>(*it);

                    Tables_[i][clientNum] = weight;

                    if (weight > BaseWeights_.back()) {
                        BaseWeights_.back() = weight;
                    }

                    HasZeroWeights_ |= (weight == 0);
                }
            }

            void GetBaseWeights(TClientWeights& weights) const {
                weights.clear();

                for (size_t i = 0; i < BaseWeights_.size(); ++i) {
                    weights.push_back(TClientWeight(BaseWeights_[i], i));
                }

                PrepareWeights(weights);
            }

            bool IsMainGroup(size_t idx) const {
                Y_ASSERT(!Tables_.empty());
                const TWeights& w = Tables_[0];
                Y_ASSERT(w.size() > idx);
                return w[idx] > 0;
            }

            bool NeedPing() const {
                return Options_.PingZeroWeight && (Options_.AllowDynamicWeights || HasZeroWeights_);
            }

        protected:
            const TBalancingOptions& Options_;
            TWeightsVector Tables_;
            TWeights BaseWeights_;
            bool HasZeroWeights_ = false;
            TAtomic CurIdx_ = 0;
    };

    class TStaticProvider: public TWeightsContainer {
            typedef TVector<TClientWeights> TClientWeightsVector;

        public:
            class TIteratorWeights {
                public:
                    TIteratorWeights(const TStaticProvider& parent)
                        : Parent_(parent)
                    {
                    }

                    const TClientWeights& GetWeights(size_t attempt) const {
                        return Parent_.SumTables_[attempt % Parent_.SumTables_.size()];
                    }

                    const TClientWeights& GetBaseWeights() const {
                        Parent_.GetBaseWeights(Weights_);

                        return Weights_;
                    }

                    void SetClientStatus(size_t clientNum, ui64 answerTime, const THostInfo& props) {
                        Y_UNUSED(clientNum);
                        Y_UNUSED(answerTime);
                        Y_UNUSED(props);
                    }

                    void ReportStats(IOutputStream& out, size_t num) const {
                        out << "<stat-weight>";
                        out << Parent_.BaseWeights_[num];
                        out << "</stat-weight>";
                    }

                private:
                    const TStaticProvider& Parent_;
                    mutable TClientWeights Weights_;
            };

            TStaticProvider(const TBalancingOptions& opts)
                : TWeightsContainer(opts)
            {
            }

            void BuildWeightTables() {
                if (Tables_.empty()) {
                    ythrow yexception() << "no weight tables calculated";
                }

                for (TWeightsVector::const_iterator tbl = Tables_.begin(); tbl != Tables_.end(); ++tbl) {
                    SumTables_.push_back(TClientWeights());

                    TClientWeights& sumTbl = SumTables_.back();

                    for (TWeights::const_iterator weight = tbl->begin(); weight != tbl->end(); ++weight) {
                        sumTbl.push_back(TClientWeight(*weight, weight - tbl->begin()));
                    }

                    PrepareWeights(sumTbl);
                }
            }

            static TStringBuf Type() {
                return "Static";
            }
        private:
            TClientWeightsVector SumTables_;
    };

    static inline double Avg(double avg, double value, double mult) {
        return avg * (1.0 - mult) + value * mult;
    }

    template<class TImpl, class TDynamicWeight>
    class TDynamicProviderBase: public TWeightsContainer {
            typedef TVector<TDynamicWeight> TDynamicWeights;
            typedef TMsFixedVector<TDynamicWeight> TSharedDynamicWeights;

        public:
            class TIteratorWeights {
                public:
                    TIteratorWeights(const TDynamicProviderBase& parent)
                        : Parent_(parent)
                    {
                        Parent().GetDynamicWeights(DynamicWeights_);
                        Weights_.reserve(DynamicWeights_.size());
                    }

                    const TClientWeights& GetWeights(size_t attempt) const {
                        Weights_.clear();

                        const TWeights& baseWeights = Parent().BaseWeights_;
                        const TWeights& staticWeights = Parent().Tables_[attempt % Parent().Tables_.size()];

                        Y_ASSERT(staticWeights.size() == DynamicWeights_.size());

                        double overloadWeight = 0;

                        if (Parent().DistributeBadSourceWeight_) {
                            for (size_t i = 0; i < DynamicWeights_.size(); ++i) {
                                if (DynamicWeights_[i].Weight < Parent().Options_.WeightDistributionThreshold) {
                                    overloadWeight += (1.0 - DynamicWeights_[i].Weight) * staticWeights[i];
                                }
                            }
                        }

                        for (size_t i = 0; i < DynamicWeights_.size(); ++i) {
                            double val = 1000 * (staticWeights[i] + overloadWeight * baseWeights[i])
                                              * DynamicWeights_[i].GetWeight();
                            Weights_.push_back(TClientWeight(val, i));
                        }

                        PrepareWeights(Weights_);

                        return Weights_;
                    }

                    const TClientWeights& GetBaseWeights() const {
                        Parent_.GetBaseWeights(Weights_);

                        return Weights_;
                    }

                    void SetClientStatus(size_t clientNum, ui64 answerTime, const THostInfo& props) {
                        Parent().UpdateWeights(clientNum, answerTime, props);
                    }

                    void ReportStats(IOutputStream& out, size_t num) const {
                        const TDynamicWeight& wh = DynamicWeights_[num];

                        out << "<stat-weight>" << Parent_.BaseWeights_[num] << "</stat-weight>";
                        out << "<dyn-weight>";
                        wh.ReportStats(out);
                        out << "</dyn-weight>";
                    }

                private:
                    inline TDynamicProviderBase& Parent() const noexcept {
                        return const_cast<TDynamicProviderBase&>(Parent_);
                    }

                private:
                    const TDynamicProviderBase& Parent_;
                    TDynamicWeights DynamicWeights_;
                    mutable TClientWeights Weights_;
            };

            TDynamicProviderBase(const TBalancingOptions& opts)
                : TWeightsContainer(opts)
                , Pool_(0)
                , MinDynWeight_(Options_.MinDynWeight)
            {
                Mult_[0] = Options_.BadDynWeightMult;
                Mult_[1] = Options_.GoodDynWeightMult;
            }

            void BuildWeightTables() {
                if (Tables_.empty()) {
                    ythrow yexception() << "no weight tables calculated";
                }

                DistributeBadSourceWeight_ = HasZeroWeights_ && (Options_.WeightDistributionThreshold > 0.0);
                TSharedDynamicWeights(&Pool_, BaseWeights_.size()).Swap(DynamicWeights_);
            }

        private:
            void GetDynamicWeights (TDynamicWeights& res) {
                TGuard<TMutex> lock(Lock_);

                res.assign(DynamicWeights_.Begin(), DynamicWeights_.End());
            }

            void UpdateWeights(size_t clientNum, ui64 answerTime, const THostInfo& props) {
                TGuard<TMutex> lock(Lock_);

                (static_cast<TImpl*>(this))->DoUpdateWeights(clientNum, answerTime, props);
            }

        private:
            TMemoryPool Pool_;
            TMutex Lock_;

        protected:
            TSharedDynamicWeights DynamicWeights_;
            const double MinDynWeight_ = 0.1;
            const double AvgSlowMult_ = 0.001;
            const double AvgFastMult_ = 0.1;
            bool DistributeBadSourceWeight_ = false;
            double Mult_[2];
    };

    struct TDynamicWeightBase {
            inline TDynamicWeightBase() noexcept {
            }

            inline size_t ReqsCount() const noexcept {
                return Succ + Fail;
            }

            inline double ApplySmoothFunction(double x) const noexcept {
                const size_t N = NHttpSearchClient::SmoothThreshold;
                const size_t n = Min(ReqsCount(), N);

                return (x * n + 1.0 * (N - n)) / N;
            }

            inline void ReportStats(IOutputStream& out) const {
                {
                    out << "<weight>";
                    out << Weight;
                    out << "</weight>";
                }

                {
                    out << "<succ>";
                    out << Succ;
                    out << "</succ>";
                }

                {
                    out << "<fail>";
                    out << Fail;
                    out << "</fail>";
                }
            }

            double Weight = 1.0;
            size_t Ncpu = 1;
            size_t Succ = 0;
            size_t Fail = 0;
    };

    struct TRobustnessDynamicWeight: public TDynamicWeightBase {
            inline TRobustnessDynamicWeight() noexcept
                : TDynamicWeightBase()
            {
            }

            inline double GetWeight() const noexcept {
                return Weight * Robustness;
            }

            inline void ReportStats(IOutputStream& out) const {
                TDynamicWeightBase::ReportStats(out);
                out << "<robustness>" << Robustness << "</robustness>";
            }

            double Robustness = 1.0;
    };

    class TRobustnessDynamicProvider : public TDynamicProviderBase<TRobustnessDynamicProvider, TRobustnessDynamicWeight> {
            typedef TDynamicProviderBase<TRobustnessDynamicProvider, TRobustnessDynamicWeight> TBase;

        public:
            TRobustnessDynamicProvider(const TBalancingOptions& opts)
                : TBase(opts)
            {
            }

            void DoUpdateWeights(size_t num, ui64 answerTime, const THostInfo& props) {
                Y_ASSERT(num < DynamicWeights_.Size());

                const bool success = answerTime > 0;
                TRobustnessDynamicWeight& wh = DynamicWeights_[num];

                wh.Weight = Max(Min(Mult_[success] * wh.Weight, 1.0), MinDynWeight_);

                if (success && props.Accessibility >= 0.0) {
                    double weight;

                    if (props.Accessibility < 1.0) {
                        weight = Max<double>(Avg(wh.Robustness, props.Accessibility, 0.1), Max<double>(props.Accessibility, 0.01));
                    } else {
                        weight = Min<double>(Avg(wh.Robustness, props.Accessibility, 0.3), 1.0);
                    }

                    wh.Robustness = weight;
                }
            }

            static TStringBuf Type() {
                return "Robust";
            }
    };

    struct TPendulumDynamicWeight: public TDynamicWeightBase {
            inline TPendulumDynamicWeight() noexcept
                : TDynamicWeightBase()
            {
            }

            inline double GetWeight() const noexcept {
                return ApplySmoothFunction(10000.0 * Weight / (AvgTime * AvgNctx / Ncpu + 1.0));
            }

            inline void ReportStats(IOutputStream& out) const {
                TDynamicWeightBase::ReportStats(out);
                {
                    out << "<avg-time>";
                    out << AvgTime;
                    out << "</avg-time>";
                }

                {
                    out << "<load-weight>";
                    out << GetWeight();
                    out << "</load-weight>";
                }

                {
                    out << "<avg-nctx>";
                    out << AvgNctx;
                    out << "</avg-nctx>";
                }
            }

            double AvgTime = 1000000.0;
            double AvgNctx = 10.0;
    };

    class TPendulumDynamicProvider : public TDynamicProviderBase<TPendulumDynamicProvider, TPendulumDynamicWeight> {
            typedef TDynamicProviderBase<TPendulumDynamicProvider, TPendulumDynamicWeight> TBase;

        public:
            TPendulumDynamicProvider(const TBalancingOptions& opts)
                : TBase(opts)
            {
            }

            void DoUpdateWeights(size_t num, ui64 answerTime, const THostInfo& props) {
                Y_ASSERT(num < DynamicWeights_.Size());

                const bool success = answerTime > 0;
                TPendulumDynamicWeight& wh = DynamicWeights_[num];

                wh.Weight = Max(Min(Mult_[success] * wh.Weight, 1.0), MinDynWeight_);

                if (success) {
                    size_t ncpu = props.Ncpu;

                    ++wh.Succ;
                    if (props.Nctx && props.BsTouched) {
                        wh.AvgTime = Avg(wh.AvgTime, answerTime, AvgSlowMult_);
                        wh.AvgNctx = Avg(wh.AvgNctx, props.Nctx, AvgFastMult_);
                        wh.Ncpu = ncpu ? ncpu : 1;
                    }
                } else {
                    ++wh.Fail;
                }

            }

            static TStringBuf Type() {
                return "Pendulum";
            }
    };

    struct TSteadyDynamicWeight: public TDynamicWeightBase  {
            inline TSteadyDynamicWeight() noexcept
                : TDynamicWeightBase()
            {
            }

            inline double CalculateLoadWeight() const noexcept {
                double nctxDiff = AvgNctxDiff > 0 ? AvgNctxDiff : 0.0;

                return sqrt(AvgTime * (AvgUnAns + 1.0)) * (1.0 + nctxDiff) * 0.001;
            }

            inline double GetWeight() const noexcept {
                return Weight * LoadWeight;
            }

            inline void ReportStats(IOutputStream& out) const {
                TDynamicWeightBase::ReportStats(out);

                {
                    out << "<avg-time>";
                    out << AvgTime;
                    out << "</avg-time>";
                }

                {
                    out << "<load-weight>";
                    out << GetWeight();
                    out << "</load-weight>";
                }

                {
                    out << "<avg-nctx>";
                    out << AvgNctx;
                    out << "</avg-nctx>";
                }

                {
                    out << "<avg-unans>";
                    out << AvgUnAns;
                    out << "</avg-unans>";
                }
            }

            double AvgTime = 1.0;
            double AvgNctx = 0.0;
            double AvgUnAns = 0.0;
            double AvgNctxDiff = 0.0;
            double LoadWeight = 1.0;
    };

    class TSteadyDynamicProvider : public TDynamicProviderBase<TSteadyDynamicProvider, TSteadyDynamicWeight> {
            typedef TDynamicProviderBase<TSteadyDynamicProvider, TSteadyDynamicWeight> TBase;

        public:
            TSteadyDynamicProvider(const TBalancingOptions& opts)
                : TBase(opts)
            {
            }

            void DoUpdateWeights(size_t num, ui64 answerTime, const THostInfo& props) {
                Y_ASSERT(num < DynamicWeights_.Size());

                const bool success = answerTime > 0;
                TSteadyDynamicWeight& wh = DynamicWeights_[num];

                wh.Weight = Max(Min(Mult_[success] * wh.Weight, 1.0), MinDynWeight_);
                wh.AvgUnAns = Avg(wh.AvgUnAns, props.UnanswerCount, wh.ApplySmoothFunction(AvgSlowMult_));

                if (success) {
                    ++wh.Succ;
                    if (props.Nctx && props.BsTouched) {
                        double avgNctxPrev = wh.AvgNctx;

                        wh.AvgTime = Avg(wh.AvgTime, answerTime, AvgSlowMult_);
                        wh.AvgNctx = Avg(wh.AvgNctx, props.Nctx, AvgFastMult_);
                        wh.AvgNctxDiff = Avg(wh.AvgNctxDiff, (wh.AvgNctx - avgNctxPrev), AvgFastMult_);
                    }
                } else {
                    ++wh.Fail;
                }

                AvgLoadWeight_ = Avg(AvgLoadWeight_, wh.CalculateLoadWeight(), AvgFastMult_);

                double dw = wh.LoadWeight - (wh.CalculateLoadWeight() / AvgLoadWeight_ - 1.0) * 0.01;
                wh.LoadWeight = wh.ApplySmoothFunction(Max(Min(dw, 3.0), MinDynWeight_));
            }

            static TStringBuf Type() {
                return "Steady";
            }
        private:
            double AvgLoadWeight_ = 1.0;
    };
}

namespace NHttpSearchClient {
    bool SplitToGroups(const TString& str, TVector<TGroupData>& groups, bool rawIpAddrs) {
        const bool hasBrace = (str.find('(') != TString::npos);
        const bool weighted =
            hasBrace ||
            (str.find(' ') != TString::npos) ||
            (GetWeightsStart(str) != TString::npos);

        if (!weighted) {
            return false;
        }

        if (hasBrace) {
            SplitToBracedGroups(str, groups, rawIpAddrs);
        } else {
            SplitToSimpleGroups(str, groups);
        }

        return true;
    }


    template<class T>
    class TWeightedClient: public THttpSearchClientBase {
    public:
            using TBalancingProvider = T;
    private:
            typedef typename TBalancingProvider::TIteratorWeights TIteratorWeights;

            struct TIteratorParams {
                const TWeightedClient* Parent;
                const TRequestHash Hash;
                const TRequestHash HashToPass;
                const bool AllowRandomGroupSelection;
                const bool LinearHash;
            };

            class TWeightedIterator: public TGenericIterator {
                    typedef TVector<TGenericIteratorRef> TIterators;
                    typedef THashMap<TConnDataKey, size_t, THash<TConnDataKey> > TConnData2Idx;

                public:
                    TWeightedIterator(const TIteratorParams& p)
                        : TGenericIterator()
                        , Parent_(p.Parent)
                        , DoPing_(p.AllowRandomGroupSelection && Parent_->Provider_.NeedPing() && (RandomNumber<float>() < 0.005))
                        , ItWeights_(Parent_->Provider_)
                        , ClientIterators_(ClientCount())
                        , InitialHash_(p.Hash)
                        , Hash_(InitialHash_)
                        , HashToPass_(p.HashToPass)
                        , CachedWeights_(std::make_pair(0, &WeightsForAttempt(0)))
                        , AllowRandomGroupSelection_(p.AllowRandomGroupSelection)
                        , LinearHash_(p.LinearHash)
                    {
                    }

                    ~TWeightedIterator() override {
                    }

                    const TConnData* Next(size_t attempt) override {
                        if (!Eof()) {
                            const TConnData* next = GetConnData(attempt);

                            // not use RandomGroupSkipping when iterate by connData not for real request,
                            // but for collecting all search source hosts (symptom: HashToPass_ == 0)
                            if (Parent_->Options_.RandomGroupSkipping > 0 && attempt == 0 && HashToPass_ != 0) {
                                if (RandomNumber<float>() < Parent_->Options_.RandomGroupSkipping) {
                                    next = GetConnData(attempt);
                                }
                            }

                            return next;
                        }

                        return nullptr;
                    }

                    void SetClientStatus(const TConnData* conn, const THostInfo& props, const TErrorDetails& errorDetails) override {
                        TConnData2Idx::const_iterator idx = ConnDataIdxs_.find(conn);

                        Y_ASSERT(idx != ConnDataIdxs_.end() && !!ClientIterators_[idx->second]);

                        ItWeights_.SetClientStatus(idx->second, errorDetails.RequestDuration.MicroSeconds(), props);
                        ClientIterators_[idx->second]->SetClientStatus(conn, props, errorDetails);
                    }

                    bool Eof() const override {
                        if (UsedCount() < ClientCount()) {
                            return false;
                        }

                        for (TIterators::const_iterator it = ClientIterators_.begin(); it != ClientIterators_.end(); ++it) {
                            const TGenericIterator* iter = (*it).Get();

                            if (!iter || !iter->Eof()) {
                                return false;
                            }
                        }

                        return true;
                    }

                private:
                    inline TWeightedClient* Parent() const noexcept {
                        return const_cast<TWeightedClient*>(Parent_);
                    }

                    inline size_t ClientCount() const noexcept {
                        return Parent()->Clients_.size();
                    }

                    inline size_t CalculateNext(size_t attempt) {
                        Hash_ = InitialHash_;
                        size_t num = DoNext(attempt);
                        size_t count = 0;

                        while (ClientMustBeSkipped(num) && count < ClientCount()) {
                            Hash_ = ShiftBits(Hash_);
                            HashToPass_ = ShiftBits(HashToPass_);
                            num = DoNext(attempt);
                            ++count;
                        }
                        while (ClientMustBeSkipped(num) && count < ClientCount() * 2) {
                            num = (num + 1) % ClientCount();
                            ++count;
                        }

                        return num;
                    }

                    inline bool ClientMustBeSkipped(size_t num) const {
                        const TGenericIterator* iter = ClientIterators_[num].Get();
                        const bool notAllClientsHasBeenTouched = UsedCount() < ClientCount();

                        /*
                         * Hint: iter == 0 if corresponding client has not been touched
                         */
                        return (iter != nullptr) && (notAllClientsHasBeenTouched || iter->Eof());
                    }

                    inline size_t UsedCount() const noexcept {
                        size_t count = 0;

                        for (TIterators::const_iterator it = ClientIterators_.begin(); it != ClientIterators_.end(); ++it) {
                            if (!!*it) {
                                ++count;
                            }
                        }

                        return count;
                    }

                    inline const TConnData* GetConnData(size_t attempt) {
                        const TConnData* res = nullptr;
                        size_t item = 0;

                        do {
                            item = CalculateNext(attempt);
                            TGenericIteratorRef& iter = ClientIterators_[item];

                            if (!iter.Get()) {
                                TSeed seed;
                                if (LinearHash_) {
                                    seed.Hash = HashToPass_;
                                } else {
                                    seed.Hash = ShiftBits(HashToPass_);
                                }
                                seed.LinearHash = LinearHash_;
                                seed.AllowRandomGroupSelection = AllowRandomGroupSelection_;

                                iter = Parent()->Clients_[item]->CreateGenericIterator(seed);
                            }

                            Y_ASSERT(iter.Get());

                            res = iter->Next(attempt);
                        } while (!res && !Eof());

                        if (res) {
                            ConnDataIdxs_.insert(TConnData2Idx::value_type(res, item));
                        }

                        return res;
                    }

                    inline const TClientWeights& WeightsForAttempt(size_t attempt) const {
                        return DoPing_ ? ItWeights_.GetBaseWeights() : ItWeights_.GetWeights(attempt);
                    }

                    const TClientWeights& SelectWeights(size_t attempt) {
                        if (attempt != CachedWeights_.first) {
                            const TClientWeights& weights = WeightsForAttempt(attempt);
                            CachedWeights_ = std::make_pair(attempt, &weights);
                        }

                        return *CachedWeights_.second;
                    }

                    size_t DoNext(size_t attempt) {
                        return ClientByHash(SelectWeights(attempt));
                    }

                    template <class C>
                    inline size_t ClientByHash(const C& c) const noexcept {
                        return ClientByHash(c.begin(), c.end());
                    }

                    template <class It>
                    inline size_t ClientByHash(It beg, It end) const noexcept {
                        Y_ASSERT(beg != end);

                        if (end - beg < 2) {
                            return 0;
                        }

                        const double sum = (*(end - 1)).first;

                        if (LinearHash_) {
                            const double m = sum * Hash_ / MaxCeil<TRequestHash>();
                            return LowerBound(beg, end, m, TClientWeightLess())->second;
                        }

                        // ACHTUNG! when ((all static weights a same) && (# of groups == # of clients in each group))
                        // -- aka square matrix
                        // skiping of first step (e.g. when RandomGroupSkipping occurs) will cause the dependence of selected
                        // client in group and group number due to Hash_ == ShiftBits(HashToPass_)
                        // AVOID SQUARE MATRIX IN CONFIG
                        const double step = (sum - beg->first < 1.0) ? 0 : beg->first;
                        double m = (Hash_ % (TRequestHash)(sum - step + 0.1))*sum/(sum - step + 0.1);

                        return LowerBound(beg, end, m, TClientWeightLess())->second;
                    }

                private:
                    const TWeightedClient* Parent_ = nullptr;
                    const bool DoPing_ = true;
                    TIteratorWeights ItWeights_;
                    TIterators ClientIterators_;
                    TRequestHash InitialHash_ = 0;
                    TRequestHash Hash_ = 0;
                    TRequestHash HashToPass_ = 0;
                    TConnData2Idx ConnDataIdxs_;
                    std::pair<size_t,  const TClientWeights*> CachedWeights_;
                    bool AllowRandomGroupSelection_ = true;
                    bool LinearHash_ = false;
            };

        public:
            TWeightedClient(const TVector<TGroupData>& pageGroups, const TClientOptions& opts, TConnGroup* parent,
                            TGroupSplitter groupSplitter, TClientCreator clientCreator)
                : THttpSearchClientBase(*opts.Script, *opts.Options)
                , GroupId_(*opts.GroupId)
                , Provider_(Options_)
            {
                TVector<TString> groupIds(Reserve(pageGroups.size()));
                for (TVector<TGroupData>::const_iterator it = pageGroups.begin(); it != pageGroups.end(); ++it) {
                    TVector<TString> tmp;

                    StringSplitter(it->GroupWeights).Split('@').SkipEmpty().Collect(&tmp);

                    if (tmp.empty()) {
                        ythrow yexception() << "shit happen while parsing " << *opts.GroupId;
                    }

                    bool hasGroupId = !IsGroupWeight(tmp[0]);
                    Provider_.AddWeights(tmp.begin() + hasGroupId, tmp.end(), it - pageGroups.begin(), pageGroups.size());

                    if (hasGroupId) {
                        groupIds.push_back(TString::Join(GroupId_, ".", tmp[0]));
                    } else {
                        size_t groupNum = it - pageGroups.begin();
                        groupIds.push_back(TString::Join(GroupId_, ".", ToString(groupNum)));
                    }
                }

                Provider_.BuildWeightTables();

                for (TVector<TGroupData>::const_iterator grp = pageGroups.begin(); grp != pageGroups.end(); ++grp) {
                    TBalancingOptions options(Options_);
                    size_t groupNum = grp - pageGroups.begin();

                    if (!options.EnableInheritance) {
                        options.AllowDynamicWeights = false;
                        options.RandomGroupSelection = false;
                        options.RandomGroupSkipping = 0.0;
                    }

                    if (!grp->GroupOptions.empty()) {
                        options.Parse(grp->GroupOptions);
                    }

                    TClientOptions copy(opts);

                    copy.Script = &grp->Group;
                    copy.Options = &options;
                    copy.GroupId = &groupIds[groupNum];
                    copy.IsMain = opts.IsMain && Provider_.IsMainGroup(groupNum);

                    try {
                        THolder<THttpSearchClientBase> client;
                        if (clientCreator) {
                            client = clientCreator(copy, parent, grp->Ip, groupSplitter);
                        }
                        if (!client) {
                            client = CreateClient(copy, parent, grp->Ip, groupSplitter, clientCreator);
                        }
                        Clients_.push_back(std::move(client));
                    } catch (const TSystemError&) {
                        Cerr << "[System Error] " << CurrentExceptionMessage() << Endl;
                    }
                }

                if (Clients_.empty()) {
                    throw yexception() << "Empty client group (" << *opts.Script << ")";
                }
            }

            ~TWeightedClient() override {
            }

            TGenericIteratorRef CreateGenericIterator(TSeed seed) const override {
                const TIteratorParams p = {
                    this,
                    (Options_.RandomGroupSelection && seed.AllowRandomGroupSelection) ? RandomNumber<TRequestHash>() : seed.Hash,
                    seed.Hash,
                    seed.AllowRandomGroupSelection,
                    seed.LinearHash
                };

                return new TWeightedIterator(p);
            }

            void DoReportStats(IOutputStream& out) const override {
                {
                    out << "<type>";
                    out << TBalancingProvider::Type();
                    out << "</type>";
                }

                out << "<conns>";
                typename TBalancingProvider::TIteratorWeights w(Provider_);

                for (size_t i = 0; i < Clients_.size(); ++i) {
                    out << "<conn>";

                    w.ReportStats(out, i);

                    {
                        out << "<client>";
                        Clients_[i]->DoReportStats(out);
                        out << "</client>";
                    }

                    out << "</conn>";
                }

                out << "</conns>";
            }

        private:
            typedef TVector<THolder<THttpSearchClientBase>> TClients;

            const TString GroupId_;
            TClients Clients_;
            TBalancingProvider Provider_;
    };

    class TDynamicWeightedClientFactory {
    public:
        TDynamicWeightedClientFactory() {
            Register<TWeightedClient<TPendulumDynamicProvider>>("");
            Register<TWeightedClient<TPendulumDynamicProvider>>("Default");

            Register<TWeightedClient<TPendulumDynamicProvider>>();
            Register<TWeightedClient<TSteadyDynamicProvider>>();
            Register<TWeightedClient<TRobustnessDynamicProvider>>();
        }

        template <typename... Args>
        THolder<THttpSearchClientBase> Create(TStringBuf name, Args&&... args) const {
            auto it = Creators_.find(name);
            if (it == Creators_.end()) {
                Cerr << "Cannot create " << name << " dynamic balancer. Use default" << Endl;
                it = Creators_.find("");
            }

            Y_VERIFY(it != Creators_.end());

            return it->second(std::forward<Args>(args)...);
        }
    private:
        template <class T>
        void Register(TStringBuf type = T::TBalancingProvider::Type()) {
            Creators_[type] = [](const TVector<TGroupData>& groups, const TClientOptions& opts, TConnGroup* parent,
                    TGroupSplitter groupSplitter, TClientCreator clientCreator) {
                return MakeHolder<T>(groups, opts, parent, groupSplitter, clientCreator);
            };
        }
    private:
        using TCreator = std::function<THolder<THttpSearchClientBase>(const TVector<TGroupData>&, const TClientOptions&,
                                                                      TConnGroup*, TGroupSplitter, TClientCreator)>;
        THashMap<TString, TCreator> Creators_;
    };

    THolder<THttpSearchClientBase> CreateClient(const TClientOptions& opts, TConnGroup* parent, const TString& ip,
                                                TGroupSplitter groupSplitter, TClientCreator clientCreator) {
        if (!*opts.Script) {
            Y_ENSURE(opts.Options->AllowEmptySources);
            return MakeHolder<TEmptySearchClient>();
        }

        TVector<TGroupData> groups;
        const bool weighted = groupSplitter(opts.Script->data(), groups, opts.Options->RawIpAddrs);

        if (weighted) {
            if (ip) {
                throw yexception() << "explicit ip address for weighted client";
            }

            if (groups.empty()) {
                Y_ENSURE(opts.Options->AllowEmptySources);
                return MakeHolder<TEmptySearchClient>();
            }

            return CreateWeightedClient(groups,opts, parent, groupSplitter, clientCreator);
        } else {
            return MakeHolder<TSimpleHttpSearchClient>(opts, parent, ip);
        }
    }

    THolder<THttpSearchClientBase> CreateClient(const TClientOptions& opts, TConnGroup* parent, TGroupSplitter groupSplitter,
                                                TClientCreator clientCreator) {
        return CreateClient(opts, parent, "", groupSplitter, clientCreator);
    }

    THolder<THttpSearchClientBase> CreateWeightedClient(const TVector<TGroupData>& groups, const TClientOptions& opts, TConnGroup* parent,
                                                        TGroupSplitter groupSplitter, TClientCreator clientCreator) {
        if (opts.Options->AllowDynamicWeights) {
            return Singleton<TDynamicWeightedClientFactory>()->Create(opts.Options->DynBalancerType, groups, opts, parent,
                                                                      groupSplitter, clientCreator);
        } else {
            return MakeHolder<TWeightedClient<TStaticProvider>>(groups, opts, parent, groupSplitter, clientCreator);
        }
    }

}
