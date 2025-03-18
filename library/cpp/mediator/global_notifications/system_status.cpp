#include "system_status.h"
#include <library/cpp/logger/global/global.h>
#include <util/system/backtrace.h>

namespace {
    [[noreturn]] void AbortServerStatus(TSystemStatusMessage::ESystemStatus status, const TString& message) {
        SendGlobalMessage<TSystemStatusMessage>(message, status);
        FATAL_LOG << "ServerDataStatus: " << message << Endl;
        PrintBackTrace();
        _exit(-2);
    }
}

void AbortFromCorruptedIndex(const TString& message) {
    AbortServerStatus(TSystemStatusMessage::ESystemStatus::ssIncorrectData, message);
}

void AbortFromCorruptedConfig(const TString& message) {
    AbortServerStatus(TSystemStatusMessage::ESystemStatus::ssIncorrectConfig, message);
}
