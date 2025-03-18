#pragma once

#include <library/cpp/terminate_handler/segv_handler.h>

void ReopenStdinToDevNull();

void ReopenStdoutStderrToDevNull();

void InitializeDaemonGeneric(
        const TString& logFileOption,
        const TString& pidFileOption,
        bool foreground);
