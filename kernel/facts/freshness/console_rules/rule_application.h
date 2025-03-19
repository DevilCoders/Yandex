#pragma once

#include <kernel/facts/word_set_match/word_set_match.h>

#include <util/generic/fwd.h>

class TRWMutex;

namespace NFacts {

    const TString JoinFactFields(
        const TString& query,
        const TString& answer,
        const TString& originalUrl,
        const TString& extendedText,
        const TString& headline
    );

    bool IsQueryBanned(const TString& query, const TSet<TString>& bannedQueries);
    bool IsFactUrlBanned(const TString& normalizedUrl, const THashSet<TString>& bannedUrls);
    bool IsFactBanned(const TString& answer, const TString& normalizedUrl, const THashSet<std::pair<TString, TString>>& bannedAnswerUrlPairs);
    const TVector<TString>* GetFactMatchingWordset(const TString& factFieldsJoined, const TWordSetSubstringMatch& substringFilter);

}
