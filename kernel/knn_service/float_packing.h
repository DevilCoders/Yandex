#pragma once

#include <util/system/defaults.h>
#include <util/generic/ylimits.h>
#include <util/generic/array_ref.h>
#include <util/str_stl.h>
#include <cmath>

namespace NKnnService {
    struct FloatPacking {
        template<class T>
        Y_FORCE_INLINE static T Pack(float x) {
            static_assert(-Min<T>() > 0, "signed only");
            int res = std::lround(x * (Max<T>()));
            Y_ASSERT(int(T(res)) == res);
            return res;
        }

        template<class T>
        Y_FORCE_INLINE static float UnPack(T x) {
            static_assert(-Min<T>() > 0, "signed only");
            return x / float(Max<T>());
        }

        template<class T>
        Y_FORCE_INLINE static T Pack(i16 x) {
            static_assert(-Min<T>() > 0, "signed only");
            static_assert(sizeof(T) == 1, "only i8");
            return Pack<T>(UnPack(x));
        }

        template<class T>
        Y_FORCE_INLINE static float UnPackSq(float x) {
            static_assert(-Min<T>() > 0, "signed only");
            return x / float(Max<T>() * Max<T>());
        }
    };

    struct TUnpackable16DotProduct {
        using TResult = float;
        using TLess = ::TGreater<float>;

        static float DotProductUnpackable16Alloca(const i16* l, const i16* r, int length);
        static float DotProductUnpackable16Heap(const i16* l, const i16* r, int length);

        float operator()(const i16* l, const i16* r, int length) const {
            if (Y_LIKELY(length < 10000)) {
                return DotProductUnpackable16Alloca(l, r, length);
            } else {
                return DotProductUnpackable16Heap(l, r, length);
            }
        }
    };

    TString PackEmbedsBase64(TArrayRef<const float> emb);
    TVector<float> UnpackEmbedsBase64(TStringBuf packed);
    TString PackEmbedsAsInt8Base64(TArrayRef<const float> emb);
    TVector<float> UnpackEmbedsAsInt8Base64(TStringBuf packed);

    i64 DistanceToInteger(float dist);
    float IntegerToDistance(i64 dist);

    template<typename T>
    T PackMinMax(float val, float min = 0.0, float max = 1.0) {
        if (min == max) return Min<T>();
        double res = (val - min) * 1.0 / (max - min);
        if (val > max) return Max<T>();
        if (val < min) return Min<T>();
        return static_cast<T>(res * Max<T>());
    }

    template<typename T>
    float UnpackMinMax(T val, float min = 0.0, float max = 1.0) {
        return static_cast<float>(val) * (max - min) / Max<T>() + min;
    }
}
