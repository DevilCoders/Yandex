#pragma once

#include <library/cpp/packedtypes/packedfloat.h>
#include <util/system/types.h>
#include <util/stream/output.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/array_ref.h>
#include <util/generic/ymath.h>

namespace NClickSim {

#pragma pack(1)
    struct THashWithWeight {
        ui32 Hash;
        ui8 Weight;

        THashWithWeight(ui32 hash, ui8 weight)
                : Hash(hash)
                , Weight(weight)
        {
        }

        THashWithWeight() = default;

        bool operator < (const THashWithWeight& other) const {
            return Hash < other.Hash;
        }
    };
#pragma pack()
    static_assert(sizeof(THashWithWeight) == sizeof(ui32) + sizeof(ui8), "sizeof(THashWithWeight)");

    // LeftJoin - callback only events where left side is not null.
    template <bool LeftJoin, class TCallback>
    ui64 DotProductWithCallback(const THashWithWeight* it1, const THashWithWeight* end1,
                                const THashWithWeight* it2, const THashWithWeight* end2, TCallback& cb)
    {
        if (Y_UNLIKELY(it1 == end1)) {
            if constexpr (!LeftJoin) {
                std::for_each(it2, end2, [&cb](const THashWithWeight &w) { cb(nullptr, &w); });
            }
            return 0;
        }

        if (Y_UNLIKELY(it2 == end2)) {
            std::for_each(it1, end1, [&cb](const THashWithWeight& w) { cb(&w, nullptr);});
            return 0;
        }

        ui64 result = 0;
        for (;;) {
            if (it1->Hash == it2->Hash) {
                result += static_cast<ui32>(it1->Weight) * it2->Weight;
                cb(it1, it2);
                ++it1;
                ++it2;
                if (it1 == end1) {
                    if constexpr (!LeftJoin) {
                        std::for_each(it2, end2, [&cb](const THashWithWeight &w) { cb(nullptr, &w); });
                    }
                    return result;
                }
                if (it2 == end2) {
                    std::for_each(it1, end1, [&cb](const THashWithWeight& w) { cb(&w, nullptr);});
                    return result;
                }
            } else if (it1->Hash < it2->Hash) {
                cb(it1, nullptr);
                ++it1;
                if (it1 == end1) {
                    if constexpr (!LeftJoin) {
                        std::for_each(it2, end2, [&cb](const THashWithWeight &w) { cb(nullptr, &w); });
                    }
                    return result;
                }
            } else {
                Y_ASSERT(it1->Hash > it2->Hash);
                if constexpr (!LeftJoin) {
                    cb(nullptr, it2);
                }
                ++it2;
                if (it2 == end2) {
                    std::for_each(it1, end1, [&cb](const THashWithWeight& w) { cb(&w, nullptr);});
                    return result;
                }
            }
        }
    }

    class TBinarizedWordHashVector {
    private:
        TVector<THashWithWeight> Hashes;

    public:
        static constexpr ui32 MaxWeight = Max<ui8>();
        static constexpr ui64 MaxWeightSquared = Sqr(MaxWeight);

        TBinarizedWordHashVector() = default;
        explicit TBinarizedWordHashVector(const THashMap<TString, double>& word2weight);
        explicit TBinarizedWordHashVector(const TMap<ui32, ui8>& word2weight);
        ui64 operator*(const TBinarizedWordHashVector& other) const;
        bool operator == (const TBinarizedWordHashVector& other) const;
        void Save(IOutputStream& os) const;
        TString SaveToString() const;
        TString SaveToLeb128() const;
        void Load(TArrayRef<const char> data);
        void LoadFromLeb128(TStringBuf s);

        [[nodiscard]]
        size_t Size() const {
            return Hashes.size();
        }

        [[nodiscard]]
        const THashWithWeight* Data() const {
            return Hashes.data();
        }

        [[nodiscard]]
        bool Empty() const {
            return Hashes.empty();
        }

        [[nodiscard]]
        ui32 Hash(size_t idx) const {
            return Hashes[idx].Hash;
        }

        [[nodiscard]]
        ui8 Weight(size_t idx) const {
            return Hashes[idx].Weight;
        }

        ui64 DotProduct(const THashWithWeight* begin, const THashWithWeight* end) const;

        // LeftJoin - callback only events where left side is not null.
        template <bool LeftJoin, class TCallback>
        ui64 DotProductWithCallback(const THashWithWeight* begin, const THashWithWeight* end, TCallback& cb) const {
            return ::NClickSim::DotProductWithCallback<LeftJoin, TCallback>(Hashes.data(), Hashes.data() + Hashes.size(), begin, end, cb);
        }

        // Normalize dot product value (vectors are assumed to be normalized)
        template<bool assertOnOutOfRange = true>
        static float Normalize(size_t clickSim) {
            if constexpr (assertOnOutOfRange) {
                Y_ASSERT(clickSim <= MaxWeightSquared * 1.015);
            }
            return static_cast<float>(std::min(clickSim, MaxWeightSquared)) * (1.0f / MaxWeightSquared);
        }
    };

    struct TMimMatchedAnnWeightCalcer {
        ui8 MinWeight = Max<ui8>();

        void operator() (const THashWithWeight* l, const THashWithWeight* r) {
            Y_ASSERT(l);
            if (r) {
                MinWeight = Min(MinWeight, r->Weight);
            } else {
                MinWeight = 0;
            }
        }

        [[nodiscard]]
        ui8 GetMinWeight() const {
            return MinWeight;
        }
    };

    struct TMaxNotMatchedWeightCalcer {
        ui8 MaxQueryNotMatchedWordWeight = 0;

        void operator() (const THashWithWeight* l, const THashWithWeight* r) {
            Y_ASSERT(l);
            if (!r) {
                MaxQueryNotMatchedWordWeight = Max(MaxQueryNotMatchedWordWeight, l->Weight);
            }
        }

        ui8 GetMaxQueryNotMatchedWordWeight() const {
            return MaxQueryNotMatchedWordWeight;
        }
    };

    struct TPerWordCMMaxPredictionMinCalcer {
        float MaxPredictionMin = 1.0;

        void operator() (const THashWithWeight* l, const THashWithWeight* r) {
            Y_ASSERT(l);
            if (r) {
                MaxPredictionMin = Min(MaxPredictionMin, Sqr(Frac2Float(l->Weight)) * Frac2Float(r->Weight));
            } else {
                MaxPredictionMin = 0.0;
            }
        }

        float GetMaxPredictionMin() const {
            return MaxPredictionMin;
        }
    };
}
