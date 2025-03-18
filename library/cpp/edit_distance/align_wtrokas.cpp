#include "align_wtrokas.h"

#include <cstdlib>
#include <util/string/split.h>

namespace NEditDistance {
    TUtf16String SEPARATOR(u" ");

    TVector<size_t> GetWordLengths(TUtf16String& wtroka) {
        TVector<TUtf16String> tokens;
        StringSplitter(wtroka).SplitByString(SEPARATOR.data()).SkipEmpty().Collect(&tokens);
        TVector<size_t> lengths(tokens.size());
        for (size_t i = 0; i < tokens.size(); ++i) {
            lengths[i] = tokens[i].size();
        }
        wtroka = JoinStrings(tokens.begin(), tokens.end(), SEPARATOR);
        return lengths;
    }

    TVector<TUtf16String> RecoverStringsByLengths(const TUtf16String& wtroka, const TVector<size_t> lengths) {
        TVector<TUtf16String> substrings(lengths.size());
        size_t currentIndex = 0;
        for (size_t i = 0; i < lengths.size(); ++i) {
            substrings[i] = TUtf16String(wtroka, currentIndex, lengths[i]);
            currentIndex += lengths[i] + 1;
        }

        return substrings;
    }

    TUtf16String ChangeSymbol(const TUtf16String& wtroka, size_t index, wchar16 symbol) {
        TVector<wchar16> vector((wchar16*)wtroka.begin(), (wchar16*)wtroka.end());
        vector[index] = symbol;
        return TUtf16String(vector.begin(), vector.end());
    }

    TUtf16String RemoveSequentialSeparators(const TUtf16String& wtroka, const wchar16* separator) {
        TVector<TUtf16String> tokens;
        StringSplitter(wtroka).SplitByString(separator).SkipEmpty().Collect(&tokens);
        return JoinStrings(tokens.begin(), tokens.end(), separator);
    }

    size_t CalculateDifference(const VectorParWtrok& wtroki) {
        double diff = 0.;
        auto f = [&](const std::pair<TUtf16String, TUtf16String>& pair) {
            diff += std::abs((int)pair.first.size() - (int)pair.second.size());
        };
        std::for_each(wtroki.begin(), wtroki.end(), f);
        return diff;
    }

    VectorParWtrok AlignWtrokasOfDifferentLength(const TUtf16String& lhs, const TUtf16String& rhs) {
        TUtf16String joinedLhs(lhs), joinedRhs(rhs);
        auto lhsLengths = GetWordLengths(joinedLhs);
        auto rhsLengths = GetWordLengths(joinedRhs);
        size_t minSize = std::min(lhsLengths.size(), rhsLengths.size());
        bool swapped = false;
        if (lhsLengths.size() < rhsLengths.size()) {
            swapped = true;
            std::swap(lhsLengths, rhsLengths);
        }

        if (!lhsLengths.size() || !rhsLengths.size()) {
            return VectorParWtrok(1, std::pair<TUtf16String, TUtf16String>(lhs, rhs));
        }

        while (lhsLengths.size() != rhsLengths.size()) {
            TVector<size_t> bestMerge;
            TVector<int> bestDifference(2, MAX_DIFF);
            for (size_t currentInd = 0; currentInd < lhsLengths.size() - 1; ++currentInd) {
                TVector<size_t> currentLengths;
                size_t wordInd = 0;
                while (wordInd < lhsLengths.size()) {
                    if (wordInd == currentInd) {
                        currentLengths.push_back(lhsLengths[wordInd] + lhsLengths[wordInd + 1] + 1);
                        ++wordInd;
                    } else {
                        currentLengths.push_back(lhsLengths[wordInd]);
                    }
                    ++wordInd;
                }

                size_t forwardDifference = 0, backwardDifference = 0;
                for (size_t i = 0; i < minSize; ++i) {
                    forwardDifference += std::abs((int)currentLengths[i] - (int)rhsLengths[i]);
                    backwardDifference += std::abs((int)currentLengths[currentLengths.size() - 1 - i] -
                                                   (int)rhsLengths[rhsLengths.size() - 1 - i]);
                }

                size_t difference = std::min(forwardDifference, backwardDifference);
                int auxiliaryDifference = currentInd == currentLengths.size() - 1 ? -MAX_DIFF : -static_cast<int>(lhsLengths[currentInd + 1]);

                int overallDifference_[] = {static_cast<int>(difference), auxiliaryDifference};
                TVector<int> overallDifference(overallDifference_, overallDifference_ + 2);

                if (std::lexicographical_compare(overallDifference.begin(), overallDifference.end(),
                                                 bestDifference.begin(), bestDifference.end()))
                {
                    bestDifference = overallDifference;
                    bestMerge = currentLengths;
                }
            }

            lhsLengths = bestMerge;
        }

        if (swapped) {
            std::swap(lhsLengths, rhsLengths);
        }

        TVector<TUtf16String> lhsWordList = RecoverStringsByLengths(joinedLhs, lhsLengths);
        TVector<TUtf16String> rhsWordList = RecoverStringsByLengths(joinedRhs, rhsLengths);
        VectorParWtrok result(lhsWordList.size());
        for (size_t i = 0; i < lhsWordList.size(); ++i) {
            result[i] = std::pair<TUtf16String, TUtf16String>(lhsWordList[i], rhsWordList[i]);
        }
        return result;
    }

    VectorParWtrok AlignWtrokas(const TUtf16String& lhs, const TUtf16String& rhs) {
        if (!lhs.size() || !rhs.size()) {
            return VectorParWtrok();
        }

        TVector<TUtf16String> tokens;
        const TUtf16String lhsStripped = RemoveSequentialSeparators(lhs, SEPARATOR.data());
        const TUtf16String rhsStripped = RemoveSequentialSeparators(rhs, SEPARATOR.data());

        VectorParWtrok bestAlignment = AlignWtrokasOfDifferentLength(lhsStripped, rhsStripped);

        size_t bestDifference = CalculateDifference(bestAlignment);
        if (bestDifference > 3 &&
            std::count(lhsStripped.begin(), lhsStripped.end(), SEPARATOR[0]) == std::count(rhsStripped.begin(), rhsStripped.end(), SEPARATOR[0]))
        {
            size_t lastSpace = 0;
            size_t curSpace = lhsStripped.find(SEPARATOR.data());
            while (curSpace != TUtf16String::npos) {
                TUtf16String lhsReplaced = ChangeSymbol(lhsStripped, curSpace, wchar16('_'));
                auto currentAlignment = AlignWtrokasOfDifferentLength(lhsReplaced, rhsStripped);

                size_t currentDifference = CalculateDifference(currentAlignment);
                if (currentDifference < bestDifference) {
                    bestDifference = currentDifference;
                    bestAlignment = currentAlignment;
                }

                lastSpace = curSpace;
                curSpace = lhsStripped.find(SEPARATOR.data(), lastSpace + 1);
            }
        }

        return bestAlignment;
    }

}
