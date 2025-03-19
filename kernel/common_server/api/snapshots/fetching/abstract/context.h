#pragma once
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {
    class TDBSnapshot;
    class TSnapshotsController;

    class ISnapshotContentFetcherContext {
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const = 0;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
    public:
        using TFactory = NObjectFactory::TObjectFactory<ISnapshotContentFetcherContext, TString>;
        using TPtr = TAtomicSharedPtr<ISnapshotContentFetcherContext>;
        virtual ~ISnapshotContentFetcherContext() = default;
        virtual NJson::TJsonValue SerializeToJson() const {
            return DoSerializeToJson();
        }
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            return DoDeserializeFromJson(jsonInfo);
        }
        virtual TString GetClassName() const = 0;
    };

    class TSnapshotContentFetcherContext: public TBaseInterfaceContainer<ISnapshotContentFetcherContext> {
    private:
        using TBase = TBaseInterfaceContainer<ISnapshotContentFetcherContext>;
    public:
        using TBase::TBase;
    };
}
