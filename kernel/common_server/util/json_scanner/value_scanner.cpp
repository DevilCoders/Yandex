#include "value_scanner.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {

    bool TArrayScanner::Check() const {
        if (Index && Iterator - Value.GetArraySafe().begin() != *Index) {
            return false;
        }
        for (auto&& i : Conditions) {
            const NJson::TJsonValue* jsonValue = Iterator->GetValueByPath(i.GetKey());
            if (!jsonValue) {
                if (i.GetValues().contains("null")) {
                    continue;
                } else {
                    return false;
                }
            }
            if (!i.GetValues().contains(jsonValue->GetStringRobust())) {
                return false;
            }
        }
        return true;
    }

    bool TArrayScanner::Next() {
        if (!IsValid()) {
            return false;
        }
        while (IsValid()) {
            ++Iterator;
            if (IsValid() && Check()) {
                return true;
            }
        }
        return true;
    }

    bool TArrayScanner::DeserializeFromString(const TStringBuf info) {
        TStringBuf values;
        TStringBuf condition;
        if (!info.TrySplit('?', values, condition)) {
            values = info;
        }
        if (values != "*" && values != "") {
            ui32 idx = 0;
            if (!TryFromString<ui32>(values, idx)) {
                TFLEventLog::Log("incorrect format - cannot parse value as integer")("raw_data", info);
                return false;
            }
            Index = idx;
        }
        TStringBuf condLocal;
        TStringBuf condNext;
        while (condition) {
            if (condition.TrySplit('&', condLocal, condNext)) {
            } else {
                condLocal = condition;
                condNext = TStringBuf();
            }

            TCondition c;
            if (!c.DeserializeFromString(condLocal)) {
                return false;
            }
            AddCondition(std::move(c));
            condition = condNext;
        }
        return true;
    }

    bool TMapScanner::Next() {
        if (!IsValid()) {
            return false;
        }
        while (IsValid()) {
            ++Iterator;
            if (IsValid() && Check()) {
                return true;
            }
        }
        return true;
    }

    bool TMapScanner::DeserializeFromString(const TStringBuf info) {
        if (!!info && info != "*") {
            Indexes.clear();
            StringSplitter(info).SplitBySet(", ").SkipEmpty().Collect(&Indexes);
            if (Indexes.size() == 1) {
                Iterator = Value.GetMapSafe().find(*Indexes.begin());
            } else {
                Iterator = Value.GetMapSafe().begin();
            }
        }
        return true;
    }

    bool TArrayScanner::TCondition::DeserializeFromString(TStringBuf condition) {
        TStringBuf key;
        TStringBuf value;
        if (!condition.TrySplit('=', key, value)) {
            TFLEventLog::Error("incorrect condition for array")("raw_data", condition);
            return false;
        }
        Key = key;
        Values.emplace(value);
        return true;
    }

}
