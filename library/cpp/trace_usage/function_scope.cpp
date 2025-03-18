#include "function_scope.h"

namespace NTraceUsage {
    TFunctionScope::TFunctionScope(TStringBuf functionName)
        : Registry(TGlobalRegistryGuard::GetCurrentRegistry())
        , FunctionName(functionName)
    {
        if (Registry) {
            Registry->ReportOpenFunction(FunctionName);
        }
    }
    TFunctionScope::~TFunctionScope() {
        if (Registry) {
            Registry->ReportCloseFunction(FunctionName);
        }
    }

}
