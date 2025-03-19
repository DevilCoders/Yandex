#pragma once
#include <kernel/common_server/library/storage/reply/decoder.h>
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>

namespace NCS {
    namespace NPQ {
        namespace NDatabase {
            class TObject {
            private:
                using TBase = TBaseInterfaceContainer<IPQMessage>;
                CSA_DEFAULT(TObject, TString, MessageId);
                CSA_DEFAULT(TObject, TBlob, Content);
                CSA_DEFAULT(TObject, TInstant, ConstructionInstant);
                CSA_DEFAULT(TObject, TInstant, LastPingInstant);
                CSA_DEFAULT(TObject, TString, CurrentQueueAgentId);
            public:
                TObject() = default;
                TObject(const IPQMessage& message) {
                    Content = message.GetContent();
                    MessageId = message.GetMessageId();
                }

                class TDecoder: public TBaseDecoder {
                private:
                    DECODER_FIELD(MessageId);
                    DECODER_FIELD(Content);

                    DECODER_FIELD(ConstructionInstant);
                    DECODER_FIELD(LastPingInstant);
                    DECODER_FIELD(CurrentQueueAgentId);
                public:
                    TDecoder() = default;
                    TDecoder(const TMap<TString, ui32>& decoderBase) {
                        MessageId = GetFieldDecodeIndex("message_id", decoderBase);
                        Content = GetFieldDecodeIndex("content", decoderBase);
                        ConstructionInstant = GetFieldDecodeIndex("construction_instant", decoderBase);
                        LastPingInstant = GetFieldDecodeIndex("last_ping_instant", decoderBase);
                        CurrentQueueAgentId = GetFieldDecodeIndex("current_queue_agent_id", decoderBase);
                    }
                };
                Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

                NStorage::TTableRecord SerializeToTableRecord() const;
            };
        }
    }
}
