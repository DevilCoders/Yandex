#pragma once

#include "precalculated_table.h"

#include <kernel/text_machine/interface/hit.h>

#include <library/cpp/vec4/vec4.h>
#include <library/cpp/json/json_value.h>

#include <util/generic/typetraits.h>

namespace NTextMachine {
namespace NCore {
namespace NSeq4f {
    namespace NPrivate {
        struct TIdentityOp {
            template <typename ValueType>
            ValueType operator () (ValueType x) {
                return x;
            }
        };

        struct TPlusOneLogOp {
            float operator()(float x) {
                return log(1.0 + x);
            }

            static TPrecalculatedTable<8192, TPlusOneLogOp> Table;
        };

        struct TValVec {
            float Val;
            TVec4f Vec;

            TValVec(float val)
                : Val(val)
                , Vec(val, val, val, val)
            {}

            template <typename T>
            T Get() const;
        };
        template<>
        inline float TValVec::Get<float>() const {
            return Val;
        }
        template<>
        inline TVec4f TValVec::Get<TVec4f>() const {
            return Vec;
        }

        struct TAddV4Op {
            template <typename T>
            Y_FORCE_INLINE T operator() (T x, T y) const {
                return x + y;
            }
        };

        struct TAddWMulV4Op {
            TValVec W;

            TAddWMulV4Op(float wValue)
                : W(wValue)
            {}
            template <typename T>
            Y_FORCE_INLINE T operator() (T x, T y) const {
                return x + (W.Get<T>() * y);
            }
        };

        struct TIPowV4Op {
            int Power = 0;
            TValVec One{1.0f};

            TIPowV4Op(int power)
                : Power(power)
            {}
            template <typename T>
            Y_FORCE_INLINE T operator() (T x) const {
                static const T one(1.0f);
                int absPower = Power;
                if (Power < 0) {
                    x = one / x;
                    absPower = -Power;
                }
                Y_ASSERT(absPower >= 0);
                T res = one;
                while (absPower >= 2) {
                    if (absPower & 0x1) {
                        res *= x;
                    }
                    x *= x;
                    absPower >>= 1;
                }
                Y_ASSERT(absPower <= 1);
                if (absPower & 0x1) {
                    res *= x;
                }
                return res;
            }
        };

        struct TMinV4Op {
            Y_FORCE_INLINE TVec4f operator() (TVec4f x, TVec4f y) const {
                return TVec4f::Min(x, y);
            }
            Y_FORCE_INLINE float operator() (float x, float y) const {
                return ::Min<float>(x, y);
            }
        };

        struct TMaxV4Op {
            Y_FORCE_INLINE TVec4f operator() (TVec4f x, TVec4f y) const {
                return TVec4f::Max(x, y);
            }
            Y_FORCE_INLINE float operator() (float x, float y) const {
                return ::Max<float>(x, y);
            }
        };

        struct TWMulLogV4Op {
            TValVec W;

            TWMulLogV4Op(float wValue)
                : W(wValue)
            {}
            template <typename T>
            Y_FORCE_INLINE T operator() (T x) const {
                return W.Get<T>() * TPlusOneLogOp::Table.GetValue(x);
            }
        };

        struct TMulConstV4Op {
            TValVec C;

            TMulConstV4Op(float cValue)
                : C(cValue)
            {
            }
            template <typename T>
            Y_FORCE_INLINE T operator() (T x) const {
                return x * C.Get<T>();
            }
        };

        struct TMulV4Op {
            template <typename T>
            Y_FORCE_INLINE T operator() (T x, T y) const {
                return x * y;
            }
        };

        struct TDivV4Op {
            template <typename T>
            Y_FORCE_INLINE T operator() (T x, T y) const {
                return x / y;
            }
        };

        struct TKNormV4Op {
            TValVec K;

            TKNormV4Op(float kValue)
                : K(kValue)
            {
            }
            template <typename T>
            Y_FORCE_INLINE T operator() (T x) const {
                return x / (x + K.Get<T>());
            }
        };

        struct TMulKNormV4Op {
            TValVec K;

            TMulKNormV4Op(float kValue)
                : K(kValue)
            {
            }
            template <typename T>
            Y_FORCE_INLINE T operator() (T x, T y) const {
                return (x / (x + K.Get<T>())) * y;
            }
        };

        struct TKNormPlusV4Op {
            TValVec K, P, MinusP;

            TKNormPlusV4Op(float kValue, float pValue)
                : K(kValue)
                , P(pValue)
                , MinusP(1.0 - pValue)
            {
            }
            template <typename T>
            Y_FORCE_INLINE T operator() (T x) const {
                return P + MinusP * (x / (x + K.Get<T>()));
            }
        };

        struct TMulKNormPlusV4Op {
            TValVec K, P, MinusP;

            TMulKNormPlusV4Op(float kValue, float pValue)
                : K(kValue)
                , P(pValue)
                , MinusP(1.0 - pValue)
            {
            }
            template <typename T>
            Y_FORCE_INLINE T operator() (T x, T y) const {
                return (P + MinusP * (x / (x + K.Get<T>()))) * y;
            }
        };

        struct TLinearMixV4Op {
            TValVec A, MinusA;

            TLinearMixV4Op(float alphaValue)
                : A(alphaValue)
                , MinusA(1.0 - alphaValue) {}
            template <typename T>
            Y_FORCE_INLINE T operator() (T x, T y) const {
                return x * MinusA.Get<T>() + A.Get<T>() * y;
            }
        };

        template <typename SeqType, typename ReturnType, typename OpType, bool Is4f>
        struct TAccumSequenceHelper {
            static ReturnType Do(ReturnType r, SeqType&& x, const OpType& op) {
                for (size_t n = x.Avail(); n; --n) {
                    r = op(r, x.Next());
                }
                return r;
            }
        };
        template <typename SeqType, typename ReturnType, typename OpType>
        struct TAccumSequenceHelper<SeqType, ReturnType, OpType, true> {
            static ReturnType Do(ReturnType r, SeqType&& x, const OpType& op) {
                size_t n = x.Avail();
                if (n >= 8) {
                    TVec4f v = x.Next4f();
                    n -= 4;
                    do {
                        v = op(v, x.Next4f());
                        n -= 4;
                    } while (n >= 4);
                    float buf[4];
                    v.Store(buf);
                    r = op(op(op(op(r, buf[0]), buf[1]), buf[2]), buf[3]);
                }
                for (; n; --n) {
                    r = op(r, x.Next());
                }
                return r;
            }
        };

        template <typename SeqType, typename OutType, bool Is4f, bool IsZeroCopy>
        struct TCopySequenceHelper {
            static void Do(SeqType&& x, OutType out) {
                for (size_t n = x.Avail(); n; --n) {
                    *out++ = x.Next();
                }
            }
        };
        template <typename SeqType, typename OutType, bool Is4f>
        struct TCopySequenceHelper<SeqType, OutType, Is4f, true> {
            static void Do(SeqType&& x, OutType out) {
                Copy(x.Ptr(), x.End(), out);
            }
        };
        template <typename SeqType, bool Is4f>
        struct TCopySequenceHelper<SeqType, std::remove_const_t<typename SeqType::TValue>*, Is4f, true> {

            static void Do(SeqType&& x, std::remove_const_t<typename SeqType::TValue>* out) {
                static_assert(sizeof(*out) == sizeof(*x.Ptr()), "");
                memcpy(out, x.Ptr(), x.Avail() * sizeof(typename SeqType::TValue));
            }
        };
        template <typename SeqType, typename OutType>
        struct TCopySequenceHelper<SeqType, OutType, true, false> {
            static void Do(SeqType&& x, OutType out) {
                size_t n = x.Avail();
                for (; n >= 4; n -= 4) {
                    x.Next4f().Store(out);
                    out += 4;
                }
                for (; n; --n) {
                    *out++ = x.Next();
                }
            }
        };

        template <typename ValueType>
        class TSeq {
        public:
            using TValue = const ValueType;

            enum {
                IsZeroCopy = 1,
                Is4f = 0
            };

        private:
            TValue* CurPtr = nullptr;
            TValue* EndPtr = nullptr;

        public:
            TSeq() = default;
            TSeq(TValue* data, size_t count)
                : CurPtr(data)
                , EndPtr(data + count)
            {}

            size_t Avail() const {
                return EndPtr - CurPtr;
            }
            TValue Next() {
                Y_ASSERT(CurPtr < EndPtr)  ;
                TValue res = *CurPtr;
                CurPtr += 1;
                return res;
            }
            TValue* Ptr() const {
                return CurPtr;
            }
            TValue* End() const {
                return EndPtr;
            }
            void Advance(size_t n) {
                Y_ASSERT(CurPtr + n <= EndPtr);
                CurPtr += n;
            }
        };

        template <typename SeqType, typename OpType>
        class TMappedSeq {
        public:
            using TSequence = SeqType;
            using TOp = OpType;
            using TInValue = typename TSequence::TValue;
            using TOutValue = decltype(TOp()(TInValue()));
            using TValue = TOutValue;

            enum {
                IsZeroCopy = 0,
                Is4f = 0
            };

        private:
            TSequence Seq;
            TOp Op;

        public:
            TMappedSeq() = default;
            TMappedSeq(const TSequence& seq, const TOp& op)
                : Seq(seq)
                , Op(op)
            {}

            size_t Avail() const {
                return Seq.Avail();
            }
            TValue Next() {
                return Op(Seq.Next());
            }
        };

        class TSeq4f
            : public TSeq<float>
        {
        public:
            using TBase = TSeq<float>;

            enum {
                Is4f = 1
            };

        public:
            TSeq4f() = default;
            TSeq4f(TBase::TValue* data, size_t count)
                : TBase(data, count)
            {}

            TVec4f Next4f() {
                Y_ASSERT(TBase::Avail() >= 4);
                TVec4f v;
                v.Load(TBase::Ptr());
                TBase::Advance(4);
                return v;
            }
        };

        template <typename SeqType>
        class TSeq4fAdapter
            : private SeqType
        {
        public:
            using TBase = SeqType;

            enum {
                IsZeroCopy = 0,
                Is4f = 1
            };

        public:
            template <typename... Args>
            TSeq4fAdapter(Args... args)
                : TBase(args...)
            {}

            using TBase::Avail;
            using TBase::Next;

            TVec4f Next4f() {
                Y_ASSERT(TBase::Avail() >= 4);
                float f0 = TBase::Next();
                float f1 = TBase::Next();
                float f2 = TBase::Next();
                float f3 = TBase::Next();
                return TVec4f(f0, f1, f2, f3);
            }
        };

        template <typename SeqTypeX, typename SeqTypeY, typename OpType>
        class TBinaryOpSeq4f {
        public:
            using TSequenceX = SeqTypeX;
            using TSequenceY = SeqTypeY;
            using TOp = OpType;
            using TValue = float;

            enum {
                IsZeroCopy = 0,
                Is4f = 1
            };

            static_assert(TSequenceX::Is4f, "");
            static_assert(TSequenceY::Is4f, "");

        private:
            TSequenceX SeqX;
            TSequenceY SeqY;
            TOp Op;

        public:
            TBinaryOpSeq4f() = default;
            TBinaryOpSeq4f(const TSequenceX& seqX, const TSequenceY& seqY, const TOp& op)
                : SeqX(seqX)
                , SeqY(seqY)
                , Op(op)
            {
                Y_ASSERT(seqX.Avail() == seqY.Avail());
            }
            size_t Avail() const {
                return SeqX.Avail();
            }
            float Next() {
                return Op(SeqX.Next(), SeqY.Next());
            }
            TVec4f Next4f() {
                return Op(SeqX.Next4f(), SeqY.Next4f());
            }
        };

        template <typename SeqTypeX, typename OpType>
        class TUnaryOpSeq4f {
        public:
            using TSequenceX = SeqTypeX;
            using TOp = OpType;
            using TValue = float;

            enum {
                IsZeroCopy = 0,
                Is4f = 1
            };

            static_assert(TSequenceX::Is4f, "");

        private:
            TSequenceX SeqX;
            TOp Op;

        public:
            TUnaryOpSeq4f() = default;
            TUnaryOpSeq4f(const TSequenceX& seqX, const TOp& op)
                : SeqX(seqX)
                , Op(op)
            {
            }
            size_t Avail() const {
                return SeqX.Avail();
            }
            float Next() {
                return Op(SeqX.Next());
            }
            TVec4f Next4f() {
                return Op(SeqX.Next4f());
            }
        };

        class TConstSeq4f {
        public:
            using TValue = float;

            enum {
                IsZeroCopy = 0,
                Is4f = 1
            };

        private:
            float Value = 0.0f;
            size_t Count = 0;

        public:
            TConstSeq4f() = default;
            TConstSeq4f(float value, size_t count)
                : Value(value)
                , Count(count)
            {}
            size_t Avail() const {
                return Count;
            }
            float Next() {
                Y_ASSERT(Count >= 1);
                Count -= 1;
                return Value;
            }
            TVec4f Next4f() {
                Y_ASSERT(Count >= 4);
                Count -= 4;
                return TVec4f(Value, Value, Value, Value);
            }
        };
    } // NPrivate

    template <typename SeqType, typename OutType>
    void Copy(SeqType&& x, OutType out) {
        using TSeq = std::remove_reference_t<SeqType>;
        NPrivate::TCopySequenceHelper<TSeq, OutType, TSeq::Is4f, TSeq::IsZeroCopy>::Do(std::forward<TSeq>(x), out);
    }
    template <typename SeqType, typename ReturnType, typename OpType = NPrivate::TAddV4Op>
    ReturnType Accum(ReturnType r, SeqType&& x, const OpType& op = OpType()) {
        using TSeq = std::remove_reference_t<SeqType>;
        return NPrivate::TAccumSequenceHelper<TSeq, ReturnType, OpType, TSeq::Is4f>::Do(r, std::forward<TSeq>(x), op);
    }

    template <typename SeqType>
    auto Make(const SeqType& x) -> NPrivate::TSeq4fAdapter<SeqType> {
        return NPrivate::TSeq4fAdapter<SeqType>(x);
    }
    template <typename SeqType, typename OpType>
    auto Make(const SeqType& x, const OpType& op) -> NPrivate::TSeq4fAdapter<NPrivate::TMappedSeq<SeqType, OpType>> {
        return Make(NPrivate::TMappedSeq<SeqType, OpType>(x, op));
    }
    template <typename ValueType, typename OpType>
    auto Make(const ValueType* data, size_t count, const OpType& op) -> decltype(Make(NPrivate::TSeq<ValueType>(), op)) {
        return Make(NPrivate::TSeq<ValueType>(data, count), op);
    }
    template <typename SeqTypeX,typename OpType>
    auto MakeOp(const SeqTypeX& x, const OpType& op) -> NPrivate::TUnaryOpSeq4f<SeqTypeX, OpType> {
        return NPrivate::TUnaryOpSeq4f<SeqTypeX, OpType>(x, op);
    }
    template <typename SeqTypeX, typename SeqTypeY, typename OpType>
    auto MakeOp(const SeqTypeX& x, const SeqTypeY& y, const OpType& op) -> NPrivate::TBinaryOpSeq4f<SeqTypeX, SeqTypeY, OpType> {
        return NPrivate::TBinaryOpSeq4f<SeqTypeX, SeqTypeY, OpType>(x, y, op);
    }

    // Basic ops
    //
    template <typename SeqTypeX, typename SeqTypeY>
    auto Add(SeqTypeX&& x, SeqTypeY&& y)
        -> decltype(MakeOp(x, y, NPrivate::TAddV4Op()))
    {
        return MakeOp(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), NPrivate::TAddV4Op());
    }
    template <typename SeqTypeX, typename SeqTypeY>
    auto Add(SeqTypeX&& x, SeqTypeY&& y, float weight)
        -> decltype(MakeOp(x, y, NPrivate::TAddWMulV4Op(weight)))
    {
        return MakeOp(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), NPrivate::TAddWMulV4Op(weight));
    }
    template <typename SeqTypeX>
    auto Mul(SeqTypeX&& x, float weight)
        -> decltype(MakeOp(x, NPrivate::TMulConstV4Op(weight)))
    {
        return MakeOp(std::forward<SeqTypeX>(x), NPrivate::TMulConstV4Op(weight));
    }
    template <typename SeqTypeX, typename SeqTypeY>
    auto Mul(SeqTypeX&& x, SeqTypeY&& y)
        -> decltype(MakeOp(x, y, NPrivate::TMulV4Op()))
    {
        return MakeOp(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), NPrivate::TMulV4Op());
    }
    template <typename SeqTypeX, typename SeqTypeY>
    auto Div(SeqTypeX&& x, SeqTypeY&& y)
        -> decltype(MakeOp(x, y, NPrivate::TDivV4Op()))
    {
        return MakeOp(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), NPrivate::TDivV4Op());
    }
    template <typename SeqTypeX, typename SeqTypeY>
    auto Min(SeqTypeX&& x, SeqTypeY&& y)
        -> decltype(MakeOp(x, y, NPrivate::TMinV4Op()))
    {
        return MakeOp(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), NPrivate::TMinV4Op());
    }
    template <typename SeqTypeX, typename SeqTypeY>
    auto Max(SeqTypeX&& x, SeqTypeY&& y)
        -> decltype(MakeOp(x, y, NPrivate::TMaxV4Op()))
    {
        return MakeOp(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), NPrivate::TMaxV4Op());
    }
    template <typename SeqTypeX>
    auto Pow(SeqTypeX&& x, int power)
        -> decltype(MakeOp(x, NPrivate::TIPowV4Op(power)))
    {
        return MakeOp(std::forward<SeqTypeX>(x), NPrivate::TIPowV4Op(power));
    }

    // TM-specific ops
    //
    template <typename SeqTypeX>
    auto Log(SeqTypeX&& x, float weight)
        -> decltype(MakeOp(x, NPrivate::TWMulLogV4Op(weight)))
    {
        return MakeOp(std::forward<SeqTypeX>(x), NPrivate::TWMulLogV4Op(weight));
    }
    template <typename SeqTypeX>
    auto Norm(SeqTypeX&& x, float k)
        -> decltype(MakeOp(x, NPrivate::TKNormV4Op(k)))
    {
        return MakeOp(std::forward<SeqTypeX>(x), NPrivate::TKNormV4Op(k));
    }
    template <typename SeqTypeX, typename SeqTypeY>
    auto MulNorm(SeqTypeX&& x, SeqTypeY&& y, float k)
        -> decltype(MakeOp(x, y, NPrivate::TMulKNormV4Op(k)))
    {
        return MakeOp(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), NPrivate::TMulKNormV4Op(k));
    }
    template <typename SeqTypeX>
    auto NormPlus(SeqTypeX&& x, float k, float plus)
        -> decltype(MakeOp(x, NPrivate::TKNormPlusV4Op(k, plus)))
    {
        return MakeOp(std::forward<SeqTypeX>(x), NPrivate::TKNormPlusV4Op(k, plus));
    }
    template <typename SeqTypeX, typename SeqTypeY>
    auto MulNormPlus(SeqTypeX&& x, SeqTypeY&& y, float k, float plus)
        -> decltype(MakeOp(x, y, NPrivate::TMulKNormPlusV4Op(k, plus)))
    {
        return MakeOp(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), NPrivate::TMulKNormPlusV4Op(k, plus));
    }

    template <typename SeqTypeX, typename SeqTypeY>
    auto LinearMix(SeqTypeX&& x, SeqTypeY&& y, float alpha) -> NPrivate::TBinaryOpSeq4f<SeqTypeX, SeqTypeY, NPrivate::TLinearMixV4Op> {
        return MakeOp(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), NPrivate::TLinearMixV4Op(alpha));
    }

    template <typename SeqTypeX, typename SeqTypeY>
    float CalcBm15(SeqTypeX&& x, SeqTypeY&& y, float k) {
        return Accum(0.0f, MulNorm(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), k));
    }
    template <typename SeqTypeX, typename SeqTypeY>
    float CalcBm15Max(SeqTypeX&& x, SeqTypeY&& y, float k) {
        return Accum(0.0f, MulNorm(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), k), NPrivate::TMaxV4Op {});
    }
    template <typename SeqTypeX, typename SeqTypeY>
    float CalcCoverageEstimation(SeqTypeX&& occurences, SeqTypeY&& weights, float cutOffValue) {
        return Accum(
            0.0f,
            MakeOp(
                std::forward<SeqTypeY>(weights),
                Min(std::forward<SeqTypeX>(occurences), NPrivate::TConstSeq4f(cutOffValue, occurences.Avail())),
                NPrivate::TMulV4Op {}
            ),
            NPrivate::TAddV4Op {}
        );
    }
    template <typename SeqTypeX>
    float CalcBm15W0(SeqTypeX&& x, float k) {
        const size_t count = x.Avail();
        return Accum(0.0f, Norm(std::forward<SeqTypeX>(x), k)) / float(count);
    }
    template <typename SeqTypeX, typename SeqTypeY>
    float CalcBm15Plus(SeqTypeX&& x, SeqTypeY&& y, float k, float plus) {
        return Accum(0.0f, MulNormPlus(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), k, plus));
    }
    template <typename SeqTypeX>
    float CalcBm15PlusW0(SeqTypeX&& x, float k, float plus) {
        const size_t count = x.Avail();
        return Accum(0.0f, NormPlus(std::forward<SeqTypeX>(x), k, plus)) / float(count);
    }
    template <typename SeqTypeX, typename SeqTypeY>
    float CalcBm15Mix(SeqTypeX&& x, SeqTypeY&& y, float k, float alpha) {
        const size_t count = x.Avail();
        return Accum(0.0f, MulNormMix(std::forward<SeqTypeX>(x), std::forward<SeqTypeY>(y), k, alpha)) / float(count);
    }

    template <typename SeqTypeX>
    NJson::TJsonValue ToJson(SeqTypeX&& x) {
        NJson::TJsonValue value;
        for (size_t i = 0; x.Avail() > 0; ++i) {
            value[i] = x.Next();
        }
        return value;
    }

    using NPrivate::TSeq;
    using NPrivate::TSeq4f;
    using NPrivate::TConstSeq4f;
} // NSeq4f
} // NCore
} // NTextMachine
