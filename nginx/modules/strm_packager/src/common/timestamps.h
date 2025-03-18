#pragma once

#include <nginx/modules/strm_packager/src/common/math.h>

#include <util/generic/yexception.h>
#include <util/string/printf.h>
#include <util/system/types.h>

#include <utility>

namespace NStrm::NPackager {
    inline i64 ConvertTimescaleFloor(i64 ts, i64 fromTimescale, i64 toTimescale) {
        auto tsd = ADiv(ts, fromTimescale);
        i64 result = tsd.quot * toTimescale;
        result += DivFloor(tsd.rem * toTimescale, fromTimescale);
        return result;
    }

    inline i64 ConvertTimescaleCeil(i64 ts, i64 fromTimescale, i64 toTimescale) {
        auto tsd = ADiv(ts, fromTimescale);
        i64 result = tsd.quot * toTimescale;
        result += DivCeil(tsd.rem * toTimescale, fromTimescale);
        return result;
    }

    template <typename T, i64 TimescaleValue>
    class TTimestamp {
    public:
        using TType = T;

        static constexpr i64 Timescale = TimescaleValue;

        TTimestamp()
            : Value(0)
        {
        }

        explicit TTimestamp(const TType value)
            : Value(value)
        {
        }

        explicit TTimestamp(const TType value, const i64 fromTimescale) {
            Value = ConvertTimescaleFloor(value, fromTimescale, Timescale);
        }

        template <i64 TimescaleValue2>
        explicit TTimestamp(const TTimestamp<T, TimescaleValue2>& other)
            : TTimestamp(other.Value, other.Timescale)
        {
        }

        template <typename T2>
        explicit TTimestamp(const TTimestamp<T2, TimescaleValue>& other)
            : Value(other.Value)
        {
        }

        TType ConvertToTimescale(i64 newTimescale) const {
            return ConvertTimescaleFloor(Value, Timescale, newTimescale);
        }

        TType ConvertToTimescaleCeil(i64 newTimescale) const {
            return ConvertTimescaleCeil(Value, Timescale, newTimescale);
        }

        i64 MilliSeconds() const {
            return ConvertTimescaleFloor(Value, Timescale, 1000);
        }

        i64 MilliSecondsCeil() const {
            return ConvertTimescaleCeil(Value, Timescale, 1000);
        }

    public:
        TTimestamp& operator+=(const TTimestamp& a) {
            Value += a.Value;
            return *this;
        }

        friend TTimestamp operator*(const T a, const TTimestamp& b) {
            return TTimestamp(a * b.Value);
        }

        friend TTimestamp operator*(const TTimestamp& b, const T a) {
            return TTimestamp(a * b.Value);
        }

        TTimestamp operator/(const T a) const {
            return TTimestamp(Value / a);
        }

        T operator/(const TTimestamp& a) const {
            return Value / a.Value;
        }

        template <typename T2>
        friend auto operator+(const TTimestamp<T, TimescaleValue>& a, const TTimestamp<T2, TimescaleValue>& b) {
            return TTimestamp<decltype(T(0) + T2(0)), TimescaleValue>(a.Value + b.Value);
        }

        template <typename T2>
        friend TTimestamp operator-(const TTimestamp<T, TimescaleValue>& a, const TTimestamp<T2, TimescaleValue>& b) {
            return TTimestamp<decltype(T(0) - T2(0)), TimescaleValue>(a.Value - b.Value);
        }

        friend bool operator<=(const TTimestamp& a, const TTimestamp& b) {
            return a.Value <= b.Value;
        }

        friend bool operator>=(const TTimestamp& a, const TTimestamp& b) {
            return a.Value >= b.Value;
        }

        friend bool operator<(const TTimestamp& a, const TTimestamp& b) {
            return a.Value < b.Value;
        }

        friend bool operator>(const TTimestamp& a, const TTimestamp& b) {
            return a.Value > b.Value;
        }

        friend bool operator==(const TTimestamp& a, const TTimestamp& b) {
            return a.Value == b.Value;
        }

        friend IOutputStream& operator<<(IOutputStream& stream, const TTimestamp& a) {
            const i64 msa = a.ConvertToTimescale(1000);
            return stream << "[" << Sprintf("%ld.%03ld", msa / 1000, msa % 1000) << " = " << a.Value << "/" << TimescaleValue << "]";
        }

    public:
        TType Value;
    };

    using Ti64TimeMs = TTimestamp<i64, 1000>;
    using Ti64TimeP = TTimestamp<i64, 90000>;
    using Ti32TimeP = TTimestamp<i32, 90000>;

    inline Ti64TimeP Ms2P(const i64 msValue) {
        return Ti64TimeP(Ti64TimeMs(msValue));
    }

    // [Begin; End) interval on PTS timeline, or DTS timeline in kaltura mode, see GetSamplesInterval
    template <typename TsT>
    struct TInterval {
        TsT Begin;
        TsT End;

        template <typename TsT2>
        explicit operator TInterval<TsT2>() const {
            return TInterval<TsT2>{.Begin = TsT2(Begin), .End = TsT2(End)};
        }

        bool operator==(const TInterval& a) const {
            return Begin == a.Begin && End == a.End;
        }

        friend IOutputStream& operator<<(IOutputStream& stream, const TInterval& a) {
            return stream << "[" << a.Begin << "; " << a.End << "]";
        }
    };

    using TIntervalMs = TInterval<Ti64TimeMs>;
    using TIntervalP = TInterval<Ti64TimeP>;

}
