#include "dater_stats.h"

#include <util/generic/yexception.h>
#include <util/generic/ymath.h>
#include <util/string/cast.h>
#include <util/stream/printf.h>
#include <bitset>

namespace NDater {
    TDaterStats::TStatKey::TStatKey(const TDaterDate& d, EStatItemMode mode)
        : Data()
    {
        LongYear = !!d.LongYear;
        WordMonth = !!d.WordPattern;
        HasNoDay = !d.Day;
        HasNoMonth = !d.Month;
        HasNoYear = d.NoYear;
        Source = d.From;

        switch (StatItemMode = mode) {
            case SIM_MonthsYears:
                Month = d.Month;
                [[fallthrough]];
            case SIM_Years:
                SetRealYear(d.Year);
                break;
            case SIM_DaysMonths:
                Day = d.Day;
                Month = d.Month;
                break;
        }
    }

    TDaterStats::TDaterStats()
        : Ctx()
    {
    }

    TDaterStats::TDaterStats(TDaterStatsCtx* c)
        : Ctx(c)
    {
        if (c)
            c->Clear();
    }

    TDaterStats::~TDaterStats() {
        if (Ctx)
            Ctx->Clear();
    }

    bool TDaterStats::Empty() const {
        return !Ctx || Ctx->Items.empty();
    }

    void TDaterStats::Clear() {
        if (!Ctx)
            return;
        Ctx->Clear();
    }

    void TDaterStats::InitCtx() {
        if (!Ctx)
            CtxHolder.Reset(Ctx = new TDaterStatsCtx);
    }

    void TDaterStats::DoAdd(const TDaterDate& d, EStatItemMode mode) {
        TStatKey k(d, mode);
        TStatItemsIdx::insert_ctx c;
        TStatItemsIdx::const_iterator it = Ctx->Idx->find(k, c);

        if (it != Ctx->Idx->end()) {
            Ctx->Items[it->second].Counter += 1;
        } else {
            Ctx->Idx->insert_direct(std::make_pair(k, Ctx->Items.size()), c);
            Ctx->Items.push_back(k);
        }
    }

    template <typename TOp>
    void TDaterStats::Reduce(TOp& op) const {
        if (!Ctx)
            return;

        for (TStatItems::const_iterator it = Ctx->Items.begin(); it != Ctx->Items.end(); ++it)
            op.Process(*it);
    }

    void TDaterStats::Add(const TDaterDate& d) {
        if (!d.Valid())
            return;

        InitCtx();

        if (!Ctx->Idx) {
            TStatItemsIdx* ptr = Ctx->Pool.Allocate<TStatItemsIdx>();
            Ctx->Idx = new (ptr) TStatItemsIdx(&Ctx->Pool);
        }

        if (d.Year && d.Year >= (ui32)ST_MinYear && d.Year <= (ui32)ST_MaxYear) {
            DoAdd(d, SIM_Years);
            DoAdd(d, SIM_MonthsYears);
        }

        if (d.Month)
            DoAdd(d, SIM_DaysMonths);
    }

    void TDaterStats::Commit() const {
        if (!Ctx)
            return;

        Ctx->Commit();
    }

    struct TDaterStats::TYearComparer {
    private:
        ui16 MaxYear;
        ui16 MinYear;

    public:
        TStatKey Ref;
        TStatKey Mask;

        TYearComparer()
            : MaxYear()
            , MinYear(ST_MaxYear - ST_ZeroYear)
        {
        }

        void Process(const TStatItem& t) {
            if (SIM_Years != t.StatItemMode)
                return;

            if (t.EqualTo(Ref, Mask)) {
                MaxYear = Max(MaxYear, t.Year);
                MinYear = Min(MinYear, t.Year);
            }
        }

        ui16 GetMinYear() const {
            if (!MinYear || MinYear == ST_MaxYear - ST_ZeroYear)
                return TDaterDate::ErfZeroYear;
            return MinYear + ST_ZeroYear;
        }

        ui16 GetMaxYear() const {
            if (!MaxYear)
                return TDaterDate::ErfZeroYear;
            return MaxYear + ST_ZeroYear;
        }
    };

    struct TDaterStats::TCounter {
        ui32 Count;

        std::bitset<(1 << 14)> Uniqs;

        TStatKey Ref;
        TStatKey Mask;

        TCounter(EStatItemMode mode)
            : Count()
        {
            Ref.StatItemMode = mode;
            Mask.StatItemMode = -1;
        }

        void Process(const TStatItem& t) {
            if (t.EqualTo(Ref, Mask) && t.Item && t.Item < Uniqs.size()) {
                Count += t.Counter;
                Uniqs.set(t.Item);
            }
        }
    };

    struct TDaterStats::TPrinter {
        EStatItemMode Mode;
        TStringStream Out;

        TStatKey Last;
        TStatKey Mask;

        TPrinter(EStatItemMode m)
            : Mode(m)
        {
            Mask.Data = -1;
            Mask.Item = 0;
        }

        void PrintHeader(const TStatKey& t) {
            Out << TDaterDate::EncodeFrom(t.Source, false) << '.';
            Out << (t.HasNoDay ? '_' : 'D');
            Out << (t.HasNoMonth ? '_' : 'M');
            Out << (t.HasNoYear ? '_' : 'Y');
            Out << (t.WordMonth ? (t.LongYear ? 'W' : 'w') : (t.LongYear ? 'D' : 'd'));
            Out << ": ";
        }

        void Process(const TStatItem& t) {
            if (Mode != t.StatItemMode)
                return;

            if (!t.EqualTo(Last, Mask)) {
                Finalize();
                PrintHeader(t);
            } else {
                if (!Out.Empty())
                    Out << ", ";
            }

            switch (Mode) {
                case SIM_Years:
                    Printf(Out, "%04d=%d", t.RealYear(), t.Counter);
                    break;
                case SIM_MonthsYears:
                    Printf(Out, "%02d.%04d=%d", t.Month, t.RealYear(), t.Counter);
                    break;
                case SIM_DaysMonths:
                    Printf(Out, "%02d.%02d=%d", t.Day, t.Month, t.Counter);
                    break;
            }

            Last = t;
        }

        void Finalize() {
            if (!Out.Empty())
                Out << "; ";
        }
    };

    struct TDaterStats::TYearNormLikelihoodCalcer {
    private:
        double Count;
        double Mean;
        double M2;
        double CountUniq;
        double MeanUniq;
        double M2Uniq;

        static void ProcessImpl(ui16 value, double* count, double* mean, double* m2, ui32 repeat) {
            for (; repeat > 0; --repeat) {
                ++(*count);
                double delta = value - *mean;
                *mean += delta / *count;
                *m2 += delta * (value - *mean);
            }
        }

    public:
        TYearNormLikelihoodCalcer()
            : Count(0)
            , Mean(0)
            , M2(0)
            , CountUniq(0)
            , MeanUniq(0)
            , M2Uniq(0)
        {
        }

        void Process(const TStatItem& item) {
            if (item.Item && item.StatItemMode == SIM_Years) {
                ProcessImpl(item.Year, &Count, &Mean, &M2, item.Counter);
                ProcessImpl(item.Year, &CountUniq, &MeanUniq, &M2Uniq, 1);
            }
        }

        double Likelihood() {
            if (Count > 1) {
                double variance = M2 / (Count - 1);
                return 0.5 * Count * log(2 * PI * variance) + M2Uniq / (2 * variance);
            } else {
                return -2 * Count;
            }
        }

        double NormalizedLikelihood() {
            return Count ? Likelihood() / Count : -2;
        }
    };

    struct TDaterStats::TAverageSourceSegmentCalcer {
    private:
        ui32 Count;
        double Sum;

    public:
        TAverageSourceSegmentCalcer()
            : Count(0)
            , Sum(0)
        {
        }

        void Process(const TStatItem& item) {
            if (item.Item && item.StatItemMode == SIM_Years) {
                Count += item.Counter;
                Sum += item.Counter * item.Source;
            }
        }

        double AverageSourceSegment() {
            return Count > 0 ? Sum / (Count * (TDaterDate::FromsCount - 1)) : 0;
        }
    };

    template <typename TOp>
    static void SetSource(TOp& op, TDaterDate::EDateFrom from) {
        op.Ref.Source = from;
        op.Mask.Source = -1;
    }

    template <typename TOp>
    static void SetYear(TOp& op, ui32 year) {
        op.Ref.SetRealYear(year);
        op.Mask.Year = -1;
    }

    template <typename TOp>
    static void SetDay(TOp& op, ui32 day) {
        op.Ref.Day = day;
        op.Mask.Day = -1;
    }

    template <typename TOp>
    static void SetMonth(TOp& op, ui32 month) {
        op.Ref.Month = month;
        op.Mask.Month = -1;
    }

    template <ui32 flag>
    inline bool IsSet(ui32 flags) {
        return (flags & flag) == flag;
    }

    template <typename TOp>
    static void SetFlags(TOp& op, ui32 flags, ui32 mask) {
        op.Ref.HasNoYear = !IsSet<TDaterStats::ST_HAS_YEAR>(flags);
        op.Ref.HasNoMonth = !IsSet<TDaterStats::ST_HAS_MONTH>(flags);
        op.Ref.HasNoDay = !IsSet<TDaterStats::ST_HAS_DAY>(flags);
        op.Ref.WordMonth = IsSet<TDaterStats::ST_WORD_MONTH>(flags);
        op.Ref.LongYear = IsSet<TDaterStats::ST_LONG_YEAR>(flags);

        op.Mask.HasNoYear = IsSet<TDaterStats::ST_HAS_YEAR>(mask) ? Max<ui16>() : 0;
        op.Mask.HasNoMonth = IsSet<TDaterStats::ST_HAS_MONTH>(mask) ? Max<ui16>() : 0;
        op.Mask.HasNoDay = IsSet<TDaterStats::ST_HAS_DAY>(mask) ? Max<ui16>() : 0;
        op.Mask.WordMonth = IsSet<TDaterStats::ST_WORD_MONTH>(mask) ? Max<ui16>() : 0;
        op.Mask.LongYear = IsSet<TDaterStats::ST_LONG_YEAR>(mask) ? Max<ui16>() : 0;
    }

    template <typename TOp>
    static void SetMode(TOp& op, TDaterDate::EDateMode mode) {
        switch (mode) {
            default:
                ythrow yexception() << "invalid mode: " << (int)mode;
                break;
            case TDaterDate::ModeNoMonth:
                SetFlags(op, TDaterStats::ST_HAS_YEAR, TDaterStats::ST_HAS_ALL);
                break;
            case TDaterDate::ModeNoDay:
                SetFlags(op, TDaterStats::ST_HAS_YEAR | TDaterStats::ST_HAS_MONTH, TDaterStats::ST_HAS_ALL);
                break;
            case TDaterDate::ModeFull:
                SetFlags(op, TDaterStats::ST_HAS_ALL, TDaterStats::ST_HAS_ALL);
                break;
            case TDaterDate::ModeFullNoYear:
                SetFlags(op, TDaterStats::ST_HAS_DAY | TDaterStats::ST_HAS_MONTH, TDaterStats::ST_HAS_ALL);
                break;
        }
    }

    ui32 TDaterStats::MaxYear(TDaterDate::EDateFrom from) const {
        TYearComparer y;
        SetSource(y, from);
        Reduce(y);
        return y.GetMaxYear();
    }

    ui32 TDaterStats::MinYear(TDaterDate::EDateFrom from) const {
        TYearComparer y;
        SetSource(y, from);
        Reduce(y);
        return y.GetMinYear();
    }

    ui32 TDaterStats::MaxYear(TDaterDate::EDateFrom from, TDaterDate::EDateMode mode) const {
        TYearComparer y;
        SetSource(y, from);
        SetMode(y, mode);
        Reduce(y);
        return y.GetMaxYear();
    }

    ui32 TDaterStats::MinYear(TDaterDate::EDateFrom from, TDaterDate::EDateMode mode) const {
        TYearComparer y;
        SetSource(y, from);
        SetMode(y, mode);
        Reduce(y);
        return y.GetMinYear();
    }

    ui32 TDaterStats::CountYears(TDaterDate::EDateFrom from) const {
        TCounter y(SIM_Years);
        SetSource(y, from);
        Reduce(y);
        return y.Count;
    }

    ui32 TDaterStats::CountYears(TDaterDate::EDateFrom from, ui32 year) const {
        return CountYears(from, year, 0, 0);
    }

    ui32 TDaterStats::CountYears(TDaterDate::EDateFrom from, ui32 year, TDaterDate::EDateMode mode) const {
        TCounter y(SIM_Years);
        SetSource(y, from);
        SetYear(y, year);
        SetMode(y, mode);
        Reduce(y);
        return y.Count;
    }

    ui32 TDaterStats::CountYears(TDaterDate::EDateFrom from, ui32 year, ui32 flags, ui32 mask) const {
        TCounter y(SIM_Years);
        SetSource(y, from);
        SetYear(y, year);
        SetFlags(y, flags, mask);
        Reduce(y);
        return y.Count;
    }

    ui32 TDaterStats::CountYears(TDaterDate::EDateFrom from, TDaterDate::EDateMode mode) const {
        TCounter y(SIM_Years);
        SetSource(y, from);
        SetMode(y, mode);
        Reduce(y);
        return y.Count;
    }

    ui32 TDaterStats::CountUniqYears(TDaterDate::EDateFrom from) const {
        TCounter y(SIM_Years);
        SetSource(y, from);
        Reduce(y);
        return y.Uniqs.count();
    }

    ui8 TDaterStats::YearNormLikelihood() const {
        TYearNormLikelihoodCalcer likelihood;
        Reduce(likelihood);
        ui8 mapped_likelihood = Min(6.0, Max(0.0, likelihood.NormalizedLikelihood() + 2)) / 6 * Max<ui8>();
        return mapped_likelihood;
    }

    ui8 TDaterStats::AverageSourceSegment() const {
        TAverageSourceSegmentCalcer calcer;
        Reduce(calcer);
        ui8 mapped_average = calcer.AverageSourceSegment() * Max<ui8>();
        return mapped_average;
    }

    ui32 TDaterStats::CountMonthsYears(TDaterDate::EDateFrom from, ui32 month, ui32 year) const {
        return CountMonthsYears(from, month, year, 0, 0);
    }

    ui32 TDaterStats::CountMonthsYears(TDaterDate::EDateFrom from, ui32 month, ui32 year, ui32 flags, ui32 mask) const {
        TCounter y(SIM_MonthsYears);
        SetSource(y, from);
        SetYear(y, year);
        SetMonth(y, month);
        SetFlags(y, flags, mask);
        Reduce(y);
        return y.Count;
    }

    ui32 TDaterStats::CountDaysMonths(TDaterDate::EDateFrom from, ui32 day, ui32 month) const {
        return CountDaysMonths(from, day, month, 0, 0);
    }

    ui32 TDaterStats::CountDaysMonths(TDaterDate::EDateFrom from, ui32 day, ui32 month, ui32 flags, ui32 mask) const {
        TCounter y(SIM_DaysMonths);
        SetSource(y, from);
        SetDay(y, day);
        SetMonth(y, month);
        SetFlags(y, flags, mask);
        Reduce(y);
        return y.Count;
    }

    TString TDaterStats::ToStringYears() const {
        Commit();
        TPrinter y(SIM_Years);
        Reduce(y);
        y.Finalize();
        return y.Out.Str();
    }

    TString TDaterStats::ToStringMonthsYears() const {
        Commit();
        TPrinter y(SIM_MonthsYears);
        Reduce(y);
        y.Finalize();
        return y.Out.Str();
    }

    TString TDaterStats::ToStringDaysMonths() const {
        Commit();
        TPrinter y(SIM_DaysMonths);
        Reduce(y);
        y.Finalize();
        return y.Out.Str();
    }

    static void Strip(TStringBuf& s) {
        while (s.StartsWith(' '))
            s.Skip(1);

        while (s.EndsWith(' '))
            s.Chop(1);
    }

    void TDaterStats::FromString(TStringBuf s) {
        InitCtx();

        while (!!s) {
            TStringBuf nam = s.NextTok(';');
            TStringBuf val = nam.SplitOff(':');

            Strip(nam);

            TStatKey k;
            k.Source = TDaterDate::DecodeFrom(!nam ? '?' : nam[0]);

            {
                TStringBuf mod = nam.SplitOff('.');
                k.HasNoDay = k.HasNoMonth = k.HasNoYear = 1;
                for (ui32 i = 0; i < 3 && !!mod; ++i) {
                    switch (mod[0]) {
                        default:
                            break;
                        case '_':
                            break;
                        case 'D':
                            k.HasNoDay = 0;
                            break;
                        case 'M':
                            k.HasNoMonth = 0;
                            break;
                        case 'Y':
                            k.HasNoYear = 0;
                            break;
                    }
                    mod.Skip(1);
                }

                if (!!mod) {
                    switch (mod[0]) {
                        default:
                            break;
                        case 'w':
                            k.LongYear = 0;
                            k.WordMonth = 1;
                            break;
                        case 'W':
                            k.LongYear = 1;
                            k.WordMonth = 1;
                            break;
                        case 'd':
                            k.LongYear = 0;
                            k.WordMonth = 0;
                            break;
                        case 'D':
                            k.LongYear = 1;
                            k.WordMonth = 0;
                            break;
                    }
                }
            }

            {
                while (!!val) {
                    TStatItem kk(k, 0);
                    TStringBuf cnt = val.NextTok(',');
                    TStringBuf bbb = cnt.NextTok('=');
                    TStringBuf aaa = bbb.NextTok('.');

                    Strip(cnt);
                    Strip(aaa);
                    Strip(bbb);

                    try {
                        if (aaa.size() == 4 && !bbb) {
                            kk.SetRealYear(::FromString<ui16>(aaa));
                            kk.StatItemMode = SIM_Years;
                        } else if (aaa.size() <= 2 && bbb.size() == 4) {
                            kk.SetRealYear(::FromString<ui16>(bbb));
                            kk.Month = ::FromString<ui16>(aaa);
                            kk.StatItemMode = SIM_MonthsYears;

                            if (kk.Month > 12)
                                continue;
                        } else if (aaa.size() <= 2 && bbb.size() <= 2) {
                            kk.Day = ::FromString<ui16>(aaa);
                            kk.Month = ::FromString<ui16>(bbb);
                            kk.StatItemMode = SIM_DaysMonths;

                            if (kk.Day > 31 || kk.Month > 12)
                                continue;
                        } else
                            continue;

                        kk.Counter = ::FromString<ui32>(cnt);
                        Ctx->Items.push_back(kk);
                    } catch (const yexception&) {
                    }
                }
            }
        }
    }

}
