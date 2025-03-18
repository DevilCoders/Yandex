#pragma once

#include <util/ysaveload.h>
#include <util/generic/vector.h>
#include <util/system/types.h>

namespace NSampling {
    template <typename T, typename TId, template <typename, typename> class S>
    class TWithID {
    public:
        using TWeight = T;
        using TIdentifier = TId;
        using TSampler = S<TWeight, ui32>;

        template <typename U>
        inline TWithID& Push(const U weight, const TIdentifier id) {
            Sampler_.Push(weight);
            Identifiers_.push_back(id);
            return *this;
        }

        template <typename TItValue, typename TItId>
        inline TWithID& PushMany(TItValue beginValue, TItValue endValue, TItId beginId) {
            for (; beginValue != endValue; ++beginValue, ++beginId) {
                Push(*beginValue, *beginId);
            }

            return *this;
        }

        template <typename G>
        const TIdentifier& operator()(G& generator) const {
            const auto index = Sampler_(generator);
            return Identifiers_.at(index);
        }

        template <typename TSum = TWeight>
        TWithID& Prepare() {
            Sampler_.template Prepare<TSum>();
            Identifiers_.shrink_to_fit();
            return *this;
        }

        size_t Size() const {
            return Identifiers_.size();
        }

        Y_SAVELOAD_DEFINE(Sampler_, Identifiers_)

    protected:
        TSampler Sampler_;
        TVector<TIdentifier> Identifiers_;
    };
}
