#include "scanner.h"
#include <util/string/split.h>

namespace NCS {

    bool TJsonScannerByPath::Scan(const TString& path) {
        const TVector<TString> fullPath = StringSplitter(path).SplitBySet("[]\n\r").SkipEmpty().ToList<TString>();
        if (fullPath.empty()) {
            return true;
        }
        CurrentStack.emplace_back(TStackContext(StartNode, ContextInfo));
        auto it = fullPath.begin();
        if (!CurrentStack.back().Prepare(*it)) {
            return false;
        }
        while (true) {
            auto& context = CurrentStack.back();
            if (!context.IsValid()) {
                CurrentStack.pop_back();
                if (CurrentStack.empty()) {
                    return true;
                } else {
                    --it;
                    CHECK_WITH_LOG(CurrentStack.back().Next());
                }
            } else {
                if (++it == fullPath.end()) {
                    if (!Execute(context.GetValue())) {
                        return false;
                    }
                    --it;
                    CHECK_WITH_LOG(context.Next());
                } else {
                    CurrentStack.emplace_back(TStackContext(context.GetValue(), ContextInfo));
                    if (!CurrentStack.back().Prepare(*it)) {
                        return false;
                    }
                }
            }
        }
    }

}
