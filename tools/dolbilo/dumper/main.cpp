#include "responses_mode.h"

#include <library/cpp/dolbilo/stat.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/push_parser/http_parser.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/generic/array_size.h>
#include <util/memory/tempbuf.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/string/vector.h>
#include <util/datetime/base.h>

#include <cmath>

static const TString DEFAULT_FIXED_INTERVALS = "def";

class TColumnsFormatter {
    public:
        struct TNewLine {
        };

        struct TNextColumn {
        };

        inline TColumnsFormatter(size_t cols)
            : ColWidth_(new size_t[cols])
            , NCol_(cols)
        {
            memset(ColWidth_.Get(), 0, sizeof(size_t) * NCol_);
            Fields_.push_back("");
        }

        inline TColumnsFormatter& operator<<(const TString& s) {
            TString& a = Fields_.back();
            a += s;

            const size_t w = a.length();
            size_t& mw = ColWidth_[CurrCol_];

            if (w > mw) {
                mw = w;
            }

            return *this;
        }

        inline TColumnsFormatter& operator<<(const TNewLine&) {
            for (size_t i = 0; i < NCol_ - CurrCol_; ++i) {
                Fields_.push_back("");
            }

            CurrCol_ = 0;

            return *this;
        }

        inline TColumnsFormatter& operator<<(const TNextColumn&) {
            Fields_.push_back("");
            CurrCol_ = (CurrCol_ + 1) % NCol_;

            return *this;
        }

        inline void Flush(IOutputStream& outs) {
            if (Fields_.back() != "" || CurrCol_) {
                *this << TNewLine();
            }

            if (Fields_.back() == "") {
                Fields_.pop_back();
            }

            TString out;
            TVector<TString>::iterator r = Fields_.begin();

            while (r != Fields_.end()) {
                for (size_t i = 0; i < NCol_; ++i) {
                    size_t slen = r->length();
                    const size_t colwidth = ColWidth_[i];
                    if (slen < colwidth) {
                        slen = colwidth;
                    }

                    const size_t len = out.length();
                    out += *r++;
                    out.resize(len + slen, ' ');
                }

                outs << out << Endl;
                out.resize(0);
            }
        }

    private:
        TVector<TString> Fields_;
        TArrayHolder<size_t> ColWidth_;
        size_t NCol_ = 0;
        size_t CurrCol_ = 0;
};

static const TColumnsFormatter::TNewLine NewLine = {};
static const TColumnsFormatter::TNextColumn NextColumn = {};

static inline TString HRTime(const TInstant& time) {
    time_t value = time.TimeT();
    TString result = CTimeR(&value);
    return StripInPlace(result);
}

static inline TString DoubleToString(double x, size_t precision) {
    TString out = ToString(x);
    size_t idx = out.find('.');

    const bool notfound = idx == TString::npos;
    size_t n = notfound ? out.length() : idx;

    if (precision) {
        n += precision + 1;

        if (notfound) {
            out += '.';
        }
    }

    out.resize(n, '0');
    return out;
}

static inline TString MicrosecToTimeStr(ui64 t1) {
    unsigned t = t1 / 1000000;

    const unsigned nmin = 60;
    const unsigned nhour = nmin * 60;
    const unsigned nday = 24 * nhour;

    const unsigned day = t / nday;
    t %= nday;

    const unsigned hour = t / nhour;
    t %= nhour;

    const unsigned min = t / nmin;
    t %= nmin;

    const unsigned sec = t;

    char buf[1024];
    if (day) {
        sprintf(buf, "%dd %02d:%02d:%02d", day, hour, min, sec);
    } else {
        sprintf(buf, "%02d:%02d:%02d", hour, min, sec);
    }

    return TString(buf);
}

static inline TVector<ui64> ParseBoundsParam(const TString& s) {
    TVector<TString> b = SplitString(s, ",");

    TVector<ui64> bounds;
    bounds.reserve(b.size());

    for (auto r : b) {
        const unsigned z = FromString<unsigned>(StripInPlace(r));

        if (!bounds.empty() && bounds.back() >= z) {
            ythrow yexception() << "invalid bounds";
        }

        bounds.push_back(z);
    }

    return bounds;
}

struct TOpts {
    inline TOpts(int argc, char** argv) {
        NLastGetopt::TOpts opts;
        opts.SetTitle("===========================================================\n"
                      "See https://wiki.yandex-team.ru/JandeksPoisk/Sepe/Dolbilka/\n"
                      "===========================================================\n");
        opts.AddHelpOption('h');
        opts.AddLongOption('a', "stats-all", "Out all possible stats.")
            .Optional().NoArgument()
            .StoreValue(&OutAll, true);
        opts.AddLongOption('c', "stats-common", "Out common stats.")
            .Optional().NoArgument()
            .StoreValue(&OutCommon, true);
        opts.AddLongOption('t', "timing", "Out request timings.")
            .Optional().NoArgument()
            .StoreValue(&OutRequestTimings, true);
        opts.AddLongOption('T', "timeline", "Out request timeline.")
            .Optional().NoArgument()
            .StoreValue(&OutRequestTimeline, true);
        opts.AddLongOption('u', "responses", "Out responses.")
            .Optional()
            .OptionalArgument()
            .StoreResult(&OutResponsesModeString)
            .StoreValue(&OutResponses, true)
            .StoreValue(&OutResponsesMode, ResponsesMode::Raw);
        opts.AddLongOption('A', "all-requests", "Out all requests.")
            .Optional().NoArgument()
            .StoreValue(&OutAllReqs, true);
        opts.AddLongOption('H', "hist", "Out time histogram.")
            .Optional().NoArgument()
            .StoreValue(&OutHistogram, true);
        opts.AddLongOption('l', "hist-ln", "Make logarithmic histogram.")
            .Optional().NoArgument()
            .StoreValue(&OutHistogramLn, true);
        opts.AddLongOption('m', "hist-fixed",
                "Use fixed bounds in histogram.\n"
                "Bounds format is '" + DEFAULT_FIXED_INTERVALS + "' for default bounds\n"
                "or 'b_1, b_2, b_3, ..., b_n', where\n"
                "b_i - bound in microsec, b_i < b_{i+1}.")
            .Optional().RequiredArgument("BOUNDSLIST")
            .StoreResult(&FixedIntervals);
        opts.AddLongOption('L', "long-requests", "Out long requests.")
            .Optional().RequiredArgument("DURATION")
            .StoreResult(&OutLongReqs);
        opts.AddLongOption('q', "quant", "Show response time quantiles.")
            .Optional().NoArgument()
            .StoreValue(&OutResponseTimeQuant, true);
        opts.AddLongOption('d', "dump-type","Dump type.")
            .Optional().RequiredArgument("TYPE")
            .StoreResult(&DumpType);
        opts.AddLongOption('f', "input",
                "File with executor dump (stdin by default)")
            .Optional().RequiredArgument("FILE")
            .StoreResult(&FileName);

        const NLastGetopt::TOptsParseResult optsres(&opts, argc, argv);

        if (!FixedIntervals.empty()) {
            OutHistogramFixed = true;
        }

        if (OutAll) {
            OutHistogram = true;
            OutHistogramLn = true;
            if (!OutHistogramFixed) {
                FixedIntervals = DEFAULT_FIXED_INTERVALS;
                OutHistogramFixed = true;
            }
            OutCommon = true;
        }

        TryFromString(OutResponsesModeString, OutResponsesMode);
    }

    TString FileName = "-";
    bool OutHistogram = false;
    bool OutHistogramLn = false;
    bool OutHistogramFixed = false;
    bool OutCommon = false;
    bool OutRequestTimings = false;
    bool OutRequestTimeline = false;
    bool OutResponseTimeQuant = false;
    bool OutResponses = false;
    TString OutResponsesModeString;
    ResponsesMode OutResponsesMode = ResponsesMode::Off;
    bool OutAll = false;
    bool OutAllReqs = false;
    TString DumpType;
    TString FixedIntervals;
    TDuration OutLongReqs;
};

class IStat {
    public:
        inline IStat() noexcept {
        }

        virtual ~IStat() {
        }

        virtual void Process(const TDevastateStatItem& item) = 0;
        virtual void Out(IOutputStream& out) = 0;
};

class TStats: public IStat {
        typedef TVector<IStat*> TStorage;
    public:
        inline TStats() {
        }

        inline void operator() (const TDevastateStatItem& item) {
            Process(item);
        }

        inline void Add(TAutoPtr<IStat> s) {
            Storage_.push_back(s.Get());
            Y_UNUSED(s.Release());
        }

        ~TStats() override {
            for (TStorage::const_iterator it = Storage_.begin(); it != Storage_.end(); ++it) {
                delete *it;
            }
        }

        void Process(const TDevastateStatItem& item) override {
            for (TStorage::iterator it = Storage_.begin(); it != Storage_.end(); ++it) {
                (*it)->Process(item);
            }
        }

        void Out(IOutputStream& out) override {
            for (TStorage::iterator it = Storage_.begin(); it != Storage_.end(); ++it) {
                (*it)->Out(out);

                if (it + 1 != Storage_.end()) {
                    out << '\n';
                }
            }
        }

    private:
        TStorage Storage_;
};

class TLongReqStats: public IStat {
    public:
        inline TLongReqStats(const TDuration& t)
            : T_(t)
        {
        }

        ~TLongReqStats() override {
        }

        void Process(const TDevastateStatItem& item) override {
            const TDuration delta = item.EndTime() - item.StartTime();

            if (delta > T_) {
                Reqs_.push_back(item);
            }
        }

        void Out(IOutputStream& out) override {
            for (size_t i = 0; i < Reqs_.size(); ++i) {
                out << Reqs_[i].StartTime().MicroSeconds() << "\t" << Reqs_[i].EndTime().MicroSeconds() << "\t" << Reqs_[i].Url() << Endl;
            }
        }

    private:
        TDuration T_;
        TVector<TDevastateStatItem> Reqs_;
};

class TAllReq: public IStat {
    public:
        inline TAllReq() {
        }

        ~TAllReq() override {
        }

        void Process(const TDevastateStatItem& item) override {
            Reqs_.push_back(item);
        }

        void Out(IOutputStream& out) override {
            for (size_t i = 0; i < Reqs_.size(); ++i) {
                const TDevastateStatItem& item = Reqs_[i];
                const TDuration delta = item.EndTime() - item.StartTime();
                out << delta.MicroSeconds() << "\t" << item.StartTime().MicroSeconds() << "\t" << item.EndTime().MicroSeconds() << "\t" << item.Url() << Endl;
            }
        }

    private:
        TVector<TDevastateStatItem> Reqs_;
};

class TCommonStats: public IStat {
        typedef THashMap<TString, size_t> TGroupByCode;
    public:
        ~TCommonStats() override {
        }

        void Process(const TDevastateStatItem& item) override {
            ++ErrCodes_[item.ErrDescription()];

            if (item.ErrCode()) {
                ++ErrCount_;
            } else {
                ++Count_;

                if (!StartTime_.GetValue() || StartTime_ > item.StartTime()) {
                    StartTime_ = item.StartTime();
                }

                if (!EndTime_.GetValue() || EndTime_ < item.EndTime()) {
                    EndTime_ = item.EndTime();
                }

                const ui64 delta = (item.EndTime() - item.StartTime()).MicroSeconds();

                ReqsTime_ += delta;
                ReqsTimeSquares_ += delta * delta;
                DataReaded_ += item.DataLength();
            }
        }

        void Out(IOutputStream& out) override {
            const ui64 full_time = (EndTime_ - StartTime_).MicroSeconds();

            TColumnsFormatter fmt(2);

            const double reqSec = Count_ * 1000000.0 / full_time;
            const double avg = (double)ReqsTime_ / Count_;
            const double stdDev = sqrt((ReqsTimeSquares_ - Count_ * avg * avg) / Count_);
            const double avgLen = (double)DataReaded_ / Count_;

            fmt << "start time " << NextColumn << ToString(StartTime_) << ", " <<
                HRTime(StartTime_) << NextColumn;
            fmt << "end time " << NextColumn << ToString(EndTime_) << ", " <<
                HRTime(EndTime_) << NextColumn;
            fmt << "full time " << NextColumn << ToString(full_time) << ", " <<
                MicrosecToTimeStr(full_time) << NextColumn;
            fmt << "data readed " << NextColumn << ToString(DataReaded_) << NextColumn;
            fmt << "requests " << NextColumn << ToString(Count_) << NextColumn;
            fmt << "error requests " << NextColumn << ToString(ErrCount_) << NextColumn;
            fmt << "requests/sec " << NextColumn << DoubleToString(reqSec, 3) << NextColumn;
            fmt << "avg req. time " << NextColumn << DoubleToString(avg, 3) << NextColumn;
            fmt << "avg req. len " << NextColumn << DoubleToString(avgLen, 3) << NextColumn;
            fmt << "req time std deviation " << NextColumn << DoubleToString(stdDev, 3) << NextColumn;
            fmt << NewLine;

            for (TGroupByCode::const_iterator it = ErrCodes_.begin();
                it != ErrCodes_.end(); ++it) {
                fmt << it->first << " " << NextColumn << ToString(it->second) << NextColumn;
            }

            fmt.Flush(out);
        }

    private:
        TInstant StartTime_;
        TInstant EndTime_;
        size_t Count_ = 0;
        size_t ErrCount_ = 0;
        TGroupByCode ErrCodes_;
        ui64 ReqsTime_ = 0;
        ui64 ReqsTimeSquares_ = 0;
        ui64 DataReaded_ = 0;
};

class THistogram {
    public:
        inline THistogram(const TVector<ui64>& bounds)
            : N_(bounds.size() + 1)
            , Segs_(bounds.size() + 1)
        {
            for (size_t i = 0; i < N_ - 1; ++i) {
                Segs_[i].UpperBound = bounds[i];
            }

            Segs_[N_ - 1].UpperBound = (ui64)-1;
        }

        inline void operator()(const ui64& delta_t) {
            for (size_t i = 0; i < N_; ++i) {
                if (delta_t <= Segs_[i].UpperBound) {
                    ++Segs_[i].Num;
                    ++ReqTotal_;

                    break;
                }
            }
        }

        inline void Results(IOutputStream& out) {
            const double reqtotalD = 100.0 / double(ReqTotal_);
            ui64 slowern = ReqTotal_;
            ui64 lastUpperBound = 0;
            TColumnsFormatter fmt(7);

            for (size_t i = 0; i < N_; ++i) {
                const TSegment& s = Segs_[i];

                fmt << " h[" << ToString(i) << "] " << NextColumn;
                fmt << "(" << ToString(lastUpperBound) << NextColumn << "..";

                if (i < N_ - 1) {
                    fmt << ToString(s.UpperBound);
                }

                fmt << ")" << NextColumn;
                fmt << " = " << ToString(s.Num) << NextColumn;
                fmt << " (" << DoubleToString(double(s.Num) * reqtotalD, 3) << "%)," << NextColumn;
                fmt << " " << ToString(slowern) << NextColumn;
                fmt << " (" << DoubleToString(double(slowern) * reqtotalD, 3) << "%)" << NextColumn;

                lastUpperBound = s.UpperBound;
                slowern -= s.Num;
            }

            fmt.Flush(out);
        }

    private:
        struct TSegment{
            ui64 UpperBound = 0;
            ui64 Num = 0;
        };

        size_t N_ = 0;
        ui64 ReqTotal_ = 0;
        TVector<TSegment> Segs_;
};

template <class TBoundsBuilder>
class TReqTimeStat: public IStat {
    public:
        inline TReqTimeStat(THolder<TBoundsBuilder>& bb)
            : Builder_(bb.Release())
        {
        }

        ~TReqTimeStat() override {
        }

        void Process(const TDevastateStatItem& item) override {
            if (item.ErrCode() == 0) {
                const ui64 delta = (item.EndTime() - item.StartTime()).MicroSeconds();

#if 0
                if (delta > 10000000) {
                    Cerr << item.Url() << Endl;
                }
#endif

                if (delta < Min_) {
                    Min_ = delta;
                }

                if (delta > Max_) {
                    Max_ = delta;
                }

                Deltas_.push_back(delta);
            }
        }

        void Out(IOutputStream& out) override {
            if (Deltas_.empty()) {
                out << "No requests" << Endl;

                return;
            }

            THistogram h((*Builder_)(Min_, Max_, out));

            for (TVector<ui64>::iterator r = Deltas_.begin(); r != Deltas_.end(); ++r) {
                h(*r);
            }

            h.Results(out);
        }

    private:
        THolder<TBoundsBuilder> Builder_;
        TVector<ui64> Deltas_;
        ui64 Min_ = ui64(-1);
        ui64 Max_ = 0;
};

class TLinearAdaptiveBounds {
    public:
        inline TVector<ui64> operator()(const ui64& min, const ui64& max, IOutputStream& out) {
            TVector<ui64> res;

            const size_t n = 20;
            res.reserve(n + 1);

            const ui64 d = max - min;
            const double k = double(d) / double(n);

            for (size_t i = 0; i <= n; ++i) {
                res.push_back(min + ui64(k * double(i)));
            }

            out << "L range(" << min << ", " << max << ") usec" << Endl;

            return res;
        }
};

class TLogarithmicAdaptiveBounds {
    public:
        inline TVector<ui64> operator()(const ui64& min, const ui64& max, IOutputStream& out) {
            TVector<ui64> res;

            const size_t n = 20;
            res.reserve(n + 1);

            const double ming = log(double(min));
            const double maxg = log(double(max));
            const double d = maxg - ming;
            const double k = d / double(n);

            for (size_t i = 0; i <= n; ++i) {
                const ui64 bound = ui64(exp(k * double(i) + ming));

                res.push_back(bound);
            }

            out << "G range(" << ming << ", " << maxg << ") usec" << Endl;

            return res;
        }
};

class TFixedBounds {
    public:
        inline TFixedBounds(const TVector<ui64>* b) {
            if (b) {
                bounds = *b;

                return;
            }

            unsigned interv[] = {
                1,
                2,
                3,
                6,
                10,
                20,
                30,
                40,
                65,
                100,
                200,
                300,
                650,
                1000,
                2000,
                3000,
                6000,
                8200,
                10000,
                11200,
                20000
            };

            const size_t n = Y_ARRAY_SIZE(interv);
            bounds.reserve(n);
            for (size_t i = 0; i < n; ++i) {
                bounds.push_back(interv[i] * 1000);
            }
        }

        inline TVector<ui64> operator()(const ui64& min, const ui64& max, IOutputStream& out) {
            out << "F range(" << min << ", " << max << ") usec" << Endl;

            return bounds;
        }

    private:
        TVector<ui64> bounds;
};

class TRequestTimingsStat: public IStat {
    public:
        inline TRequestTimingsStat()
        {
        }

        ~TRequestTimingsStat() override {
        }

        void Process(const TDevastateStatItem& item) override {
            if (!item.ErrCode()) {
                TTotalTimeAndCount& info = Storage_[item.Url()];

                info.TotalTime_ += (item.EndTime() - item.StartTime()).MicroSeconds();
                info.TotalDataLen_ += item.DataLength();
                info.RequestsCount_++;
            }
        }

        void Out(IOutputStream& out) override {
            for (TStorage::const_iterator it = Storage_.begin(); it != Storage_.end(); it++) {
                const double avg = double(it->second.TotalTime_) / double(it->second.RequestsCount_);
                const double avgLen = double(it->second.TotalDataLen_) / double(it->second.RequestsCount_);
                out << it->first << '\t' << avg << "\t" << avgLen << Endl;
            }
        }

    private:
        struct TTotalTimeAndCount {
            ui64 TotalTime_ = 0;
            ui64 TotalDataLen_ = 0;
            size_t RequestsCount_ = 0;
        };

        typedef THashMap<TString, TTotalTimeAndCount> TStorage;

    private:
        TStorage Storage_;
};

class TRequestTimelineStat: public IStat {
    public:
        inline TRequestTimelineStat()
        {
        }

        ~TRequestTimelineStat() override {
        }

        void Process(const TDevastateStatItem& item) override {
            if (!item.ErrCode()) {
                Storage_.push_back(TTimeline(item.StartTime(), item.EndTime()));
            }
        }

        void Out(IOutputStream& out) override {
            for (TStorage::const_iterator it = Storage_.begin(); it != Storage_.end(); ++it) {
                const TTimeline &info = *it;
                out <<  TDuration(info.EndTime - info.StartTime).MicroSeconds()
                    << '\t' << info.StartTime.MicroSeconds()
                    << '\t' << info.EndTime.MicroSeconds()
                    << Endl;
            }
        }

    private:
        struct TTimeline {
            TInstant StartTime;
            TInstant EndTime;

            TTimeline(const TInstant &startTime, const TInstant &endTime)
                : StartTime(startTime)
                , EndTime(endTime)
            {
            }
        };

        typedef TVector<TTimeline> TStorage;

    private:
        TStorage Storage_;
};

class TRequestResponseStat: public IStat {
    public:
        inline TRequestResponseStat() {
        }

        ~TRequestResponseStat() override {
        }

        void Process(const TDevastateStatItem& item) override {
            if (!item.ErrCode()) {
                Storage_.push_back(item.Data());
            }
        }

        void Out(IOutputStream& out) override {
            for (TStorage::const_iterator it = Storage_.begin(); it != Storage_.end(); ++it) {
                out.Write(it->begin(), it->size());
            }
        }

        typedef TVector<TDevastateStatItem::TData> TStorage;

    private:
        TStorage Storage_;
};

class TResponseParsedHttpBodyStat: public IStat {
    public:
        inline TResponseParsedHttpBodyStat() {
        }

        ~TResponseParsedHttpBodyStat() override {
        }

        void Process(const TDevastateStatItem& item) override {
            if (!item.ErrCode()) {
                THttpParser parser(THttpParser::Response);
                if (parser.Parse(reinterpret_cast<const char*>(item.Data().data()), item.Data().size())) {
                    Storage_.push_back(parser.Content());
                }
            }
        }

        void Out(IOutputStream& out) override {
            for (TStorage::const_iterator it = Storage_.begin(); it != Storage_.end(); ++it) {
                out.Write(it->begin(), it->size());
            }
        }

        typedef TVector<TString> TStorage;

    private:
        TStorage Storage_;
};

struct TResponseTimeQuant : public IStat {
    void Process(const TDevastateStatItem& item) final {
        Timings.emplace_back(item.EndTime() - item.StartTime());
    }

    void Out(IOutputStream& out) final {
        TColumnsFormatter fmt(3);
        fmt << "Resp. time quant." << NextColumn << " " << NextColumn << "Value, seconds" << NextColumn;

        Sort(Timings);
        for (double q : {0.5, 0.9, 0.95, 0.99, 0.999, 0.9999}) {
            double floatCount = q * Timings.size();
            size_t upperIndex = static_cast<size_t>(floatCount);
            if (upperIndex == 0 || upperIndex + 1 >= Timings.size()) {
                fmt << ToString(q) << NextColumn << " " << NextColumn << "N/A" << NextColumn;
                continue;
            }
            double delta = (floatCount - floor(floatCount)) * (Timings[upperIndex].MicroSeconds() - Timings[upperIndex - 1].MicroSeconds());
            double qValue = (Timings[upperIndex - 1].MicroSeconds() + delta) / 1000000;
            fmt << ToString(q) << NextColumn << " " << NextColumn << ToString(qValue) << NextColumn;
        }

        fmt.Flush(out);
    }

private:
    TVector<TDuration> Timings;
};

int main(int argc, char** argv) {
    try {
        THolder<TOpts> opts;

        opts.Reset(new TOpts(argc, argv));

        TStats stats;
        THolder<IInputStream> is(OpenInput(opts->FileName));

        if (opts->OutCommon) {
            stats.Add(new TCommonStats());
        }

        if (opts->OutLongReqs.GetValue()) {
            stats.Add(new TLongReqStats(opts->OutLongReqs));
        }

        if (opts->OutAllReqs) {
            stats.Add(new TAllReq());
        }

        if (opts->OutHistogram) {
            THolder<TLinearAdaptiveBounds> b(new TLinearAdaptiveBounds());

            stats.Add(new TReqTimeStat<TLinearAdaptiveBounds>(b));
        }

        if (opts->OutHistogramLn) {
            THolder<TLogarithmicAdaptiveBounds> b(new TLogarithmicAdaptiveBounds());

            stats.Add(new TReqTimeStat<TLogarithmicAdaptiveBounds>(b));
        }

        if (opts->OutHistogramFixed) {
            TVector<ui64> bounds;
            TVector<ui64>* bounds2 = nullptr;

            if (opts->FixedIntervals != DEFAULT_FIXED_INTERVALS) {
                bounds = ParseBoundsParam(opts->FixedIntervals);
                bounds2 = &bounds;
            }

            THolder<TFixedBounds> b(new TFixedBounds(bounds2));

            stats.Add(new TReqTimeStat<TFixedBounds>(b));
        }

        if (opts->OutRequestTimings) {
            stats.Add(new TRequestTimingsStat());
        }

        if (opts->OutRequestTimeline) {
            stats.Add(new TRequestTimelineStat());
        }

        if (opts->OutResponsesMode == ResponsesMode::Raw) {
            stats.Add(new TRequestResponseStat());
        }

        if (opts->OutResponsesMode == ResponsesMode::ParsedHttpBody) {
            stats.Add(new TResponseParsedHttpBodyStat());
        }

        if (opts->OutResponseTimeQuant) {
            stats.Add(new TResponseTimeQuant());
        }

        ForEachStatItem(is.Get(), stats);

        stats.Out(opts->DumpType.empty() ? Cout : Cerr);

        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }

    return 1;
}
