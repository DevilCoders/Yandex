#pragma once

#include "context_value.h"

#include <library/cpp/json/json_reader.h>
#include <util/generic/map.h>
#include <util/generic/set.h>

namespace NConfig {

    template<typename TValue>
    bool AppendContextValueRulesFromJson(TContextValueRules<TValue>* rules, const NJson::TJsonValue& json, const TMap<TString, TString>& additionalContextChecks);

}  // namespace NConfig

#include "context_value_parser_json_impl.h"
