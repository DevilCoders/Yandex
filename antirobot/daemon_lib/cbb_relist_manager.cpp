#include "cbb_relist_manager.h"
#include "parse_cbb_response.h"


namespace NAntiRobot {


void TCbbReListManagerCallback::operator()(const TVector<TString>& listStrs) {
    TVector<TString> list;
    NParseCbb::ParseTextList(list, listStrs[0]);

    TVector<TRegexMatcherEntry> entries;
    entries.reserve(list.size());

    for (const auto& regex : list) {
        if (auto entry = TRegexMatcherEntry::Parse(regex)) {
            entries.push_back(std::move(*entry));
        }
    }

    Matcher->Set(TRegexMatcher(entries));
}


TRegexMatcherAccessorPtr TCbbReListManager::Add(TCbbGroupId id) {
    auto [guard, callbackData] = TBase::Add({id});
    return callbackData->Callback.Matcher;
}


} // namespace NAntiRobot
