#include "cbb_textlist_manager.h"

#include "environment.h"
#include "eventlog_err.h"
#include "match_rule_parser.h"
#include "parse_cbb_response.h"
#include "unified_agent_log.h"

#include <util/generic/algorithm.h>


namespace NAntiRobot {


void TCbbTextListManagerCallback::operator()(const TVector<TString>& listStrs) {
    CurrentRuleKeys.resize(Ids.size());

    TVector<THashSet<TBinnedCbbRuleKey>> keySets(listStrs.size());

    NAntirobotEvClass::TCbbRulesUpdated updateEvent;
    updateEvent.SetContainerName(Title);

    for (
        const auto& [listId, listStr, currentKeys, newKeys] :
            Zip(Ids, listStrs, CurrentRuleKeys, keySets)
    ) {
        size_t lineIdx = 1;

        for (const auto& lineToken : StringSplitter(listStr).Split('\n').SkipEmpty()) {
            const TString line(lineToken.Token());

            auto* const result = updateEvent.AddParseResults();
            result->SetRule(line);

            try {
                auto rule = NMatchRequest::ParseRule(line);
                const TBinnedCbbRuleKey key(TCbbRuleKey(listId, rule.RuleId), rule.ExpBin);

                if (!currentKeys.contains(key)) {
                    TPreparedRule preparedRule(std::move(rule));

                    for (auto* matcher : Matchers) {
                        matcher->Add(listId, preparedRule);
                    }
                }

                newKeys.insert(key);
                result->SetStatus("OK");
            } catch (const std::exception& exc) {
                result->SetStatus(
                    TStringBuilder() << "Error on line " << lineIdx << ": " << exc.what()
                );
            }

            ++lineIdx;
        }
    }

    for (const auto& [currentKeys, newKeys] : Zip(CurrentRuleKeys, keySets)) {
        for (const auto& key : currentKeys) {
            if (!newKeys.contains(key)) {
                for (auto* matcher : Matchers) {
                    matcher->Remove(key);
                }
            }
        }
    }

    CurrentRuleKeys = keySets;

    ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(CreateEventLogRecord(updateEvent));
}


void TCbbTextListManager::Add(
    const TVector<TCbbGroupId>& ids,
    const TString& title,
    TIncrementalRuleSet* matcher
) {
    auto [guard, callbackData] = TBase::Add(ids);
    auto& callback = callbackData->Callback;

    if (callback.Ids.empty()) {
        callback.Ids = ids;
    }

    if (!callback.Title.empty()) {
        callback.Title += ", ";
    }

    callback.Title += title;
    callback.Matchers.push_back(matcher);
}


} // namespace NAntiRobot
