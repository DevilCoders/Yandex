#include "rule_application.h"

#include "substring_match_normalization.h"

#include <util/string/join.h>

#include <util/generic/hash_set.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NFacts {

    const TString JoinFactFields(
        const TString& query,
        const TString& answer,
        const TString& originalUrl,
        const TString& extendedText,
        const TString& headline
    ) {
        return Join(
            ' ',
            query,
            answer,
            originalUrl,
            extendedText,
            headline
        );
    }

    bool IsQueryBanned(const TString& query, const TSet<TString>& bannedQueries) {
        return bannedQueries.contains(query);
    }

    bool IsFactUrlBanned(const TString& normalizedUrl, const THashSet<TString>& bannedUrls) {
        return bannedUrls.contains(normalizedUrl);
    }

    bool IsFactBanned(const TString& answer, const TString& normalizedUrl, const THashSet<std::pair<TString, TString>>& bannedAnswerUrlPairs) {
        return bannedAnswerUrlPairs.contains(std::make_pair(answer, normalizedUrl));
    }

    const TVector<TString>* GetFactMatchingWordset(const TString& factFieldsJoined, const TWordSetSubstringMatch& substringFilter) {
        return substringFilter.GetFirstMatchingWordSet(NormalizeForSubstringFilter(factFieldsJoined), /*normalizeText*/ false);
    }

}
