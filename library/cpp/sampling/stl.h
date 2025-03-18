#pragma once

#include <initializer_list>

namespace NSampling {
    template <template <typename, typename> class S, typename TSum = double, typename IntType = int>
    class TSTLWrapper {
    public:
        using result_type = IntType;
        using param_type = void; // impossible to implement with alias method
        using TSampler = S<double, result_type>;

        template <typename InputIt>
        TSTLWrapper(InputIt first, InputIt last) {
            Sampler_.PushMany(first, last);
            Sampler_.template Prepare<TSum>();
        }

        TSTLWrapper(std::initializer_list<double> weights)
            : TSTLWrapper{weights.begin(), weights.end()} {
        }

        template <typename Generator>
        result_type operator()(Generator& g) {
            return Sampler_(g);
        }

    private:
        TSampler Sampler_;
    };
}
