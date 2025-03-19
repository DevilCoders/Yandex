#include "checker.h"
#include <util/string/join.h>
#include <kernel/common_server/library/scheme/fields.h>
#include <kernel/common_server/util/json_processing.h>

namespace NCS {
    namespace NTextChecker {

        const TUtf16String TTextChecker::SpaceChar = UTF8ToWide(" ");

        TVector<TUtf16String> TTextChecker::BuildNGramms(const TWtringBuf w) const {
            TVector<TUtf16String> result;
            for (i32 i = -NGrammBase + 1; i < (i32)w.size(); ++i) {
                const TWtringBuf wb = w.substr(Max(0, i), Min(NGrammBase, NGrammBase + i));
                TUtf16String key = TUtf16String(wb.data(), wb.size());
                if (i < 0) {
                    while (key.size() < NGrammBase) {
                        key = SpaceChar + key;
                    }
                } else {
                    while (key.size() < NGrammBase) {
                        key = key + ' ';
                    }
                }
                result.emplace_back(std::move(key));
            }
            return result;
        }

        TUtf16String TTextChecker::Normalize(const TString& line) const {
            TUtf16String result = UTF8ToWide(line);
            if (!UseNormalization) {
                return result;
            }
            ::ToLower(result);
            TVector<TWtringBuf> words = StringSplitter(result.data()).SplitBySet(DelimitersSet.data()).SkipEmpty().ToList<TWtringBuf>();
            const auto pred = [this](const TWtringBuf w) {
                return w.size() < MinWordSize;
            };
            words.erase(std::remove_if(words.begin(), words.end(), pred), words.end());
            return JoinSeq(SpaceChar, words);
        }

        NCS::NScheme::TScheme TRequestFeatures::GetScheme() {
            NCS::NScheme::TScheme result;
            result.Add<NCS::NScheme::TFSBoolean>("use_whole_word_search").SetDefault(false);
            result.Add<NCS::NScheme::TFSNumeric>("misspells_count_max").SetDefault(1);
            result.Add<NCS::NScheme::TFSNumeric>("unordered_words_max").SetDefault(0);
            result.Add<NCS::NScheme::TFSNumeric>("near_words_distance_max").SetDefault(0);

            return result;
        }

        bool TRequestFeatures::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::Read(jsonInfo, "use_whole_word_search", UseWholeTextSearch)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "misspells_count_max", MisspellsCountMax)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "unordered_words_max", UnorderedWordsMax)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "near_words_distance_max", NearWordsDistanceMax)) {
                return false;
            }
            return true;
        }
    }
}
