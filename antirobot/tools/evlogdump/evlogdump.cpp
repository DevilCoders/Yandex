/**
 * Usage:
 * ./unified_agent select -s event_log_storage --no-split -d '' -f | ./antirobot_evlogdump | grep -v "TCbbRulesUpdated" | grep -v "TCacherFactors" | grep -v "New CBB block rules" | grep -v "TAntirobotFactors"
 * ./unified_agent select -s event_log_storage --no-split -d '' -f | ./antirobot_evlogdump | grep 'Level":2'
 */

#include <antirobot/tools/evlogdump/lib/evlog_descriptors.h>

#include <google/protobuf/messagext.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/dynamic_message.h>

#include <library/cpp/framing/unpacker.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/generic/yexception.h>
#include <util/string/cast.h>


namespace {
    inline constexpr size_t PROTOSEQ_FRAME_LEN_BYTES = sizeof(ui32);
    inline constexpr size_t MAGIC_SEQUENCE_LEN = 32;

    struct TMessageData {
        const NProtoBuf::Message* Prototype = nullptr;
        TString Name;
    };
}

THashMap<ui32, TMessageData> GetMessageIdToPrototype() {
    const auto& evlogTypeNameToDescriptor = NAntiRobot::GetEvlogTypeNameToDescriptor();
    const auto messageFactory = NProtoBuf::DynamicMessageFactory::generated_factory();

    THashMap<ui32, TMessageData> messageIdToPrototype;
    for (const auto& [name, descriptor] : evlogTypeNameToDescriptor) {
        const ui32 messageId = descriptor->options().GetExtension(message_id);
        messageIdToPrototype[messageId] = {
            .Prototype = messageFactory->GetPrototype(descriptor),
            .Name = name,
        };
    }
    return messageIdToPrototype;
}

void ProcessRecord(const NAntirobotEvClass::TProtoseqRecord& record) {
    const auto& eventBytes = record.Getevent();
    const NAntiRobot::TEvlogEvent event(eventBytes);

    static const auto messageIdToPrototype = GetMessageIdToPrototype();
    const TMessageData* data = messageIdToPrototype.FindPtr(event.MessageId);
    if (!data) {
        Cerr << "WARNING: Can't find proto with message_id=" + ToString(event.MessageId) << Endl;
        return;
    }

    THolder<NProtoBuf::Message> message{data->Prototype->New()};
    Y_ENSURE(
        message->ParseFromArray(event.MessageBytes.data(), event.MessageBytes.size()),
        "Failed to parse message"
    );
    Cout << record.Gettimestamp() << "\t" << data->Name << "\t" << NProtobufJson::Proto2Json(*message) << Endl;
}

bool ProcessProtoseqStream(IInputStream& in) {
    char sizeBuf[PROTOSEQ_FRAME_LEN_BYTES];
    size_t bytesCount = in.Load(sizeBuf, PROTOSEQ_FRAME_LEN_BYTES);
    if (bytesCount == 0) {
        return false;
    }
    Y_ENSURE(bytesCount == PROTOSEQ_FRAME_LEN_BYTES);

    google::protobuf::io::CodedInputStream stream(reinterpret_cast<const ui8*>(sizeBuf), PROTOSEQ_FRAME_LEN_BYTES);
    ui32 dataLen;
    Y_ENSURE(stream.ReadLittleEndian32(&dataLen));

    TTempBuf buf(PROTOSEQ_FRAME_LEN_BYTES + dataLen + MAGIC_SEQUENCE_LEN);
    buf.Append(sizeBuf, PROTOSEQ_FRAME_LEN_BYTES);
    Y_ENSURE(dataLen + MAGIC_SEQUENCE_LEN == in.Load(buf.Current(), dataLen + MAGIC_SEQUENCE_LEN));
    buf.Proceed(dataLen + MAGIC_SEQUENCE_LEN);

    NFraming::TUnpacker unpacker(NFraming::EFormat::Protoseq, TStringBuf(buf.Data(), buf.Filled()));
    NAntirobotEvClass::TProtoseqRecord record;
    TStringBuf skip; // will contains broken part of original message
    while (unpacker.NextFrame(record, skip)) {
        ProcessRecord(record);
        Y_ENSURE(skip.Size() == 0);
    }
    return true;
}

int main(int argc, char** argv) {
    Y_UNUSED(argc, argv);
    while (ProcessProtoseqStream(Cin)) {
    }
    return 0;
}
