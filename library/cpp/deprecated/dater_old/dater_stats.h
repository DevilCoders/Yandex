#pragma once

#include "structs.h"

#include <util/memory/pool.h>

#include <util/generic/hash.h>

namespace NDater {
    class TDaterStats : TNonCopyable {
    public:
        enum {
            ST_ZeroYear = 1999,
            ST_MinYear = ST_ZeroYear + 1,
            ST_MaxYear = ST_MinYear + 20,

            ST_HAS_YEAR = 1,
            ST_HAS_MONTH = 2,
            ST_HAS_DAY = 4,
            ST_HAS_ALL = ST_HAS_DAY | ST_HAS_MONTH | ST_HAS_YEAR,
            ST_WORD_MONTH = ST_HAS_MONTH | 8,
            ST_LONG_YEAR = ST_HAS_YEAR | 16,
        };

        struct TDaterStatsCtx;

    public:
        TDaterStats();
        TDaterStats(TDaterStatsCtx*);
        ~TDaterStats();

        bool Empty() const;
        void Clear();

        void Add(const TDaterDate& d);

        // years

        static ui32 DefaultYear() {
            return TDaterDate::ErfZeroYear;
        }

        ui32 MaxYear(TDaterDate::EDateFrom from) const;
        ui32 MaxYear(TDaterDate::EDateFrom from, TDaterDate::EDateMode mode) const;

        ui32 MinYear(TDaterDate::EDateFrom from) const;
        ui32 MinYear(TDaterDate::EDateFrom from, TDaterDate::EDateMode mode) const;

        ui32 CountYears(TDaterDate::EDateFrom from) const;
        ui32 CountYears(TDaterDate::EDateFrom from, TDaterDate::EDateMode mode) const;

        ui32 CountYears(TDaterDate::EDateFrom from, ui32 year) const;
        ui32 CountYears(TDaterDate::EDateFrom from, ui32 year, TDaterDate::EDateMode mode) const;
        ui32 CountYears(TDaterDate::EDateFrom from, ui32 year, ui32 flags, ui32 mask) const;

        ui32 CountUniqYears(TDaterDate::EDateFrom from) const;

        ui8 YearNormLikelihood() const;
        ui8 AverageSourceSegment() const;

        // months years

        ui32 CountMonthsYears(TDaterDate::EDateFrom from, ui32 month, ui32 year) const;
        ui32 CountMonthsYears(TDaterDate::EDateFrom from, ui32 month, ui32 year, ui32 flags, ui32 mask = -1) const;

        // days months

        ui32 CountDaysMonths(TDaterDate::EDateFrom from, ui32 day, ui32 month) const;
        ui32 CountDaysMonths(TDaterDate::EDateFrom from, ui32 day, ui32 month, ui32 flags, ui32 mask = -1) const;

        TString ToStringYears() const;
        TString ToStringMonthsYears() const;
        TString ToStringDaysMonths() const;

        void FromString(TStringBuf s);

        enum EStatItemMode {
            SIM_Years = 0, // legacy
            SIM_MonthsYears = 1,
            SIM_DaysMonths = 2,
        };

        struct TStatKey {
            union {
                ui32 Data;

                struct {
                    union {
                        ui16 Item;

                        struct {
                            ui16 Day : 5;   // 0, 1, .. 31
                            ui16 Month : 4; // 0, 1, .. 12
                            ui16 Year : 5;  // 0 (=ZeroYear), 1 (MinYear - ZeroYear), .. 21 (MaxYear - ZeroYear)
                        };
                    };

                    union {
                        ui16 Key;

                        struct {
                            ui16 LongYear : 1;
                            ui16 WordMonth : 1;
                            ui16 HasNoDay : 1;
                            ui16 HasNoMonth : 1;
                            ui16 HasNoYear : 1;
                            ui16 Source : 8;
                            ui16 StatItemMode : 2;
                        };
                    };
                };
            };

            TStatKey()
                : Data()
            {
            }

            TStatKey(const TDaterDate&, EStatItemMode mode);

            ui32 RealYear() const {
                return Year + ST_ZeroYear;
            }

            void SetRealYear(ui32 y) {
                Year = Max<ui32>(y, ST_ZeroYear) - ST_ZeroYear;
            }

            bool EqualTo(const TStatKey& ref, const TStatKey& mask) const {
                return (Data & mask.Data) == (ref.Data & mask.Data);
            }

            friend bool operator==(const TStatKey& a, const TStatKey& b) {
                return a.Data == b.Data;
            }

            friend bool operator!=(const TStatKey& a, const TStatKey& b) {
                return !(a == b);
            }

            friend bool operator<(const TStatKey& a, const TStatKey& b) {
                return a.Data < b.Data;
            }
        };

        struct TKeyHash {
            size_t operator()(const TStatKey& k) const {
                return NumericHash(k.Data);
            }
        };

        struct TStatItem : TStatKey {
            ui32 Counter;

            TStatItem()
                : Counter()
            {
            }

            TStatItem(const TStatKey& s, ui32 cnt = 1)
                : TStatKey(s)
                , Counter(cnt)
            {
            }
        };

        typedef TVector<TStatItem> TStatItems;
        typedef THashMap<TStatKey, ui32, TKeyHash, TEqualTo<TStatKey>, TPoolAllocator> TStatItemsIdx;

        struct TCounter;
        struct TYearComparer;
        struct TPrinter;
        struct TYearNormLikelihoodCalcer;
        struct TAverageSourceSegmentCalcer;

    public:
        struct TDaterStatsCtx : TNonCopyable {
            TMemoryPool Pool;

            TStatItems Items;
            TStatItemsIdx* Idx;
            ui32 CurrentYear;

            TDaterStatsCtx(time_t tstamp = 0)
                : Pool(8000)
                , Idx()
                , CurrentYear(NDatetime::TSimpleTM::New(tstamp).RealYear())
            {
                Items.reserve(256);
            }

            void Clear() {
                Idx = nullptr;
                Pool.ClearKeepFirstChunk();
                Items.clear();
            }

            void Commit() {
                Idx = nullptr;
                Pool.ClearKeepFirstChunk();
                Sort(Items.begin(), Items.end());
            }
        };

    private:
        THolder<TDaterStatsCtx> CtxHolder;
        TDaterStatsCtx* Ctx;

    public:
        TStatItems GetItems() const {
            if (Ctx == nullptr) {
                return TStatItems();
            }

            return Ctx->Items;
        }

    private:
        void InitCtx();
        void Commit() const;
        void DoAdd(const TDaterDate& d, EStatItemMode mode);

        template <typename TOp>
        void Reduce(TOp&) const;
    };

}
