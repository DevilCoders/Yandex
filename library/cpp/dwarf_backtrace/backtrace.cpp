#include "backtrace.h"

#include <contrib/libs/backtrace/backtrace.h>

#include <util/generic/yexception.h>
#include <util/system/type_name.h>
#include <util/system/execpath.h>

namespace NDwarf {
    namespace {
        void HandleLibBacktraceError(void*, const char* msg, int errnum) {
            ythrow TSystemError(errnum) << "LibBacktrace failed: " << msg;
        }

        int HandleLibBacktraceFrame(void* data, uintptr_t pc, const char* filename, int lineno, const char* function) {
            auto* result = reinterpret_cast<TVector<TLineInfo>*>(data);
            auto& lineInfo = result->emplace_back();
            lineInfo.FileName = filename != nullptr ? filename : "???";
            lineInfo.Line = lineno;
            // libbacktrace doesn't provide column numbers, so fill this field with a dummy value.
            lineInfo.Col = 0;
            lineInfo.FunctionName = function != nullptr ? CppDemangle(function) : "???";
            lineInfo.Address = reinterpret_cast<const void*>(pc);
            return 0;
        }

        backtrace_state* GetLibBacktraceContext() {
            // Intentionally never freed (see https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/backtrace/backtrace.h?rev=6789902#L80).
            static backtrace_state* ctx = backtrace_create_state(
                GetPersistentExecPath().c_str(),
                1 /* threaded */,
                HandleLibBacktraceError,
                nullptr /* data for the error callback */
            );
            return ctx;
        }
    }

    TVector<TLineInfo> ResolveBackTrace(TArrayRef<const void* const> backtrace) {
        TVector<TLineInfo> result;
        for (const void* address : backtrace) {
            address = static_cast<const char*>(address) - 1; // last byte of the call instruction
            backtrace_pcinfo(
                GetLibBacktraceContext(),
                reinterpret_cast<uintptr_t>(address),
                HandleLibBacktraceFrame,
                HandleLibBacktraceError,
                &result);
        }
        return result;
    }
}
