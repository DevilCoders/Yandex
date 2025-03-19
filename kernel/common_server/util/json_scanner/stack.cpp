#include "stack.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {
    bool TStackContext::InitContext() {
        if (!Scanner->IsValid()) {
            for (auto&& i : StoreValues) {
                ContextInfo.Remove(i);
            }
        } else {
            const NJson::TJsonValue& val = Scanner->GetValue();
            for (auto&& i : StoreValues) {
                TStringBuf l;
                TStringBuf r;
                if (i.TrySplit(':', l, r)) {
                } else {
                    l = i;
                    r = i;
                }
                if (r == "__key") {
                    ContextInfo.Put(l, Scanner->GetScannerKey());
                } else if (r == "__self") {
                    ContextInfo.Put(l, &val);
                } else {
                    auto* valLocal = val.GetValueByPath(r);
                    if (valLocal) {
                        ContextInfo.Put(l, valLocal);
                    }
                }
            }
        }
        return true;
    }

    bool TStackContext::DropContext() {
        for (auto&& i : StoreValues) {
            TStringBuf l;
            TStringBuf r;
            if (i.TrySplit(':', l, r)) {
            } else {
                l = i;
                r = i;
            }
            ContextInfo.Remove(l);
        }
        return true;
    }

    TStackContext::TStackContext(const NJson::TJsonValue& value, TJsonScannerContext& contextInfo)
        : ContextInfo(contextInfo)
    {
        if (value.IsArray()) {
            Scanner = MakeHolder<TArrayScanner>(value);
        } else if (value.IsMap()) {
            Scanner = MakeHolder<TMapScanner>(value);
        } else {
            Scanner = MakeHolder<TValueScanner>(value);
        }
    }

    bool TStackContext::Prepare(const TString& matchingContext) {
        MatchingExpression = matchingContext;
        TStringBuf sb(MatchingExpression.data(), MatchingExpression.size());
        TStringBuf match;
        TStringBuf store;
        if (sb.TrySplit('|', match, store)) {

        } else {
            match = sb;
        }
        if (!Scanner->DeserializeFromString(match)) {
            TFLEventLog::Log("cannot parse json scanner");
            return false;
        }
        if (!Scanner->Prepare()) {
            TFLEventLog::Log("cannot prepare json scanner");
            return false;
        }
        StringSplitter(store).Split('&').Collect(&StoreValues);
        return InitContext();
    }

    bool TStackContext::Next() {
        DropContext();
        if (!Scanner->Next()) {
            return false;
        }
        InitContext();
        return true;
    }

}
