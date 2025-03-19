#pragma once
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/interfaces/container.h>

namespace NCS {
    class TDBSnapshot;
    class ISnapshotsController;

    class ISnapshotContentFetcher {
    protected:
        virtual bool DoFetchData(const TDBSnapshot& snapshotInfo, const ISnapshotsController& controller, const IBaseServer& server, const TString& userId) const = 0;
        virtual NJson::TJsonValue DoSerializeToJson() const = 0;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
    public:
        using TFactory = NObjectFactory::TObjectFactory<ISnapshotContentFetcher, TString>;
        using TPtr = TAtomicSharedPtr<ISnapshotContentFetcher>;
        virtual ~ISnapshotContentFetcher() = default;
        virtual NJson::TJsonValue SerializeToJson() const {
            return DoSerializeToJson();
        }
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            return DoDeserializeFromJson(jsonInfo);
        }
        virtual bool FetchData(const TDBSnapshot& snapshotInfo, const ISnapshotsController& controller, const IBaseServer& server, const TString& userId) const;
        virtual TString GetClassName() const = 0;
    };

    class TSnapshotContentFetcher: public TBaseInterfaceContainer<ISnapshotContentFetcher> {
    private:
        using TBase = TBaseInterfaceContainer<ISnapshotContentFetcher>;
    public:
        using TBase::TBase;
    };
}
