#include "query.h"

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/string/split.h>

TString NClickSim::Query2SaasKey(const TString& normalizedQuery) {
    TString key = MD5::CalcRaw(normalizedQuery);
    key = Base64EncodeUrl(key);
    Y_ASSERT(key.EndsWith(",,"));
    Y_ASSERT(key.length() == 24);
    Y_ASSERT(key.find(',') == 22);
    return key.substr(0, 22);
}

std::optional<NClickSim::TWordVector> NClickSim::MakeVectorFromLingBoostExpansions(
        const TVector<std::pair<TString, float>>& expansions, size_t stripTo, const THashSet<TString>* stopWords)
{
    std::optional<NClickSim::TWordVector> result;

    for (const auto& [requestStr, w] : expansions) {
        if (Y_LIKELY(w > 0)) {
            TVector<TString> words;
            StringSplitter(requestStr).Split(' ').AddTo(&words);
            if (stopWords) {
                words.erase(std::remove_if(words.begin(), words.end(), [stopWords](const TString& w) -> bool {
                        return stopWords->contains(w); }),
                    words.end());
            }
            if (Y_UNLIKELY(words.empty())) {
                continue;
            }
            if (result) {
                ::NClickSim::TWordVector v(words);
                v *= w;
                (*result) += v;
            } else {
                result.emplace(words);
                (*result) *= w;
            }
        }
    }

    if (result) {
        result->Strip(stripTo);
        result->Normalize();
    }

    return result;
}
