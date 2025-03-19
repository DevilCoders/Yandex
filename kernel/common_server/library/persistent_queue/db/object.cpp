#include "object.h"

namespace NCS {
    namespace NPQ {
        namespace NDatabase {

            bool TObject::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
                READ_DECODER_VALUE(decoder, values, MessageId);
                TString contentStr;
                if (!decoder.GetValueBytes(decoder.GetContent(), values, contentStr)) {
                    return false;
                }
                Content = TBlob::FromString(std::move(contentStr));
                READ_DECODER_VALUE_INSTANT(decoder, values, ConstructionInstant);
                READ_DECODER_VALUE_INSTANT(decoder, values, LastPingInstant);
                READ_DECODER_VALUE(decoder, values, CurrentQueueAgentId);
                return true;
            }

            NStorage::TTableRecord TObject::SerializeToTableRecord() const {
                NStorage::TTableRecord result;
                result.SetNotEmpty("message_id", MessageId);
                result.SetBytes("content", Content.AsStringBuf());
                result.Set("construction_instant", ConstructionInstant.Seconds());
                result.Set("last_ping_instant", LastPingInstant.Seconds());
                result.SetNotEmpty("current_queue_agent_id", CurrentQueueAgentId);
                return result;
            }

        }
    }
}
