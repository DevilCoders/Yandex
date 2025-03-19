#pragma once
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/object_factory/object_factory.h>
#include <kernel/common_server/abstract/frontend.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/datetime/base.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/library/time_restriction/time_restriction.h>
#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/common/host_filter.h>
#include <kernel/common_server/common/attributed.h>
#include <kernel/common_server/library/unistat/cache.h>

class IRTBackgroundProcessState;
class TRTBackgroundProcessContainer;

class TRTBackgroundProcessFinishedMessage: public IMessage {
    RTLINE_ACCEPTOR_DEF(TRTBackgroundProcessFinishedMessage, RTBackgroundName, TString);
    RTLINE_ACCEPTOR_DEF(TRTBackgroundProcessFinishedMessage, RTBackgroundType, TString);


public:
    TRTBackgroundProcessFinishedMessage(const TString& rtBackgroundName, const TString& rtBackgroundType)
        : RTBackgroundName(rtBackgroundName)
        , RTBackgroundType(rtBackgroundType)
    {
    }
};

class IRTBackgroundProcess : public TAttributedEntity<TAttributedEntityDefaultFieldNames> {
    CSA_DEFAULT(IRTBackgroundProcess, TString, Description);
    CSA_READONLY_DEF(TTimeRestrictionsPool, TimeRestrictions);
    CSA_DEFAULT(IRTBackgroundProcess, THostFilter, HostFilter);
    CSA_DEFAULT(IRTBackgroundProcess, TString, RTProcessName);
    CSA_READONLY(TDuration, Freshness, TDuration::Minutes(1));
    CSA_READONLY_MAYBE(TDuration, Timeout);
    CSA_READONLY_DEF(TSet<TString>, Owners);

protected:
    mutable TInstant DataActuality;
    mutable TInstant StartInstant;
public:

    TCSSignals::TSignalBuilder RTSignal(const TString& name = Default<TString>(), const double value = 1) const {
        if (!!name) {
            return TCSSignals::Signal(GetRTProcessName() + "." + name, value);
        } else {
            return TCSSignals::Signal(GetRTProcessName(), value);
        }
    }

    TCSSignals::TSignalBuilder RTSignalProblem(const TString& name = Default<TString>()) const {
        if (!!name) {
            return TCSSignals::SignalProblem(GetRTProcessName() + "." + name);
        } else {
            return TCSSignals::SignalProblem(GetRTProcessName());
        }
    }

    class ITaskExecutor {
    public:
        virtual bool FlushActiveState(TAtomicSharedPtr<IRTBackgroundProcessState> state) = 0;
        virtual bool FlushStatus(const TString& status) = 0;
    };

    class TExecutionContext {
    private:
        const IBaseServer& BaseServer;
        CS_ACCESS(TExecutionContext, TInstant, LastFlush, TInstant::Zero());
    public:
        ITaskExecutor& Executor;

        TExecutionContext(const TExecutionContext& parent)
            : BaseServer(parent.BaseServer)
            , Executor(parent.Executor)
        {

        }

        virtual ~TExecutionContext() = default;

        TExecutionContext(const IBaseServer& server, ITaskExecutor& executor)
            : BaseServer(server)
            , Executor(executor) {

        }

        const IBaseServer& GetServer() const {
            return BaseServer;
        }

        template <class T>
        const T& GetServerAs() const {
            const T* server = VerifyDynamicCast<const T*>(&BaseServer);
            CHECK_WITH_LOG(server);
            return *server;
        }
    };
protected:
    virtual NFrontend::TScheme DoGetScheme(const IBaseServer& /*server*/) const {
        return NFrontend::TScheme();
    }
    virtual TAtomicSharedPtr<IRTBackgroundProcessState> DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> state, const TExecutionContext& context) const = 0;
    virtual NJson::TJsonValue DoSerializeToJson() const {
        return NJson::JSON_MAP;
    }
    virtual bool DoDeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) {
        return true;
    }

public:

    virtual bool IsSimultaneousProcess() const {
        return false;
    }

    virtual bool NeedGroupLock() const {
        return false;
    }

    bool IsHostAvailable(const IBaseServer& server) const {
        return HostFilter.CheckHost(server);
    }

    virtual TString GetType() const = 0;
    IRTBackgroundProcess() = default;
    virtual ~IRTBackgroundProcess() = default;

    using TFactory = NObjectFactory::TParametrizedObjectFactory<IRTBackgroundProcess, TString>;
    using TPtr = TAtomicSharedPtr<IRTBackgroundProcess>;

    NJson::TJsonValue GetReport() const {
        return SerializeToJson();
    }

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonSettings) final;

    virtual NJson::TJsonValue SerializeToJson() const final;

    virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const final;

    virtual TInstant GetNextStartInstant(const TInstant lastExecution) const = 0;
    virtual TAtomicSharedPtr<IRTBackgroundProcessState> Execute(TAtomicSharedPtr<IRTBackgroundProcessState> state, const TExecutionContext& context) const noexcept;
};

class TRTBackgroundProcessContainer {
private:
    IRTBackgroundProcess::TPtr ProcessSettings;
    CS_ACCESS(TRTBackgroundProcessContainer, bool, Enabled, false);
    CSA_DEFAULT(TRTBackgroundProcessContainer, TString, Name);
    CSA_READONLY(ui64, Revision, Max<ui64>());
public:
    TString GetClassName() const;
    static NFrontend::TScheme GetScheme(const IBaseServer& server);

    bool HasRevision() const {
        return Revision != Max<ui64>();
    }

    TMaybe<ui64> GetRevisionMaybe() const {
        return HasRevision() ? Revision : TMaybe<ui64>();
    }

    class TDecoder: public TBaseDecoder {
    public:
        RTLINE_ACCEPTOR(TDecoder, Revision, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Name, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Type, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Meta, i32, -1);
        RTLINE_ACCEPTOR(TDecoder, Enabled, i32, -1);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase, const bool strict = true)
            : TBaseDecoder(strict)
        {
            Revision = GetFieldDecodeIndex("bp_revision", decoderBase);
            Name = GetFieldDecodeIndex("bp_name", decoderBase);
            Meta = GetFieldDecodeIndex("bp_settings", decoderBase);
            Type = GetFieldDecodeIndex("bp_type", decoderBase);
            Enabled = GetFieldDecodeIndex("bp_enabled", decoderBase);
        }
    };

    bool operator! () const {
        return !ProcessSettings;
    }

    const IRTBackgroundProcess* operator->() const {
        return ProcessSettings.Get();
    }

    const IRTBackgroundProcess* Get() const {
        return ProcessSettings.Get();
    }

    IRTBackgroundProcess* operator->() {
        return ProcessSettings.Get();
    }

    Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        return TBaseDecoder::DeserializeFromJson(*this, jsonInfo);
    }

    TRTBackgroundProcessContainer GetDeepCopyUnsafe() const;

    TRTBackgroundProcessContainer() = default;
    TRTBackgroundProcessContainer(IRTBackgroundProcess::TPtr settings)
        : ProcessSettings(settings)
    {
    }

    bool CheckOwner(const TString& userId) const;
    bool CheckOwner(const TSet<TString>& availableUserIds) const;

    TString GetTitle() const;
    NJson::TJsonValue GetReport() const;
    NStorage::TTableRecord SerializeToTableRecord() const;

};

class IRTRegularBackgroundProcess: public IRTBackgroundProcess {
private:
    using TBase = IRTBackgroundProcess;

private:
    RTLINE_ACCEPTOR(IRTRegularBackgroundProcess, Period, TDuration, TDuration::Max());
    RTLINE_ACCEPTOR_DEF(IRTRegularBackgroundProcess, Timetable, TSet<ui16>);
    RTLINE_ACCEPTOR(IRTRegularBackgroundProcess, RobotUserId, TString, "ec65f4f0-8fa8-4887-bfdc-ca01c9906696");

protected:
    virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;

    virtual NJson::TJsonValue DoSerializeToJson() const override {
        NJson::TJsonValue result = TBase::DoSerializeToJson();
        if (Period != TDuration::Max()) {
            TJsonProcessor::WriteDurationString(result, "period", Period);
        }
        if (Timetable.size()) {
            JWRITE_DEF(result, "timetable", JoinSeq(",", Timetable), "");
        }
        JWRITE_DEF(result, "robot_user_id", RobotUserId, "ec65f4f0-8fa8-4887-bfdc-ca01c9906696");
        return result;
    }

    virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
public:
    TString GetRobotId() const {
        return "rt(" + GetRTProcessName() + "-" + GetRobotUserId() + ")";
    }

    virtual TInstant GetNextStartInstant(const TInstant lastCallInstant) const override;

    virtual TAtomicSharedPtr<IRTBackgroundProcessState> Execute(TAtomicSharedPtr<IRTBackgroundProcessState> state, const TExecutionContext& context) const noexcept override;
};
