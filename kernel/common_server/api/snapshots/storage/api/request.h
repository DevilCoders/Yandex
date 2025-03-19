#pragma once
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/library/storage/records/t_record.h>

namespace NCS {

    template <class TSnapshotObject>
    class TSnapshotObjectsRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    private:
        CSA_DEFAULT(TSnapshotObjectsRequest, TVector<NCS::NStorage::TTableRecordWT>, ObjectIds);
        CS_ACCESS(TSnapshotObjectsRequest, ui32, SnapshotId, 0);
    public:
        TSnapshotObjectsRequest() = default;
        TSnapshotObjectsRequest(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId)
            : ObjectIds(objectIds)
            , SnapshotId(snapshotId)
        {

        }

        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
            NJson::TJsonValue jsonBody = NJson::JSON_MAP;
            TJsonProcessor::WriteObjectsArray(jsonBody, "object_ids", ObjectIds);
            TJsonProcessor::Write(jsonBody, "snapshot_id", SnapshotId);
            request.SetPostData(std::move(jsonBody));
            request.SetUri("snapshot/" + TSnapshotObject::GetTypeName() + "/info");
            return true;
        }

        class TResponse: public TJsonResponse {
        private:
            CSA_READONLY_DEF(TVector<TSnapshotObject>, Objects);
        protected:
            virtual bool IsReplyCodeSuccess(const i32 code) const override {
                return code == HTTP_OK;
            }

            virtual bool DoParseJsonReply(const NJson::TJsonValue& json) override {
                return TJsonProcessor::ReadObjectsContainer(json, "objects", Objects);
            }

            virtual bool DoParseJsonError(const NJson::TJsonValue& /*json*/) override {
                return true;
            }
        };
    };
}
