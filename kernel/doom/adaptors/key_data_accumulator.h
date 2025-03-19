#pragma once

#include <functional>

namespace NDoom {
    template <class KeyData, class AccumulatedData = KeyData, class BinaryOperation = std::plus<KeyData>>
    class TKeyDataAccumulator {
    public:
        using TKeyData = KeyData;
        using TAccumulatedData = AccumulatedData;
        using TBinaryOperation = BinaryOperation;

        static TAccumulatedData Zero() {
            return TAccumulatedData();
        }

        static void Accumulate(TAccumulatedData* acc, const TKeyData& data) {
            *acc = TBinaryOperation()(*acc, data);
        }
    };
}
