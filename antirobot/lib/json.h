#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/hash_set.h>
#include <util/generic/string.h>

#include <initializer_list>


namespace NAntiRobot {


class TKeyChecker {
public:
    TKeyChecker(std::initializer_list<TString> requiredKeys)
        : RequiredKeys(std::move(requiredKeys))
    {}

    explicit TKeyChecker(
        THashSet<TString> requiredKeys,
        THashSet<TString> optionalKeys
    )
        : RequiredKeys(std::move(requiredKeys))
        , OptionalKeys(std::move(optionalKeys))
    {}

    void Check(const NJson::TJsonValue& value) const;

private:
    THashSet<TString> RequiredKeys;
    THashSet<TString> OptionalKeys;
};


}
