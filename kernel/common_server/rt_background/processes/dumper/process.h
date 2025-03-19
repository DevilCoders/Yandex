#pragma once

#include "meta_parser.h"
#include "dumper.h"

#include <kernel/common_server/rt_background/processes/common/database.h>
#include <kernel/common_server/rt_background/processes/common/yt.h>
#include <kernel/common_server/proto/background.pb.h>
#include <kernel/common_server/library/time_restriction/time_restriction.h>

#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/rt_background/state.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/rt_background/processes/common/yt.h>

class TRTDumperState: public TRTHistoryWatcherState {
    static TFactory::TRegistrator<TRTDumperState> Registrator;
    static TFactory::TRegistrator<TRTDumperState> RegistratorForBackwardCompatibility;

public:
    virtual TString GetType() const override;
};

class TRTDumperWatcher: public TDBTableScanner {
private:
    using TBase = TDBTableScanner;
    static TFactory::TRegistrator<TRTDumperWatcher> Registrator;
    static TFactory::TRegistrator<TRTDumperWatcher> RegistratorForBackwardCompatibility;

public:
    using TBase::TBase;

    virtual TString GetType() const override {
        return GetTypeName();
    }

    static TString GetTypeName() {
        return "yt_dumper";
    }

    virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;
    virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
    virtual NJson::TJsonValue DoSerializeToJson() const override;

private:
    virtual bool ProcessRecords(const TRecordsSetWT& records, const TExecutionContext& context, TProcessRecordsContext& processContext) const override;

    virtual TRTHistoryWatcherState* BuildState() const override {
        return new TRTDumperState();
    }

    void AdvanceCursor(const TRecordsSetWT& records, ui64& lastEventId, const TString& eventIDColumn) const noexcept;

private:
    CSA_DEFAULT(TRTDumperWatcher, NCS::TRTDumperContainer, Dumper);
    CSA_DEFAULT(TRTDumperWatcher, TString, DBType);
    CS_ACCESS(TRTDumperWatcher, bool, DBDefineSchema, false);
};
