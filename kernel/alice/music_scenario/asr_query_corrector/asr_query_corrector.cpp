#include "asr_query_corrector.h"

#include <kernel/translit/translit.h>

#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>

#include <util/generic/algorithm.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/split.h>

#include <algorithm>
#include <cmath>

namespace NAlice {

    namespace NMusic {

        namespace {

            constexpr const double EPSILON = 1e-10;
            static const TVector<TString> REDUNDANTS = { "алиса",
                                                         "включи",
                                                         "музык",
                                                         "яндекс",
                                                         "поставь",
                                                         "пожалуйста",
                                                         "мне",
                                                         "нам" };

            TVector<TAsrHypo> NormalizeHypos(TVector<TAsrHypo>&& hypos) {
                Sort(hypos, [](const TAsrHypo& first, const TAsrHypo& second) {
                                return first.Confidence > second.Confidence;
                            });
                double sumExp = 0;
                for (TAsrHypo& hypo : hypos) {
                    hypo.Confidence = std::exp(hypo.Confidence);
                    sumExp += hypo.Confidence;
                }
                for (TAsrHypo& hypo : hypos) {
                    hypo.Confidence /= (sumExp + EPSILON);
                }
                return hypos;
            }

            bool AreSimilarByLevenshtein(const TStringBuf first,
                                         const TStringBuf second,
                                         const double threshold)
            {
                const double distance = static_cast<double>(NLevenshtein::Distance(first, second));
                const double length = second.length();
                return distance < threshold * length;
            }

            bool AreSimilar(const TStringBuf first, TStringBuf second, const double threshold) {
                return AreSimilarByLevenshtein(first, second, threshold) ||
                       AreSimilarByLevenshtein(TransliterateBySymbol(second), first, threshold) ||
                       AreSimilarByLevenshtein(second, TransliterateBySymbol(first), threshold);
            }

            bool IsRedundant(const TStringBuf token) {
                return std::any_of(REDUNDANTS.begin(), REDUNDANTS.end(),
                                   [&token](const TString& redundant) {
                                       return token.StartsWith(redundant);
                                   });
            }

            void TryAdd(const TStringBuf hypoToken, const TVector<TStringBuf>& realAsrTokens,
                        const double threshold, TVector<TStringBuf>& extensions)
            {
                // Do not add hypo if it is a stop word
                if (IsRedundant(hypoToken)) {
                    return;
                }

                // Check that hypoToken is not present in initial request
                for (const TStringBuf realAsrToken : realAsrTokens) {
                    if (AreSimilar(hypoToken, realAsrToken, threshold)) {
                        return;
                    }
                }

                // Check that hypoToken is not added to extensions already
                for (const TStringBuf extension : extensions) {
                    if (AreSimilar(hypoToken, extension, threshold)) {
                        return;
                    }
                }
                extensions.push_back(hypoToken);
            }

        } // anonymous namespace

        TString ExtendQueryWithAsrHypos(const TStringBuf query,
                                        TVector<TAsrHypo>&& rawHypos,
                                        const double similarityThreshold,
                                        const double confidenceThreshold,
                                        const double minimalConfidence,
                                        const size_t maxExtensionLength,
                                        const size_t maxNumHyposToCheck)
        {
            const TVector<TAsrHypo> hypos = NormalizeHypos(std::move(rawHypos));
            const TVector<TStringBuf> split = StringSplitter(query).Split(' ');
            TVector<TStringBuf> extensions;

            double sumConfidence = 0;
            for (size_t i = 0; i < hypos.size() && i < maxNumHyposToCheck; ++i) {
                const TAsrHypo& hypo = hypos[i];
                if (hypo.Confidence < minimalConfidence) {
                    break;
                }
                const TVector<TStringBuf> hypoTokens = StringSplitter(hypo.Hypo).Split(' ');
                for (const TStringBuf hypoToken : hypoTokens) {
                    TryAdd(hypoToken, split, similarityThreshold, extensions);
                }
                sumConfidence += hypo.Confidence;
                if (sumConfidence > confidenceThreshold) {
                    break;
                }
                if (extensions.size() >= maxExtensionLength) {
                    break;
                }
            }
            if (extensions.empty()) {
                return TString(query);
            }
            TStringBuilder stringBuilder;
            stringBuilder << query << ' ' << JoinSeq(" ", extensions);
            return stringBuilder;
        }

    } // namespace NMusic

} // namespace NAlice
