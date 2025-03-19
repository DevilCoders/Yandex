#pragma once
#include <kernel/common_server/api/snapshots/fetching/abstract/content.h>
#include <kernel/common_server/api/snapshots/object.h>
#include <kernel/common_server/api/snapshots/controller.h>
#include "context.h"

namespace NCS {
    template <class TSnapshotObject>
    class TFakeSnapshotFetcher: public ISnapshotContentFetcher {
    private:
        TVector<TSnapshotObject> ObjectsForFetch;
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const override {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TJsonProcessor::WriteObjectsContainer(result, "objects", ObjectsForFetch);
            return result;
        }
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
            return TJsonProcessor::ReadObjectsContainer(jsonInfo, "objects", ObjectsForFetch);
        }
        virtual bool DoFetchData(const TDBSnapshot& snapshotInfo, const ISnapshotsController& controller, const IBaseServer& /*server*/, const TString& userId) const override {
            TAtomicSharedPtr<TFakeSnapshotFetcherContext> pContext = snapshotInfo.GetFetchingContext().GetPtrAs<TFakeSnapshotFetcherContext>();
            if (!!snapshotInfo.GetFetchingContext() && !pContext) {
                TFLEventLog::Error("incorrect context class for fetcher")("incoming_context", snapshotInfo.GetFetchingContext().SerializeToJson());
                return false;
            }

            ui32 readyObjects = 0;
            if (!!pContext) {
                readyObjects = pContext->GetReadyObjects();
            }
            auto soManager = controller.GetSnapshotObjectsManager(snapshotInfo);
            if (!soManager) {
                TFLEventLog::Error("cannot fetch snapshot objects manager")("snapshot_code", snapshotInfo.GetSnapshotCode());
                return false;
            }

            ui32 idx = 0;
            for (auto&& i : ObjectsForFetch) {
                if (idx++ < readyObjects) {
                    continue;
                }
                if (!soManager->PutObjects({ i.SerializeToTableRecord() }, snapshotInfo.GetSnapshotId(), userId)) {
                    TFLEventLog::Error("cannot store snapshot objects");
                    return false;
                }
                auto context = MakeHolder<TFakeSnapshotFetcherContext>();
                context->SetReadyObjects(idx);

                if (!controller.StoreSnapshotFetcherContext(snapshotInfo.GetSnapshotCode(), context.Release())) {
                    TFLEventLog::Error("cannot store snapshot fetcher context");
                    return false;
                }
                return false;
            }
            return true;
        }
    public:
        TSnapshotObject& AddObject() {
            ObjectsForFetch.emplace_back(TSnapshotObject());
            return ObjectsForFetch.back();
        }

        static TString GetTypeName() {
            return "fake_fetcher_" + TSnapshotObject::GetTypeName();
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };
}
