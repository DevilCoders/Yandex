#include "config_constructor.h"

#include <kernel/daemon/common/time_guard.h>

#include <library/cpp/mediator/global_notifications/system_status.h>

#include <util/stream/file.h>

TGuardedDaemonConfig TServerConfigConstructorParams::GetDaemonConfig() const {
    CHECK_WITH_LOG(!!Daemon);
    return TGuardedDaemonConfig(Daemon, Mutex);
}

TString TServerConfigConstructorParams::GetText() const {
    TReadGuard rg(Mutex);
    AssertCorrectConfig(!IncorrectTextFlag, "Incorrect text for config");
    return Text;
}

void TServerConfigConstructorParams::RefreshConfig() {
    if (!!Path) {
        TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> tg("TServerConfigConstructorParams::RefreshConfig");
        TWriteGuard rg(Mutex);
        AssertCorrectConfig(TFsPath(Path).Exists(), "config is missing: %s", Path.data());
        if (Preprocessor) {
            try {
                Preprocessor->ReReadEnvironment();
                Text = Preprocessor->ReadAndProcess(Path);
                IncorrectTextFlag = false;
            } catch (...) {
                Preprocessor->SetStrict(false);
                Text = Preprocessor->ReadAndProcess(Path);
                Preprocessor->SetStrict(true);
                ERROR_LOG << "Incorrect config: " << Endl << Text << Endl << CurrentExceptionMessage() << Endl;
                IncorrectTextFlag = true;
            }
        } else {
            Text = TUnbufferedFileInput(Path).ReadAll();
        }
        Daemon.Reset(new TDaemonConfig(Text.data(), false));
        Daemon->RestoreCType(Path);

        if (Daemon->GetController().ReinitLogsOnRereadConfigs) {
            Daemon->InitLogs();
        }
        if (!!Daemon->GetController().ConfigsRoot) {
            TFsPath(Daemon->GetController().ConfigsRoot).MkDirs();
        }
        if (!!Daemon->GetController().StateRoot) {
            TFsPath(Daemon->GetController().StateRoot).MkDirs();
        }
    }
}
