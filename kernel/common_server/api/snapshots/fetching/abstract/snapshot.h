#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {
    class ISnapshotsInfoFetcher {
    public:
        using TPtr = TAtomicSharedPtr<ISnapshotsInfoFetcher>;
        using TFactory = NObjectFactory::TObjectFactory<ISnapshotsInfoFetcher, TString>;
        virtual ~ISnapshotsInfoFetcher() = default;
        virtual bool FetchInfo(const TString& groupId, const TString& userId, const IBaseServer& server) const = 0;
        virtual NJson::TJsonValue SerializeToJson() const = 0;
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
        virtual TString GetClassName() const = 0;
        virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const = 0;
    };

    class TSnapshotInfoFetcherContainerConfiguration: public TDefaultInterfaceContainerConfiguration {
    public:
        static TString GetSpecialSectionForType(const TString& /*className*/) {
            return "data";
        }
    };

    class TSnapshotsInfoFetcherContainer: public TBaseInterfaceContainer<ISnapshotsInfoFetcher, TSnapshotInfoFetcherContainerConfiguration> {
    private:
        using TBase = TBaseInterfaceContainer<ISnapshotsInfoFetcher, TSnapshotInfoFetcherContainerConfiguration>;
    public:
        using TBase::TBase;
    };

}
