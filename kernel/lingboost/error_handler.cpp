#include "error_handler.h"

#include <util/string/builder.h>
#include <util/generic/map.h>

namespace NLingBoost {
    TString TErrorHandler::GetFullErrorMessage(const TStringBuf& prefix) const {
        if (Errors.empty()) {
            return {};
        }

        TStringBuilder str;

        TMap<TStringBuf, size_t> errorByMsg;
        for (const TErrorInfo& info : Errors) {
            errorByMsg[info.Message] += 1;
        }

        bool isFirst = true;
        for (const TErrorInfo& info : Errors) {
            if (size_t* countPtr = errorByMsg.FindPtr(info.Message)) {
                if (!isFirst) {
                    str << "\n";
                } else {
                    isFirst = false;
                }

                str << prefix << info.Message << " [in " << info.Context << "]";
                if (*countPtr > 1) {
                    str << " and " << (*countPtr - 1) << " more like this";
                }
                errorByMsg.erase(info.Message);
            }
        }

        return str;
    }

    void TErrorHandler::PushErrorContext(TContextPartPtr&& part) {
        ErrorContext.push_back(std::move(part));
    }

    void TErrorHandler::PopErrorContext() {
        ErrorContext.pop_back();
        if (ErrorContext.empty()) {
            Pool.ClearKeepFirstChunk();
        }
    }

    TString TErrorHandler::GetContextString() const {
        if (ErrorContext.empty()) {
            return {"/"};
        }

        TStringStream str;
        for (const TContextPartPtr& part: ErrorContext) {
            str << "/";
            part->Print(str);
        }

        return str.Str();
    }
} // NLingBoost
