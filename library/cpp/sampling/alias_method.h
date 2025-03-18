#pragma once

#include "with_id.h"

#include <util/ysaveload.h>
#include <util/generic/typetraits.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/ymath.h>
#include <util/system/types.h>

// https://en.wikipedia.org/wiki/Alias_method
// More precisely, this is an implementation of Vose's Alias method.
// Vose, Michael D. "A linear algorithm for generating random numbers with a given distribution"

namespace NSampling {
    template <typename T, typename TId = ui32>
    class TAliasMethod {
    public:
        using TWeight = T;
        using TIdentifier = TId;

        static_assert(std::is_floating_point<TWeight>::value, "must be floating point type");

        template <typename U>
        inline TAliasMethod& Push(const U weight) {
            Y_ENSURE(weight > 0, "weight must be positive");
            Data_.push_back(TEntry{static_cast<TWeight>(weight)});
            return *this;
        }

        template <typename TIt>
        inline TAliasMethod& PushMany(TIt begin, TIt end) {
            if (Y_UNLIKELY(!Data_)) {
                Data_.assign(begin, end);
                return *this;
            }

            for (; begin != end; ++begin) {
                Push(*begin);
            }

            return *this;
        }

        template <typename S = TWeight>
        TAliasMethod& Prepare() {
            Y_ENSURE(Data_, "no samples provided");

            // Calculating cumulative sum
            auto cumSum = S{};
            for (const auto& entry : Data_) {
                cumSum += entry.Prob;
            }

            const auto sum = TWeight{cumSum};
            Y_ENSURE(IsValidFloat(sum) && sum > 0, "value is not a valid float or not positive");
            auto small = TVector<size_t>{};
            auto large = TVector<size_t>{};
            for (size_t index = 0, indexEnd = Data_.size(); index < indexEnd; ++index) {
                const auto value = Data_[index].Prob * (indexEnd / sum); // may overflow here
                Y_ENSURE(IsValidFloat(value) && value > 0, "value is not a valid float or not positive");
                if (value < 1) {
                    small.push_back(index);
                } else {
                    large.push_back(index);
                }
            }

            while (small && large) {
                const auto smallIndex = small.back();
                const auto largeIndex = large.back();
                small.pop_back();
                large.pop_back();
                Data_[smallIndex].Alias = largeIndex;
                const auto largeProbUpdated = (Data_[smallIndex].Prob + Data_[largeIndex].Prob) - 1;
                Data_[largeIndex].Prob = largeProbUpdated;
                if (largeProbUpdated < 1) {
                    small.push_back(largeIndex);
                } else {
                    large.push_back(smallIndex);
                }
            }

            while (large) {
                const auto index = large.back();
                large.pop_back();
                Data_[index].Prob = 1;
            }

            while (small) {
                const auto index = small.back();
                small.pop_back();
                Data_[index].Prob = 1;
            }

            Ready_ = true;
            return *this;
        }

        /* Generates random numbers that are distributed according to the associated probability
         * distribution. Has O(1) complexity, to be exact, requires only two call to pseudorandom
         * number generator.
         *
         * @param generator             Uniform random number generator.
         *
         * @return                      Generated random number.
         */
        template <typename G>
        TIdentifier operator()(G& generator) const noexcept {
            Y_VERIFY(Ready_, "call Prepare() first");
            const auto index = static_cast<size_t>(generator.GenRandReal2() * Data_.size());
            const auto prob = generator.GenRandReal1();
            if (prob < Data_[index].Prob) {
                return static_cast<TIdentifier>(index);
            } else {
                return static_cast<TIdentifier>(Data_[index].Alias);
            }
        }

        size_t Size() const noexcept {
            return Data_.size();
        }

        Y_SAVELOAD_DEFINE(Data_, Ready_)

    private:
        struct TEntry {
            TWeight Prob = {};
            ui32 Alias = {};

            TEntry() {
            }

            TEntry(const TWeight weight)
                : Prob{weight}
                , Alias{} {
            }

            Y_SAVELOAD_DEFINE(Prob, Alias)
        };

        bool Ready_ = false;
        TVector<TEntry> Data_ = {};
    };

    template <typename T, typename TId>
    using TAliasMethodWithID = TWithID<T, TId, TAliasMethod>;
}
