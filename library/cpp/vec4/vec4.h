#pragma once

#include <util/generic/string.h>
#include <util/stream/str.h>
#include <util/ysaveload.h>
#include <array>

#include <library/cpp/sse/sse.h>

template <typename T>
class TVec4;
class TVec4f;

class TVec4f {
public:
    using TOverlyingType = float;
    TVec4f() {
        Value_ = _mm_setzero_ps();
    }
    TVec4f(__m128 v) {
        Value_ = v;
    }
    TVec4f(float v) {
        Value_ = _mm_set1_ps(v);
    }
    TVec4f(float v0, float v1, float v2, float v3) {
        Value_ = _mm_set_ps(v3, v2, v1, v0);
    }
    TVec4f(const float* data) {
        Value_ = _mm_loadu_ps(data);
    }
    void Store(float* data) const {
        _mm_storeu_ps(data, Value_);
    }
    void Load(const float* ptr) {
        Value_ = _mm_loadu_ps(ptr);
    }
    TVec4f operator+(const TVec4f& a) const {
        return TVec4f(_mm_add_ps(Value_, a.Value_));
    }
    TVec4f operator-(const TVec4f& a) const {
        return TVec4f(_mm_sub_ps(Value_, a.Value_));
    }
    TVec4f operator*(const TVec4f& a) const {
        return TVec4f(_mm_mul_ps(Value_, a.Value_));
    }
    TVec4f& operator*=(const TVec4f& a) {
        Value_ = _mm_mul_ps(Value_, a.Value_);
        return *this;
    }
    TVec4f& operator+=(const TVec4f& a) {
        Value_ = _mm_add_ps(Value_, a.Value_);
        return *this;
    }
    TVec4f operator/(const TVec4f& a) const {
        return TVec4f(_mm_div_ps(Value_, a.Value_));
    }
    TVec4f operator==(const TVec4f& a) const {
        return TVec4f(_mm_cmpeq_ps(Value_, a.Value_));
    }
    TVec4f operator>(const TVec4f& a) const {
        return TVec4f(_mm_cmpgt_ps(Value_, a.Value_));
    }
    static TVec4f Max(const TVec4f& a, const TVec4f& b) {
        return TVec4f(_mm_max_ps(a.Value_, b.Value_));
    }
    static TVec4f Min(const TVec4f& a, const TVec4f& b) {
        return TVec4f(_mm_min_ps(a.Value_, b.Value_));
    }
    TVec4f operator&(const TVec4f& a) const {
        return TVec4f(_mm_and_ps(Value_, a.Value_));
    }

    TVec4f FastInvSqrt() const {
        return TVec4f(_mm_rsqrt_ps(Value_));
    }

    static void Transpose(TVec4f& a, TVec4f& b, TVec4f& c, TVec4f& d) {
        _MM_TRANSPOSE4_PS(a.Value_, b.Value_, c.Value_, d.Value_);
    }

    // TODO: remove
    TString ToString() const {
        float vec[4];
        Store(vec);
        TStringStream os;
        os << vec[0] << " " << vec[1] << " " << vec[2] << " " << vec[3];
        return os.Str();
    }

    inline std::array<float, 4> GetValues() const {
        std::array<float, 4> res;
        Store(res.begin());
        return res;
    }

    inline void Save(IOutputStream* s) const {
        float data[4];
        Store(data);
        ::SaveArray(s, data, 4);
    }

    inline void Load(IInputStream* s) {
        float data[4];
        ::LoadArray(s, data, 4);
        Load(data);
    }

    TVec4<int> Truncate() const;
    TVec4<int> NearbyInt() const;
    TVec4<int> AsInt() const;

private:
    __m128 Value_;
};

template <typename T>
class TVec4 {
    static_assert(sizeof(T) == 4, "TVec4 type uses 128bit type as value storage, T should be 32bit");

public:
    using TOverlyingType = T;

    TVec4(__m128i v) {
        Values = v;
    }

    TVec4() {
        Values = _mm_setzero_si128();
    }

    TVec4(T v) {
        Values = _mm_set1_epi32(v);
    }

    TVec4(T v0, T v1, T v2, T v3) {
        Values = _mm_set_epi32(v3, v2, v1, v0);
    }

    TVec4(const void* data) {
        Load(data);
    }

    void Load(const void* ptr) {
        Values = _mm_loadu_si128(static_cast<const __m128i*>(ptr));
    }

    void Store(void* data) const {
        _mm_storeu_si128(static_cast<__m128i*>(data), Values);
    }

    // TODO: remove
    TString ToString() const {
        ui32 vec[4];
        Store(vec);
        TStringStream os;
        os << vec[0] << " " << vec[1] << " " << vec[2] << " " << vec[3];
        return os.Str();
    }

    bool HasMaskInt(__m128i mask) const {
        __m128i cmp = _mm_cmpeq_epi32(mask, _mm_or_si128(mask, Values));
        int cmpMask = _mm_movemask_epi8(cmp);
        return cmpMask == 0xffff;
    }

    bool HasMask(T val) const {
        __m128i mask = _mm_set1_epi32(val);
        return HasMaskInt(mask);
    }

    bool HasMask(const TVec4<T>& other) const {
        __m128i mask = other.Values;
        return HasMaskInt(mask);
    }

    template <size_t i0, size_t i1, size_t i2, size_t i3>
    TVec4<T> Shuffle() const {
        return _mm_shuffle_epi32(Values, _MM_SHUFFLE(i0, i1, i2, i3));
    }

    template <size_t Num>
    ui32 Value() const {
        if (Num == 0)
            return _mm_cvtsi128_si32(Values);
        else
            return _mm_cvtsi128_si32(_mm_srli_si128(Values, Num * 4));
    }

    template <size_t Num>
    TVec4<T> RightShift() const {
        return _mm_srli_si128(Values, Num * 4);
    }

    template <size_t Num>
    TVec4<T> LeftShift() const {
        return static_cast<__m128i>(_mm_slli_si128(Values, Num * 4));
    }

    __m128i V128() const {
        return Values;
    }

    bool operator==(const TVec4<T>& other) const {
        __m128i cmp = _mm_cmpeq_epi32(Values, other.Values);
        return _mm_movemask_epi8(cmp) == 0xffff;
    }

    bool operator!=(const TVec4<T>& other) const {
        return !(*this == other);
    }

    TVec4<T> operator&(const TVec4<T>& other) const {
        return TVec4<T>(_mm_and_si128(Values, other.Values));
    }

    TVec4<T>& operator&=(const TVec4<T>& other) {
        return *this = *this & other;
    }

    TVec4<T> operator|(const TVec4<T>& other) const {
        return TVec4<T>(_mm_or_si128(Values, other.Values));
    }

    TVec4<T>& operator|=(const TVec4<T>& other) const {
        return *this = *this | other;
    }

    TVec4<T> operator+(const TVec4<T>& other) const {
        return TVec4<T>(_mm_add_epi32(Values, other.Values));
    }

    TVec4<T>& operator+=(const TVec4<T>& other) const {
        return *this = *this + other;
    }

    TVec4<T> operator-(const TVec4<T>& other) const {
        return TVec4<T>(_mm_sub_epi32(Values, other.Values));
    }

    TVec4<T>& operator-=(const TVec4<T>& other) const {
        return *this = *this - other;
    }

    // TODO: rename
    TVec4<T> operator>(const TVec4<T>& other) const {
        return TVec4<T>(_mm_cmpgt_epi32(Values, other.Values));
    }

    TVec4<T> operator>>(size_t shift) const {
        return TVec4<T>(_mm_srli_epi32(Values, int(shift)));
    }

    TVec4<T> operator<<(size_t shift) const {
        return TVec4<T>(_mm_slli_epi32(Values, int(shift)));
    }

    TVec4<T> CmpMask(const TVec4<T>& a) const {
        return TVec4<T>(_mm_cmpeq_epi32(Values, a.Values));
    }

    TVec4f Y_FORCE_INLINE AsFloat() const {
        return TVec4f(_mm_castsi128_ps(Values));
    }

    TVec4f Y_FORCE_INLINE ToFloat() const {
        return TVec4f(_mm_cvtepi32_ps(Values));
    }

    inline void Save(IOutputStream* s) const {
        T data[4];
        Store(data);
        ::SaveArray(s, data, 4);
    }

    inline void Load(IInputStream* s) {
        T data[4];
        ::LoadArray(s, data, 4);
        Load(data);
    }

private:
    __m128i Values;
};

using TVec4i = TVec4<int>;
using TVec4u = TVec4<ui32>;

TVec4i Y_FORCE_INLINE TVec4f::NearbyInt() const {
    return TVec4i(_mm_cvtps_epi32(Value_));
}
TVec4i Y_FORCE_INLINE TVec4f::Truncate() const {
    return TVec4i(_mm_cvttps_epi32(Value_));
}
TVec4i Y_FORCE_INLINE TVec4f::AsInt() const {
    return TVec4i(_mm_castps_si128(Value_));
}

void ValidateSSE2();

inline TVec4u Pack(TVec4u a, TVec4u b, TVec4u c, TVec4u d) {
    TVec4u ab = static_cast<__m128i>(_mm_packs_epi32(a.V128(), b.V128()));
    TVec4u cd = static_cast<__m128i>(_mm_packs_epi32(c.V128(), d.V128()));
    return TVec4u(_mm_packus_epi16(ab.V128(), cd.V128()));
}

template <class T>
static IOutputStream& PrettyPrintVec(IOutputStream& out, const T& vec) {
    typename T::TOverlyingType values[4];
    vec.Store(values);
    out << "{";
    for (unsigned int i = 0; i < 4; ++i) {
        out << values[i];
        if (i < 3) {
            out << ", ";
        }
    }
    out << "}";
    return out;
}

template <class T>
IOutputStream& operator<<(IOutputStream& out, const TVec4<T>& vec) {
    return PrettyPrintVec(out, vec);
}

inline IOutputStream& operator<<(IOutputStream& out, const TVec4f& vec) {
    return PrettyPrintVec(out, vec);
}
