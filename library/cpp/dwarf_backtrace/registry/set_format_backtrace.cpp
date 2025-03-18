#include <library/cpp/dwarf_backtrace/backtrace.h>
#include <util/stream/format.h>
#include <util/system/backtrace.h>

namespace {
    void PrintDwarfBacktrace(IOutputStream* out, void* const* backtrace, size_t size) {
        for (const auto& info : NDwarf::ResolveBackTrace({backtrace, size})) {
            *out << info.FileName << ":" << info.Line << ":" << info.Col << " in " << info.FunctionName << " (" << Hex((ptrdiff_t)info.Address, HF_ADDX) << ')' << Endl;
        }
    }

    [[maybe_unused]] auto _ = SetFormatBackTraceFn(&PrintDwarfBacktrace);
}
