#pragma once

#include <kernel/common_server/proto/background.pb.h>
#include <kernel/common_server/rt_background/state.h>

class TRTHistoryWatcherState: public IProtoStateSerializable<NCommonServerProto::THistoryProcessorData> {
private:
    using TBase = IProtoStateSerializable<NCommonServerProto::THistoryProcessorData>;
    CS_ACCESS(TRTHistoryWatcherState, ui64, LastEventId, Max<ui64>());
    CSA_DEFAULT(TRTHistoryWatcherState, TString, LastError);

protected:
    virtual void SerializeToProto(NCommonServerProto::THistoryProcessorData& proto) const override;
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromProto(const NCommonServerProto::THistoryProcessorData& proto) override;

public:
    using TBase::TBase;

    virtual NJson::TJsonValue GetReport() const override;
    virtual NFrontend::TScheme DoGetScheme() const override;
};

class TRTInstantWatcherState: public IProtoStateSerializable<NCommonServerProto::TInstantProcessorData> {
private:
    using TBase = IProtoStateSerializable<NCommonServerProto::TInstantProcessorData>;
    CS_ACCESS(TRTInstantWatcherState, TInstant, LastInstant, TInstant::Zero());

protected:
    virtual void SerializeToProto(NCommonServerProto::TInstantProcessorData& proto) const override;
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromProto(const NCommonServerProto::TInstantProcessorData& proto) override;

public:
    using TBase::TBase;

    virtual NJson::TJsonValue GetReport() const override;
    virtual NFrontend::TScheme DoGetScheme() const override;
};

