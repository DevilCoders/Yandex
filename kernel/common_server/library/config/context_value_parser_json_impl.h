#pragma once

#include "context_value_parser_json.h"

#include <library/cpp/logger/global/global.h>
#include <util/generic/cast.h>
#include <util/generic/maybe.h>

namespace NConfig {

    namespace NDetails {
        template<typename TValue>
        struct TExtractJsonValueTypeNotSupported;

        template<typename TValue, typename TEnabled = void>
        struct TExtractJsonValueHelper : TExtractJsonValueTypeNotSupported<TValue> {
            bool operator()(TValue* value, const NJson::TJsonValue &valueJson);
        };

        template<>
        struct TExtractJsonValueHelper<TString> {
            bool operator()(TString* value, const NJson::TJsonValue &valueJson) {
                *value = valueJson.GetStringSafe();
                return true;
            }
        };

        template<>
        struct TExtractJsonValueHelper<bool> {
            bool operator()(bool* value, const NJson::TJsonValue &valueJson) {
                *value = valueJson.GetBooleanSafe();
                return true;
            }
        };

        template<typename TValue>
        struct TExtractJsonValueHelper<TValue, std::enable_if_t<std::numeric_limits<TValue>::is_integer>> {
            bool operator()(TValue* value, const NJson::TJsonValue &valueJson) {
                if constexpr (std::numeric_limits<TValue>::is_signed) {
                    *value = SafeIntegerCast<TValue>(valueJson.GetIntegerSafe());
                } else {
                    *value = SafeIntegerCast<TValue>(valueJson.GetUIntegerSafe());
                }
                return true;
            }
        };

        template<typename TValue>
        struct TExtractJsonValueHelper<TValue, std::enable_if_t<std::is_floating_point_v<TValue>>> {
            bool operator()(TValue* value, const NJson::TJsonValue &valueJson) {
                *value = static_cast<TValue>(valueJson.GetDoubleSafe());
                return true;
            }
        };

        template<>
        struct TExtractJsonValueHelper<TDuration> {
            bool operator()(TDuration* value, const NJson::TJsonValue &valueJson) {
                *value = TDuration::Parse(valueJson.GetStringSafe());
                return true;
            }
        };

        template<typename TValue>
        bool ExtractJsonValue(TValue* value, const NJson::TJsonValue &valueJson) {
            try {
                return TExtractJsonValueHelper<TValue>{}(value, valueJson);
            } catch (const std::exception& exception) {
                ERROR_LOG << "Failed to extract json value: " << exception.what() << Endl;
                return false;
            }
        }

        template<typename TValue>
        void InitContextValueRuleDefaultPriority(TContextValueRule<TValue>* rule) {
            rule->Priority = 100 * rule->ContextChecks.size();
        }

    }  // namespace NDetails

    template<typename TValue>
    bool AppendContextValueRulesFromJson(TContextValueRules<TValue>* rules, const NJson::TJsonValue& json, const TMap<TString, TString>& additionalContextChecks) {
        try {
            for (size_t ruleIndex = 0; ruleIndex < json.GetArraySafe().size(); ++ruleIndex) {
                const NJson::TJsonValue *ruleJson;
                if (!json.GetValuePointer(ruleIndex, &ruleJson)) {
                    // shouldn't be possible
                    FATAL_LOG << "Failed to get rule #" << ruleIndex << Endl;
                    return false;
                }
                if (!ruleJson->IsMap()) {
                    ERROR_LOG << "Rule #" << ruleIndex << " must be a map" << Endl;
                    continue;
                }

                typename TContextValue<TValue>::TRule rule;

                rule.ContextChecks = additionalContextChecks;
                const NJson::TJsonValue *forJson;
                if (ruleJson->GetValuePointer("for", &forJson)) {
                    bool invalidRule = false;
                    for (const auto&[contextName, contextValueJson] : forJson->GetMapSafe()) {
                        const TString contextValue = contextValueJson.GetStringSafe();
                        if (rule.ContextChecks.contains(contextName)) {
                            if (rule.ContextChecks[contextName] == contextValue) {
                                WARNING_LOG << "Explicit context check duplicates implicit " << contextName << " in rule #" << ruleIndex << Endl;
                                continue;
                            } else {
                                ERROR_LOG << "Cannot override implicit context " << contextName << ", ignoring entire rule #" << ruleIndex << Endl;
                                invalidRule = true;
                                break;
                            }
                        }
                        rule.ContextChecks[contextName] = contextValueJson.GetStringSafe();
                    }
                    if (invalidRule) {
                        continue;
                    }
                }

                const NJson::TJsonValue *valueJson;
                if (!ruleJson->GetValuePointer("value", &valueJson)) {
                    ERROR_LOG << "Missing `value` property in rule #" << ruleIndex << Endl;
                    continue;
                }
                if (!NDetails::ExtractJsonValue(&rule.Value, *valueJson)) {
                    ERROR_LOG << "Failed to parse `value` property in rule #" << ruleIndex << Endl;
                    continue;
                }

                const NJson::TJsonValue *priorityJson = nullptr;
                if (ruleJson->GetValuePointer("priority", &priorityJson)) {
                    if (!priorityJson->IsInteger()) {
                        ERROR_LOG << "Property `priority` must be an integer in rule #" << ruleIndex << Endl;
                        continue;
                    }
                    rule.Priority = priorityJson->GetIntegerSafe();
                } else {
                    NDetails::InitContextValueRuleDefaultPriority(&rule);
                }

                rules->push_back(std::move(rule));
            }

            return true;
        } catch (const NJson::TJsonException& exception) {
            ERROR_LOG << "Failed to parse json: " << exception.what() << Endl;
            return false;
        }
    }

}  // namespace NConfig
