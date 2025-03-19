#pragma once

#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

#include <functional>

namespace NConfig {

    template<typename TValue>
    struct TContextValueRule {
        TMap<TString, TString> ContextChecks;
        TValue Value;
        TMaybe<int> Priority;
    };

    template<typename TValue>
    using TContextValueRules = TVector<TContextValueRule<TValue>>;

    template<typename TValue>
    class TContextValue {
    public:
        using TAggregate = std::function<TValue(const TValue&, const TValue&)>;
        using TAggregateSimple = const TValue&(const TValue&, const TValue&);
        using TRule = TContextValueRule<TValue>;
        using TRules = TContextValueRules<TValue>;

    public:
        TContextValue() = delete;  // instead use TContextValue(aggregate, defaultValue)
        TContextValue(const TContextValue&) = default;
        TContextValue(TContextValue&&) noexcept = default;
        TContextValue& operator=(const TContextValue&) = default;
        TContextValue& operator=(TContextValue&&) noexcept = default;

        TContextValue(TAggregate aggregate, TValue defaultValue);
        TContextValue(TAggregateSimple* aggregate, TValue defaultValue);

        void Init(TRules rules);

        Y_WARN_UNUSED_RESULT TValue Resolve(const TSet<TMap<TString, TString>>& contexts) const;

        Y_WARN_UNUSED_RESULT const TAggregate& GetAggregate() const { return Aggregate; }
        Y_WARN_UNUSED_RESULT const TValue& GetDefaultValue() const { return DefaultValue; }
        Y_WARN_UNUSED_RESULT const TRules& GetRules() const { return Rules; }
        Y_WARN_UNUSED_RESULT bool IsInitialized() const { return Initialized; }

    private:
        std::function<TValue(const TValue&, const TValue&)> Aggregate;
        TValue DefaultValue;
        TRules Rules;
        bool Initialized = false;
    };

}  // namespace NConfig

#include "context_value_impl.h"
