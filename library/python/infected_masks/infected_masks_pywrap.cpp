#include "infected_masks_pywrap.h"

MatchResult BaseMatcher::FindAll(const TString& url) {
    if (!SafeBrowsingMasks_) {
        throw yexception() << "Use of uninitialized BaseMatcher";
    }

    TSafeBrowsingMasks::TMatchesType matches;
    MatchResult result;

    if (SafeBrowsingMasks_->IsInfectedUrl(url, &matches)) {
        for (auto const& match : matches) {
            TVector<TString> values;
            SafeBrowsingMasks_->ReadValues(match.second, values);
            if (values.empty()) {
                values.push_back("");
            }
            MatchData matchValues(values.begin(), values.end());
            result.push_back(std::make_pair(match.first, matchValues));
        }
    }
    return result;
}
