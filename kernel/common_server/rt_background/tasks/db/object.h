#pragma once
#include <kernel/common_server/rt_background/tasks/abstract/object.h>
#include <kernel/common_server/rt_background/tasks/db/proto/task.pb.h>
#include <kernel/common_server/library/storage/reply/decoder.h>

namespace NCS {
    namespace NBackground {
        class TDBBackgroundTask: public IRTQueueTask {
        private:
            using TBase = IRTQueueTask;
            CS_ACCESS(TDBBackgroundTask, ui64, SequenceTaskId, 0);

            CSA_DEFAULT(TDBBackgroundTask, TInstant, ConstructionInstant);
            CSA_DEFAULT(TDBBackgroundTask, TInstant, LastPingInstant);
            CSA_DEFAULT(TDBBackgroundTask, TString, CurrentHost);

            Y_WARN_UNUSED_RESULT bool DeserializeCommonFromProto(const NCSProto::TDBBackgroundTask& proto);
            NCSProto::TDBBackgroundTask SerializeCommonToProto() const;
        public:
            TDBBackgroundTask() = default;
            TDBBackgroundTask(const IRTQueueTask& base)
                : TBase(base) {

            }

            class TDecoder: public TBaseDecoder {
            private:
                DECODER_FIELD(SequenceTaskId);
                DECODER_FIELD(InternalTaskId);
                DECODER_FIELD(ClassName);
                DECODER_FIELD(Data);
                DECODER_FIELD(CommonData);

                DECODER_FIELD(OwnerId);
                DECODER_FIELD(QueueId);
                DECODER_FIELD(StartInstant);
                DECODER_FIELD(ConstructionInstant);
                DECODER_FIELD(LastPingInstant);
                DECODER_FIELD(CurrentHost);
            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase) {
                    SequenceTaskId = GetFieldDecodeIndex("sequence_task_id", decoderBase);
                    InternalTaskId = GetFieldDecodeIndex("internal_task_id", decoderBase);
                    ClassName = GetFieldDecodeIndex("class_name", decoderBase);
                    Data = GetFieldDecodeIndex("data", decoderBase);
                    CommonData = GetFieldDecodeIndex("common_data", decoderBase);

                    OwnerId = GetFieldDecodeIndex("owner_id", decoderBase);
                    QueueId = GetFieldDecodeIndex("queue_id", decoderBase);
                    StartInstant = GetFieldDecodeIndex("start_instant", decoderBase);
                    ConstructionInstant = GetFieldDecodeIndex("construction_instant", decoderBase);
                    LastPingInstant = GetFieldDecodeIndex("last_ping_instant", decoderBase);
                    CurrentHost = GetFieldDecodeIndex("current_host", decoderBase);
                }
            };
            Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

            NStorage::TTableRecord SerializeToTableRecord() const;
        };
    }
}
