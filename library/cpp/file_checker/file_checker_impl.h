#pragma once

#include "file_checker.h"

#include <util/datetime/base.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/system/mem_info.h>
#include <util/system/file.h>
#include <util/system/fstat.h>

namespace NChecker {
    template <typename TObject>
    TFileChecker<TObject>::TFileChecker(const TString& fname, TFileOptions o, ui32 timerms)
        : State(new TState)
        , FName(fname)
        , Options(o)
        , Checker(*this, timerms)
    {
    }

    template <typename TObject>
    const TString& TFileChecker<TObject>::GetMonitorFileName() const {
        return FName;
    }

    template <typename TObject>
    typename TFileChecker<TObject>::TSharedObjectPtr TFileChecker<TObject>::GetObject() const {
        if (!!PendingState) {
            SwitchState();
        }

        {
            TReadGuard guard(Mutex);
            return !State ? nullptr : State->Object;
        }
    }

    template <typename TObject>
    bool TFileChecker<TObject>::IsChanged() const {
        time_t t = TFileStat(GetMonitorFileName().data()).MTime;

        if (!t && !Options.ZeroOnDelete) {
            return false;
        }

        TReadGuard guard(Mutex);
        return t != (!PendingState ? State->Timestamp : PendingState->Timestamp);
    }

    template <typename TObject>
    bool TFileChecker<TObject>::DoUpdate(bool beforestart) {
        const TString& filename = GetMonitorFileName();

        try {
            // keeping a link to the file so it won't go away suddenly
            TFileHandle handle(filename, RdOnly);

            if (!handle.IsOpen()) {
                if (Options.ZeroOnDelete) {
                    TIntrusivePtr<TState> state(new TState);
                    TWriteGuard guard(Mutex);
                    PendingState = state;
                }

                return true;
            }

            {
                TFile file(handle.Release(), filename);
                TFileStat stat(file);

                switch (GetUpdateStrategy(file, stat, GetObject())) {
                    default:
                    case US_DO_NOT_READ:
                        break;
                    case US_DROP_AND_READ: {
                        TIntrusivePtr<TState> state(new TState);
                        TWriteGuard guard(Mutex);
                        PendingState = nullptr;
                        State = state;
                        [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
                    }
                    case US_READ_AND_SWAP: {
                        TIntrusivePtr<TState> state(new TState);
                        TSharedObjectPtr oldobj = GetObject();
                        state->Timestamp = stat.MTime;
                        state->Object = MakeNewObject(file, stat, oldobj, beforestart);

                        if (state->Object) {
                            TWriteGuard guard(Mutex);
                            PendingState = state;
                        }

                        break;
                    }
                }
            }
        } catch (...) {
            TString now = ToString(TInstant::Now());
            Clog << Sprintf("%s %s (%s:%d) while reading '%s': %s",
                            now.data(), __FUNCTION__, __FILE__, __LINE__, filename.data(), CurrentExceptionMessage().data())
                 << Endl;
        }

        return true;
    }

    template <typename TObject>
    void TFileChecker<TObject>::SwitchState() const {
        TSharedObjectPtr oldobj, newobj;

        {
            TWriteGuard guard(Mutex);

            if (!PendingState)
                return;

            oldobj = !State ? nullptr : State->Object;
            newobj = PendingState->Object;
            State = PendingState;
            PendingState = nullptr;
        }

        OnSwitchState(newobj, oldobj);
    }

}
