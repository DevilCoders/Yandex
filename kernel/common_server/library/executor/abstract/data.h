#pragma once
#include <util/thread/pool.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/datetime/base.h>
#include <kernel/common_server/library/executor/proto/task.pb.h>

class TConstructionContextImpl {
private:
    TString Type;
    TString Identifier;

public:
    TConstructionContextImpl(const TString& type, const TString& identifier)
        : Type(type)
        , Identifier(identifier)
    {
    }

    template <class T>
    static TConstructionContextImpl Build(const TString& identifier) {
        return TConstructionContextImpl(T::TypeName, identifier);
    }

    const TString& GetType() const {
        return Type;
    }

    const TString& GetIdentifier() const {
        return Identifier;
    }
};

using TTaskConstructionContext = TConstructionContextImpl;
using TDataConstructionContext = TConstructionContextImpl;

class IDistributedData {
protected:
    TInstant Deadline = TInstant::Zero();
    TInstant Finished = TInstant::Zero();

protected:
    virtual NJson::TJsonValue DoGetInfo(const TCgiParameters* /*cgi*/) const {
        return NJson::TJsonValue(NJson::JSON_NULL);
    }

public:
    using TFactory = NObjectFactory::TParametrizedObjectFactory<IDistributedData, TString, TDataConstructionContext>;
    using TPtr = TAtomicSharedPtr<IDistributedData>;

public:
    virtual ~IDistributedData() = default;

    virtual bool IsExpired(const TDuration additionalWaiting) const {
        return !!Deadline && Deadline + additionalWaiting < Now();
    }

    const TString& GetIdentifier() const {
        return Identifier;
    }

    IDistributedData& SetDeadline(const TInstant value) {
        Deadline = value;
        return *this;
    }

    IDistributedData& SetFinished(const TInstant value = Now()) {
        Finished = value;
        return *this;
    }

    bool GetIsFinished() const {
        return Finished != TInstant::Zero();
    }

    bool HasDeadline() const {
        return Deadline != TInstant::Zero();
    }

    TInstant GetDeadline() const;

    IDistributedData(const TDataConstructionContext& context)
        : Identifier(context.GetIdentifier())
        , Type(context.GetType())
    {
    }

    virtual TString GetType() const {
        return Type;
    }

    virtual NJson::TJsonValue GetDataInfo(const TCgiParameters* cgi = nullptr) const final;

    void SerializeMetaToProto(NFrontendProto::TDataMeta& proto) const;

    bool ParseMetaFromProto(const NFrontendProto::TDataMeta& proto);

    Y_WARN_UNUSED_RESULT virtual bool Deserialize(const TBlob& data) = 0;
    virtual TBlob Serialize() const = 0;

private:
    TString Identifier;
    TString Type;
};
