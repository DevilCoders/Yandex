#include "objects_filter.h"
#include <kernel/common_server/library/vname_checker/checker.h>
namespace NCS {
    namespace NTags {

        TString TTagChecker::SerializeToString() const {
            TStringBuilder sb;
            if (Busy) {
                if (*Busy) {
                    sb << "*";
                } else {
                    sb << "@";
                }
            }
            if (!Exists) {
                sb << "-";
            }
            sb << TagName;
            return sb;
        }

        bool TTagChecker::DeserializeFromString(const TString& info) {
            TStringBuf sb(info.data(), info.size());
            while (sb.size()) {
                if (sb.StartsWith('#')) {
                    Busy = true;
                } else if (sb.StartsWith('-')) {
                    Exists = false;
                } else if (sb.StartsWith('+')) {
                    Exists = true;
                } else if (sb.StartsWith('@')) {
                    Busy = false;
                } else {
                    break;
                }
                sb.Skip(1);
            }
            if (!NCS::TVariableNameChecker::DefaultObjectId(sb)) {
                TFLEventLog::Error("incorrect tag checker string")("data", info)("reason", "incorrect name");
                return false;
            }
            if (sb.empty()) {
                TFLEventLog::Error("incorrect tag checker string")("data", info)("reason", "empty name");
                return false;
            }
            TagName = sb;
            return true;
        }

        TString TTagsCondition::SerializeToString() const {
            if (!OriginalString) {
                TVector<TString> tagCheckers;
                for (auto&& i : Checkers) {
                    const TString info = i.SerializeToString();
                    if (!info) {
                        continue;
                    }
                    tagCheckers.emplace_back(info);
                }
                return JoinSeq("*", tagCheckers);
            } else {
                return OriginalString;
            }
        }

        Y_WARN_UNUSED_RESULT bool TTagsCondition::DeserializeFromString(const TString& condition) {
            OriginalString = condition;
            TVector<TString> tagCheckerStrings;
            StringSplitter(condition).SplitBySet("* ").SkipEmpty().Collect(&tagCheckerStrings);
            TVector<TTagChecker> checkersLocal;
            for (auto&& i : tagCheckerStrings) {
                TTagChecker checker;
                if (!checker.DeserializeFromString(i)) {
                    TFLEventLog::Error("cannot parse tag checker")("raw_data", i);
                    return false;
                }
                checkersLocal.emplace_back(std::move(checker));
            }
            std::swap(checkersLocal, Checkers);
            return Checkers.size();
        }

        TSet<TString> TTagsCondition::GetAffectedTagNames() const {
            TSet<TString> result;
            for (auto&& i : Checkers) {
                result.emplace(i.GetTagName());
            }
            return result;
        }

        TObjectFilter& TObjectFilter::Merge(const TObjectFilter& tf) {
            if (tf.Conditions.empty()) {
                return *this;
            }
            OriginalString = "";
            for (auto&& i : tf.Conditions) {
                Conditions.emplace_back(i);
            }
            return *this;
        }

        TString TObjectFilter::SerializeToString() const {
            if (!OriginalString) {
                TVector<TString> conditionStrings;
                for (auto&& i : Conditions) {
                    conditionStrings.emplace_back(i.SerializeToString());
                }
                return JoinSeq(",", conditionStrings);
            } else {
                return OriginalString;
            }
        }

        Y_WARN_UNUSED_RESULT bool TObjectFilter::DeserializeFromString(const TString& condition) {
            OriginalString = condition;
            const TVector<TString> conditionStrings = StringSplitter(condition).SplitBySet(",").SkipEmpty().ToList<TString>();
            for (auto&& i : conditionStrings) {
                TTagsCondition c;
                if (!c.DeserializeFromString(i)) {
                    return false;
                }
                Conditions.emplace_back(std::move(c));
            }
            return true;
        }

        TSet<TString> TObjectFilter::GetAffectedTagNames() const {
            TSet<TString> result;
            for (auto&& i : Conditions) {
                const TSet<TString> tagNames = i.GetAffectedTagNames();
                result.insert(tagNames.begin(), tagNames.end());
            }
            return result;
        }

    }
}
