#pragma once

#include <library/cpp/binsaver/bin_saver.h>

#include <util/generic/vector.h>
#include <util/generic/bitops.h>

namespace NSampling {
    template <typename TFreq>
    class TSamplingTree {
    public:
        TSamplingTree(size_t numElements, TFreq initialFreq = 0);

        /* Generates random numbers that are distributed according to the associated probability
         * distribution. Has O(log(N)) complexity.
         *
         * @param generator             Uniform random number generator.
         *
         * @return                      Generated random number.
         */
        template <typename G>
        size_t operator()(G& generator) const noexcept;

        TFreq GetFreq(size_t elemIndex) const;
        void SetFreq(size_t elemIndex, TFreq freq);

    private:
        size_t GetParentPos(size_t nodePos) const noexcept;
        size_t GetLeftChildPos(size_t nodePos) const noexcept;
        size_t GetRightChildPos(size_t nodePos) const noexcept;
        size_t GetLeafPos(size_t leafIndex) const noexcept;

    private:
        ui64 NumElements;
        ui64 NumElementsNextPowOf2;
        TVector<TFreq> FreqTree;

        Y_SAVELOAD_DEFINE(NumElements, NumElementsNextPowOf2, FreqTree);
    };

    template <typename TFreq>
    TSamplingTree<TFreq>::TSamplingTree(size_t numElements, TFreq initialFreq)
        : NumElements(numElements)
        , NumElementsNextPowOf2(FastClp2(NumElements))
        , FreqTree(NumElementsNextPowOf2 * 2 - 1)
    {
        Y_ASSERT(NumElements > 0);

        // Setup initial frequencies
        for (size_t i = 0; i < NumElements; ++i) {
            FreqTree[GetLeafPos(i)] = initialFreq;
        }

        // Compute partial sums
        size_t nodePos = NumElementsNextPowOf2 - 2;
        for (size_t i = 0; i < NumElementsNextPowOf2 - 1; ++i) {
            FreqTree[nodePos] = FreqTree[GetLeftChildPos(nodePos)] + FreqTree[GetRightChildPos(nodePos)];
            --nodePos;
        }
    }

    template <typename TFreq>
    template <typename G>
    size_t TSamplingTree<TFreq>::operator()(G& generator) const noexcept {
        const TFreq freqSum = FreqTree[0];
        Y_ASSERT(freqSum > 0);

        TFreq randomNum = static_cast<TFreq>(generator.GenRandReal2() * freqSum);
        size_t currentPos = 0;
        const auto treeEndPos = GetLeafPos(0);
        while (currentPos < treeEndPos) {
            const auto left = GetLeftChildPos(currentPos);
            const auto right = GetRightChildPos(currentPos);
            if (randomNum < FreqTree[left]) {
                currentPos = left;
            } else {
                currentPos = right;
                randomNum -= FreqTree[left];
            }
        }

        return currentPos - GetLeafPos(0);
    }

    template <typename TFreq>
    TFreq TSamplingTree<TFreq>::GetFreq(size_t elemIndex) const {
        return FreqTree[GetLeafPos(elemIndex)];
    }

    template <typename TFreq>
    void TSamplingTree<TFreq>::SetFreq(size_t elemIndex, TFreq freq) {
        Y_ASSERT(freq >= 0);

        size_t currentPos = GetLeafPos(elemIndex);
        TFreq prevFreq = FreqTree[currentPos];
        while (true) {
            FreqTree[currentPos] += freq - prevFreq;
            if (currentPos == 0) {
                break;
            }
            currentPos = GetParentPos(currentPos);
        }
    }

    template <typename TFreq>
    size_t TSamplingTree<TFreq>::GetParentPos(size_t nodePos) const noexcept {
        Y_ASSERT(nodePos > 0);
        return (nodePos - 1) / 2;
    }

    template <typename TFreq>
    size_t TSamplingTree<TFreq>::GetLeftChildPos(size_t nodePos) const noexcept {
        Y_ASSERT(nodePos < GetLeafPos(0));
        return nodePos * 2 + 1;
    }

    template <typename TFreq>
    size_t TSamplingTree<TFreq>::GetRightChildPos(size_t nodePos) const noexcept {
        Y_ASSERT(nodePos < GetLeafPos(0));
        return nodePos * 2 + 2;
    }

    template <typename TFreq>
    size_t TSamplingTree<TFreq>::GetLeafPos(size_t leafIndex) const noexcept {
        Y_ASSERT(leafIndex < NumElements);
        return NumElementsNextPowOf2 - 1 + leafIndex;
    }
}
