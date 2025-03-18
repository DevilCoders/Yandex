#pragma once

#include "with_id.h"

#include <util/ysaveload.h>
#include <util/generic/algorithm.h>
#include <util/generic/typetraits.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/ymath.h>
#include <util/system/types.h>
#include <util/system/yassert.h>

// https://en.wikipedia.org/wiki/Fitness_proportionate_selection

namespace NSampling {
    template <typename T, typename TId = ui32>
    class TRouletteWheel {
    public:
        using TWeight = T;
        using TIdentifier = TId;

        static_assert(std::is_floating_point<TWeight>::value, "must be floating point type");

        template <typename U>
        inline TRouletteWheel& Push(const U weight) {
            Y_ENSURE(weight > 0, "weight must be positive");
            ProbabilityDistribution_.push_back(static_cast<TWeight>(weight));
            return *this;
        }

        template <typename TIt>
        inline TRouletteWheel& PushMany(TIt begin, TIt end) {
            if (Y_UNLIKELY(!ProbabilityDistribution_)) {
                ProbabilityDistribution_.assign(begin, end);
                return *this;
            }

            for (; begin != end; ++begin) {
                Push(*begin);
            }

            return *this;
        }

        template <typename S = TWeight>
        TRouletteWheel& Prepare() {
            Y_ENSURE(ProbabilityDistribution_, "probability space is empty");

            // Calculating cumulative sum
            auto cumSum = S{};
            for (auto& weight : ProbabilityDistribution_) {
                cumSum += weight;
                weight = cumSum;
            }

            const TWeight sum = cumSum;
            Y_ENSURE(IsValidFloat(sum) && sum > 0, "value is not a valid float or not positive");
            // Normalize, so that all values will be in [0, 1] interval
            for (auto& probability : ProbabilityDistribution_) {
                probability /= sum;
                Y_ENSURE(IsValidFloat(probability) && probability > 0, "value is not a valid float or not positive");
            }

            // Fix possible floating point arithmetic fails, last element MUST be 1, so that binary
            // search will always converge.
            ProbabilityDistribution_.back() = 1;

            Ready_ = true;

            return *this;
        }

        /* Generates random numbers that are distributed according to the associated probability
         * distribution. Has O(log(N)) complexity.
         *
         * @param generator             Uniform random number generator.
         *
         * @return                      Generated random number.
         */
        template <typename G>
        TIdentifier operator()(G& generator) const noexcept {
            Y_VERIFY(Ready_, "call Prepare() first");

            TWeight value;
            do {
                // GenRandReal2 has type double and when converting it to float it may be rounded up
                // to 1.f. See for yourself:
                //
                // ```
                // double a = 1. - std::numeric_limits<double>::epsilon();
                // float b = a;
                //
                // std::cout.precision(52);
                //
                // std::cout << "double: " << a << '\n' << "float: " << b << '\n';
                // ```
                //
                // you'll see:
                // double: 0.9999999999999997779553950749686919152736663818359375
                // float: 1
                value = static_cast<TWeight>(generator.GenRandReal2());
            } while (value >= 1); // random number from [0, 1)

            // Probably, this may be done in a more cache-friendly manner using Eytzinger layout.
            // But in this case we will need to explicitly store indices for elements of probability
            // space.
            const auto it = UpperBound(
                ProbabilityDistribution_.cbegin(),
                ProbabilityDistribution_.cend(),
                value);

            Y_ASSERT(ProbabilityDistribution_.end() != it);
            const auto index = it - ProbabilityDistribution_.begin();
            return static_cast<TIdentifier>(index);
        }

        size_t Size() const noexcept {
            return ProbabilityDistribution_.size();
        }

        Y_SAVELOAD_DEFINE(ProbabilityDistribution_, Ready_)

    protected:
        bool Ready_ = false;
        //! Before we call Prepare() weights would be stored here
        TVector<TWeight> ProbabilityDistribution_;
    };

    template <typename T, typename TId>
    using TRouletteWheelWithID = TWithID<T, TId, TRouletteWheel>;
}
