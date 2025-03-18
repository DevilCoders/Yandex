#pragma once

#include <util/generic/strbuf.h>

#include <cstdarg>

namespace NYTPython {
    using TLoggerLogCallback = void(i32 level, TStringBuf text);
    using TLoggerCheckLevelCallback = bool(i32);

    void InstallPythonLogger();
    void SetLoggerCallback(TLoggerCheckLevelCallback* checkLevel, TLoggerLogCallback* log);
}
