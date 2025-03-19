#pragma once
#include <kernel/common_server/api/snapshots/storage/abstract/storage.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/api/snapshots/object.h>
#include "request.h"

namespace NCS {
    namespace NSnapshots {
        template <class TRequest>
        class TAsyncRequestSelectionFetcher: public ISelectionResult {
        private:
            NThreading::TFuture<typename TRequest::TResponse> RequestFuture;
        protected:
            virtual bool DoFetch(TVector<TMappedObject>& result) const override {
                if (!RequestFuture.GetValueSync().IsSuccess()) {
                    TFLEventLog::Error("cannot receive reply from snapshot service");
                    return false;
                }
                result = RequestFuture.GetValueSync().GetObjects();
                return true;
            }
        public:
            TAsyncRequestSelectionFetcher(NThreading::TFuture<typename TRequest::TResponse>&& r)
                : RequestFuture(std::move(r)) {
            }

        };

        class TAPIObjectsManager: public IObjectsManager {
        private:
            using TRequest = TSnapshotObjectsRequest<TMappedObject>;
            static TFactory::TRegistrator<TAPIObjectsManager> Registrator;
            CSA_DEFAULT(TAPIObjectsManager, TString, ExternalAPIServiceId);
            NExternalAPI::TSender::TPtr Client;
        protected:
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::Read(jsonInfo, "external_service_api_id", ExternalAPIServiceId, true, false)) {
                    return false;
                }
                return true;
            }
            virtual NJson::TJsonValue DoSerializeToJson() const override {
                NJson::TJsonValue result = NJson::JSON_MAP;
                TJsonProcessor::Write(result, "external_service_api_id", ExternalAPIServiceId);
                return result;
            }
            virtual bool DoRemoveSnapshot(const ui32 /*snapshotId*/) const override {
                TFLEventLog::Error("not applicable");
                return false;
            }
            virtual bool DoCreateSnapshot(const ui32 /*snapshotId*/) const override {
                return true;
            }
            virtual bool DoRemoveSnapshotObjects(const TVector<NCS::NStorage::TTableRecordWT>& /*objectIds*/, const ui32 /*snapshotId*/) const override {
                TFLEventLog::Error("not applicable");
                return false;
            }
            virtual bool DoRemoveSnapshotObjectsBySRCondition(const ui32 /*snapshotId*/, const TSRCondition& /*customCondition*/) const override {
                TFLEventLog::Error("not applicable");
                return false;
            }
            virtual bool DoGetAllObjects(const ui32 /*snapshotId*/, TVector<TMappedObject>& /*result*/, const TObjectsFilter& /*objectsFilter*/) const override {
                TFLEventLog::Error("not applicable");
                return false;
            }

            virtual ISelectionResult::TPtr DoGetObjects(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const override {
                if (objectIds.empty()) {
                    return MakeAtomicShared<TEmptySelectionResult>();
                }

                NThreading::TFuture<TRequest::TResponse> r = Client->SendRequestAsync<TRequest>(objectIds, snapshotId);
                return MakeAtomicShared<TAsyncRequestSelectionFetcher<TRequest>>(std::move(r));
            }
            virtual bool DoPutObjects(const TVector<NCS::NSnapshots::TMappedObject>& /*result*/, const ui32 /*snapshotId*/, const TString& /*userId*/) const override {
                TFLEventLog::Error("not applicable");
                return false;
            }
            virtual bool DoUpsertObjects(const TIndex& /*index*/, const TVector<NCS::NSnapshots::TMappedObject>& /*objects*/, const ui32 /*snapshotId*/, const TString& /*userId*/) const override {
                TFLEventLog::Error("not applicable");
                return false;
            }
            virtual bool DoInitialize(const IBaseServer& server) override {
                Client = server.GetSenderPtr(ExternalAPIServiceId);
                if (!Client) {
                    TFLEventLog::Error("incorrect client_id")("client_id", ExternalAPIServiceId);
                    return false;
                }
                return true;
            }

        public:
            static TString GetTypeName() {
                return "api_snapshot_objects";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            virtual ISelectionResult::TPtr FetchObjectAsync(const TVector<NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const override {
                return DoGetObjects(objectIds, snapshotId);
            }
        };
    }
}
