#include "object.h"

namespace NCS {
    namespace NBackground {

        bool TDBBackgroundTask::DeserializeCommonFromProto(const NCSProto::TDBBackgroundTask& proto) {
            for (auto&& i : proto.GetTags()) {
                AddTag(i.GetTagId(), i.GetTagValue());
            }
            return true;
        }

        NCSProto::TDBBackgroundTask TDBBackgroundTask::SerializeCommonToProto() const {
            NCSProto::TDBBackgroundTask result;
            for (auto&& i : TBase::GetTags()) {
                auto* tagInfo = result.AddTags();
                tagInfo->SetTagId(i.first);
                tagInfo->SetTagValue(i.second);
            }
            return result;
        }

        bool TDBBackgroundTask::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            READ_DECODER_VALUE(decoder, values, SequenceTaskId);

            READ_DECODER_VALUE_INSTANT(decoder, values, StartInstant);
            READ_DECODER_VALUE_INSTANT(decoder, values, ConstructionInstant);
            READ_DECODER_VALUE_INSTANT_OPT(decoder, values, LastPingInstant);
            READ_DECODER_VALUE(decoder, values, CurrentHost);

            READ_DECODER_VALUE(decoder, values, InternalTaskId);
            READ_DECODER_VALUE(decoder, values, OwnerId);
            READ_DECODER_VALUE(decoder, values, QueueId);

            TString className;
            READ_DECODER_VALUE_TEMP(decoder, values, className, ClassName);

            NCSProto::TDBBackgroundTask commonData;
            if (!decoder.GetProtoValueBytes(decoder.GetCommonData(), values, commonData)) {
                return false;
            }
            if (!DeserializeCommonFromProto(commonData)) {
                return false;
            }

            THolder<IRTQueueAction> task(IRTQueueAction::TFactory::MakeHolder(className));
            if (!task) {
                TFLEventLog::Error("incorrect task class name")("class_name", className);
                return false;
            }

            TString data;
            if (!decoder.GetValueBytes(decoder.GetData(), values, data)) {
                return false;
            }

            const TBlob blob = TBlob::NoCopy(data.data(), data.size());
            if (!task->DeserializeFromBlob(blob)) {
                TFLEventLog::Error("cannot parse pq task")("class_name", className);
                return false;
            }
            Object = task.Release();

            return true;
        }

        NStorage::TTableRecord TDBBackgroundTask::SerializeToTableRecord() const {
            NStorage::TTableRecord result;
            result.SetNotEmpty("sequence_task_id", SequenceTaskId);
            result.SetNotEmpty("internal_task_id", GetInternalTaskId());
            result.SetNotEmpty("owner_id", GetOwnerId());
            result.SetNotEmpty("queue_id", GetQueueId());
            if (!!Object) {
                result.SetNotEmpty("class_name", Object->GetClassName());
                result.SetBytes("data", Object->SerializeToBlob().AsStringBuf());
            }
            result.SetProtoBytes("common_data", SerializeCommonToProto());
            result.Set("start_instant", StartInstant.Seconds());
            result.Set("construction_instant", ConstructionInstant.Seconds());
            result.Set("last_ping_instant", LastPingInstant.Seconds());
            result.SetNotEmpty("current_host", CurrentHost);
            return result;
        }

    }
}
