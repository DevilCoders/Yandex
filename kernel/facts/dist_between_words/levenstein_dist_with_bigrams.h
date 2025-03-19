#pragma once

#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>

#include <util/draft/matrix.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/string/vector.h>

#include <utility>

namespace NLevenstein {
    using namespace NLevenshtein;

    static constexpr float FORBIDDEN_BIGRAM_WEIGHT = 100000;

    template <class TStringType, class TDelim = TCharType<TStringType>>
    void DumpMatrix(const TVector<TStringType>& str1,
                    const TVector<TStringType>& str2,
                    size_t size1,
                    size_t size2,
                    const TDelim delim,
                    const TMatrix<float>& weights) {
        Cerr << "str1=" << JoinStrings(str1, delim) << " str2=" << JoinStrings(str2, delim) << Endl;
        Cerr << "size1=" << size1 << " size2=" << size2 << Endl;
        Cerr << "\t";
        for (size_t col = 0; col < size2 + 2; col++) {
            Cerr << col << "\t";
        }
        Cerr << Endl;
        for (size_t row = 0; row < size1 + 2; row++) {
            Cerr << row << "\t";
            for (size_t col = 0; col < size2 + 2; col++) {
                Cerr << weights[row][col] << "\t";
            }
            Cerr << Endl;
        }
        Cerr << Endl;
    }

    template <class TStringType, class TDelim = TCharType<TStringType>>
    TVector<TStringType> BuildBigrams(const TVector<TStringType>& str, TDelim delim) {
        TVector<TStringType> bigrams;
        bigrams.reserve(str.size());
        for (size_t i = 1, size = str.size(); i < size; ++i) {
            bigrams.push_back(str[i - 1] + delim + str[i]);
        }
        return bigrams;
    }

    inline bool IsWord(size_t n) {
        return n % 2 == 0;
    }

    inline bool IsBigram(size_t n) {
        return n % 2 != 0;
    }

    inline size_t Index(size_t i) {
        return i / 2 - 1;
    }

    inline size_t FindDim(size_t size) {
        return static_cast<size_t>(std::max(0, static_cast<int>(size) * 2 - 1));
    }

    /// Finds sequence of "edit moves" for two vectors of strings
    template <class TStringType,
            class TDelim = TCharType<TStringType>,
            class TReplaceWeigher = TWeightOneBinaryGetter<TCharType<TStringType>>,
            class TDeleteWeigher = TWeightOneUnaryGetter<TCharType<TStringType>>,
            class TInsertWeigher = TWeightOneUnaryGetter<TCharType<TStringType>>>
    float BigramLevensteinDistance(const TVector<TStringType>& str1,
            const TVector<TStringType>& str2,
            const TDelim delim,
            const TReplaceWeigher& replaceWeigher = TReplaceWeigher(),
            const TDeleteWeigher& deleteWeigher = TDeleteWeigher(),
            const TInsertWeigher& insertWeigher = TInsertWeigher()) {
        const size_t size1 = FindDim(str1.size());
        const size_t size2 = FindDim(str2.size());
        TMatrix<float> weights(size1 + 2, size2 + 2);
        weights[0][0] = 0;
        weights[1][0] = weights[0][1] = weights[1][1] = FORBIDDEN_BIGRAM_WEIGHT;
        const TVector<TStringType> str1Bigrams = BuildBigrams(str1, delim);
        const TVector<TStringType> str2Bigrams= BuildBigrams(str2, delim);
        for (size_t i = 2; i < size1 + 2; i++) {
            if (IsWord(i)) {
                weights[i][0] = std::min(weights[i - 1][0], weights[i - 2][0] + deleteWeigher(str1[Index(i)]));
            } else { // bigram
                weights[i][0] = weights[i - 3][0] + deleteWeigher(str1Bigrams[Index(i)]);
            }
            weights[i][1] = FORBIDDEN_BIGRAM_WEIGHT;
        }
        for (size_t i = 2; i < size2 + 2; i++) {
            if (IsWord(i)) {
                weights[0][i] = std::min(weights[0][i - 1], weights[0][i - 2] + insertWeigher(str2[Index(i)]));
            } else { // bigram
                weights[0][i] = weights[0][i - 3] + insertWeigher(str2Bigrams[Index(i)]);
            }
            weights[1][i] = FORBIDDEN_BIGRAM_WEIGHT;
        }
        // main logic
        for (size_t i = 2; i < size1 + 2; i++) {
            for (size_t j = 2; j < size2 + 2; j++) {
                if (IsWord(i) && IsWord(j)) {
                    const auto& part1 = str1[Index(i)];
                    const auto& part2 = str2[Index(j)];
                    if (part1 == part2) {
                        weights[i][j] = std::min(weights[i - 1][j - 1], weights[i - 2][j - 2]);
                    } else {
                        const float weight = std::min(weights[i - 1][j - 1], weights[i - 2][j - 2] + replaceWeigher(part1, part2));
                        weights[i][j] = weight;
                    }
                    const float deleteWeight = std::min(weights[i - 1][j], weights[i - 2][j] + deleteWeigher(part1));
                    if (weights[i][j] > deleteWeight) {
                        weights[i][j] = deleteWeight;
                    }
                    const float insertWeight = std::min(weights[i][j - 1], weights[i][j - 2] + insertWeigher(part2));
                    if (weights[i][j] > insertWeight) {
                        weights[i][j] = insertWeight;
                    }
                } else { // bigram
                    size_t iBack = 3;
                    size_t jBack = 3;
                    if (IsWord(i))
                        iBack--;
                    if (IsWord(j))
                        jBack--;
                    const float prev = weights[i - iBack][j - jBack];
                    const TVector<TStringType>& parts1 = IsWord(i) ? str1 : str1Bigrams;
                    const TVector<TStringType>& parts2 = IsWord(j) ? str2 : str2Bigrams;
                    const auto& part1 = parts1[Index(i)];
                    const auto& part2 = parts2[Index(j)];
                    if (part1 == part2) {
                        weights[i][j] = prev;
                    } else {
                        weights[i][j] = prev + replaceWeigher(part1, part2);
                    }
                    if (IsBigram(i)) {
                        const float deleteWeight = (weights[i - iBack][j]) + deleteWeigher(part1);
                        if (weights[i][j] > deleteWeight) {
                            weights[i][j] = deleteWeight;
                        }
                    }
                    if (IsBigram(j)) {
                        const float insertWeight = (weights[i][j - jBack]) + insertWeigher(part2);
                        if (weights[i][j] > insertWeight) {
                            weights[i][j] = insertWeight;
                        }
                    }
                }
            }
        }
        const auto row = size1 == 0 ? 0 : size1 + 1;
        const auto col = size2 == 0 ? 0 : size2 + 1;
        return weights[row][col];
    }
}
