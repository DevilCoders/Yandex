#pragma once

#include "daemon_config.h"
#include "options.h"
#include "patcher.h"

#include <util/generic/string.h>
#include <util/system/rwlock.h>

class TServerConfigConstructorParams {
private:
    bool IncorrectTextFlag = false;
    TString Text;
    TAtomicSharedPtr<TDaemonConfig> Daemon;
    const TString Path;
    TConfigPatcher* Preprocessor;
    TRWMutex Mutex;

public:
    TString GetTextUnsafe() const {
        TReadGuard rg(Mutex);
        return Text;
    }

    const TString& GetPath() const {
        return Path;
    }

    TGuardedDaemonConfig GetDaemonConfig() const;

    TConfigPatcher* GetPreprocessor() const {
        return Preprocessor;
    }

    bool HasDaemonConfig() const {
        return !!Daemon;
    }

    bool TextIsCorrect() const {
        return !IncorrectTextFlag;
    }

    TString GetText() const;

    void RefreshConfig();

    TServerConfigConstructorParams(const char* text,
                                   const char* path = nullptr,
                                   TConfigPatcher* preprocessor = nullptr)
        : Text(text)
        , Daemon(new TDaemonConfig(text, false))
        , Path(path)
        , Preprocessor(preprocessor)
    {
        RefreshConfig();
    }

    TServerConfigConstructorParams(TDaemonOptions& options, const TString& text = Default<TString>())
        : Text(text ? text : options.RunPreprocessor())
        , Daemon(new TDaemonConfig(text.data(), false))
        , Path(options.GetConfigFileName())
        , Preprocessor(&options.GetPreprocessor())
    {
        RefreshConfig();
    }
};
