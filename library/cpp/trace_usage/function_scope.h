#pragma once

#include "global_registry.h"

namespace NTraceUsage {
    class TFunctionScope: private NNonCopyable::TNonCopyable {
    private:
        TTraceRegistryPtr Registry;
        const TStringBuf FunctionName;

        TFunctionScope(const char*) = delete; // Protect from doing bad things

    public:
        TFunctionScope(TStringBuf functionName);
        ~TFunctionScope();
    };

}
