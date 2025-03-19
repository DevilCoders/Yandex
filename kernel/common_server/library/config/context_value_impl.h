#pragma once

#include "context_value.h"

#include <library/cpp/logger/global/global.h>

namespace NConfig {

    template<typename TValue>
    TContextValue<TValue>::TContextValue(TAggregate aggregate, TValue defaultValue) {
        Aggregate = std::move(aggregate);
        DefaultValue = std::move(defaultValue);
    }

    template<typename TValue>
    TContextValue<TValue>::TContextValue(TAggregateSimple* aggregate, TValue defaultValue) :
            TContextValue(TAggregate(aggregate), std::move(defaultValue)) {}

    template<typename TValue>
    void TContextValue<TValue>::Init(TVector<TRule> rules) {
        Rules = std::move(rules);
        Initialized = true;
    }

    template<typename TValue>
    TValue TContextValue<TValue>::Resolve(const TSet<TMap<TString, TString>>& contexts) const {
        DEBUG_LOG << "Enter TContextValue<TValue>::Resolve" << Endl;
        if (!Initialized) {
            FATAL_LOG << "TContextValue::Init must be called prior to usage" << Endl;
            return DefaultValue;
        }
        if (!Aggregate) {
            FATAL_LOG << "TContextValue::Aggregate must be not empty" << Endl;
            return DefaultValue;
        }

        auto Accumulate = [this](TMaybe<TValue>* accumulator, const TValue& value) {
            if (*accumulator) {
                *accumulator = Aggregate(**accumulator, value);
            } else {
                *accumulator = value;
            }
        };

        TMaybe<TValue> globalAccumulator;
        for (const TMap<TString, TString>& context : contexts) {
            TMaybe<int> bestPriority;
            TMaybe<TValue> currentAccumulator;
            for (const TRule &rule : Rules) {
                bool mismatch = false;
                for (const auto&[contextName, contextValue] : rule.ContextChecks) {
                    auto it = context.find(contextName);
                    if (it == context.end()) {
                        FATAL_LOG << "Context parameter is missing: " << contextName << Endl;
                        mismatch = true;
                        break;
                    }
                    if (it->second != contextValue) {
                        mismatch = true;
                        break;
                    }
                }
                if (mismatch) {
                    continue;
                }

                if (bestPriority && *bestPriority > rule.Priority) {
                    continue;
                }
                if (!bestPriority || *bestPriority < rule.Priority) {
                    bestPriority = rule.Priority;
                    currentAccumulator = Nothing();
                }

                Accumulate(&currentAccumulator, rule.Value);
            }
            if (currentAccumulator) {
                Accumulate(&globalAccumulator, *currentAccumulator);
            }
        }

        if (globalAccumulator) {
            DEBUG_LOG << "Leaving TContextValue<TValue>::Resolve with *globalAccumulator = " << *globalAccumulator << Endl;
            return *globalAccumulator;
        }

        DEBUG_LOG << "Leaving TContextValue<TValue>::Resolve with DefaultValue = " << DefaultValue << Endl;
        return DefaultValue;
    };

}  // namespace NConfig
