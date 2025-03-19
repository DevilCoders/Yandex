#pragma once

#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/util/accessor.h>

#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/object_factory/object_factory.h>

#include <util/generic/ptr.h>
#include <util/memory/blob.h>

class IRTBackgroundProcessState {
public:
    using TPtr = TAtomicSharedPtr<IRTBackgroundProcessState>;
    using TFactory = NObjectFactory::TObjectFactory<IRTBackgroundProcessState, TString>;
private:
    static TFactory::TRegistrator<IRTBackgroundProcessState> Registrator;
protected:
    virtual NFrontend::TScheme DoGetScheme() const {
        return NFrontend::TScheme();
    }
public:
    IRTBackgroundProcessState() = default;
    virtual ~IRTBackgroundProcessState() = default;

    virtual NFrontend::TScheme GetScheme() const final {
        return DoGetScheme();
    }

    virtual NJson::TJsonValue GetReport() const {
        return NJson::JSON_MAP;
    }

    virtual TString GetType() const {
        return GetTypeName();
    }

    static TString GetTypeName() {
        return "default";
    }

    virtual TBlob SerializeToBlob() const {
        return TBlob();
    }

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromBlob(const TBlob& /*data*/) {
        return true;
    }
};

class TJsonRTBackgroundProcessState: public IRTBackgroundProcessState {
protected:
    virtual NJson::TJsonValue SerializeToJson() const = 0;
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& value) = 0;

public:
    virtual NJson::TJsonValue GetReport() const override;
    virtual TBlob SerializeToBlob() const override;
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromBlob(const TBlob& data) override;
};

template <class TProto, class TBaseClass = IRTBackgroundProcessState>
class IProtoStateSerializable: public INativeProtoSerialization<TProto, TBaseClass> {
private:
    using TBase = INativeProtoSerialization<TProto, TBaseClass>;
public:
    using TBase::TBase;
};

class TRTBackgroundProcessStateContainer {
private:
    IRTBackgroundProcessState::TPtr ProcessState;
    RTLINE_ACCEPTOR_DEF(TRTBackgroundProcessStateContainer, ProcessName, TString);
    RTLINE_ACCEPTOR_DEF(TRTBackgroundProcessStateContainer, HostName, TString);
    RTLINE_ACCEPTOR(TRTBackgroundProcessStateContainer, LastExecution, TInstant, Now());
    RTLINE_ACCEPTOR(TRTBackgroundProcessStateContainer, LastFlush, TInstant, TInstant::Zero());
    RTLINE_ACCEPTOR_DEF(TRTBackgroundProcessStateContainer, Status, TString);

public:
    using TStates = TMap<TString, TRTBackgroundProcessStateContainer>;

    TRTBackgroundProcessStateContainer& SetProcessState(IRTBackgroundProcessState::TPtr state) {
        ProcessState = state;
        return *this;
    }

    IRTBackgroundProcessState::TPtr GetState() const {
        return ProcessState;
    }

    bool operator! () const {
        return !ProcessState;
    }

    const IRTBackgroundProcessState* operator->() const {
        return ProcessState.Get();
    }

    TRTBackgroundProcessStateContainer() = default;
    TRTBackgroundProcessStateContainer(IRTBackgroundProcessState::TPtr state)
        : ProcessState(state)
    {
    }

    NJson::TJsonValue GetReport() const;

    NCS::NScheme::TScheme GetScheme() const {
        NCS::NScheme::TScheme result;
        if (!!ProcessState) {
            result = ProcessState->GetScheme();
        }
        result.Add<TFSString>("host");
        result.Add<TFSString>("status");
        result.Add<TFSString>("type");
        result.Add<TFSNumeric>("last_execution").SetVisual(TFSNumeric::EVisualTypes::DateTime);
        result.Add<TFSNumeric>("last_flush").SetVisual(TFSNumeric::EVisualTypes::DateTime);
        return result;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromTableRecord(const NCS::NStorage::TTableRecordWT& record);

    NStorage::TTableRecord SerializeToTableRecord() const;
};
