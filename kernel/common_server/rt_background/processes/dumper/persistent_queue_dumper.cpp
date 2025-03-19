#include "persistent_queue_dumper.h"

namespace NCS {

    TPQDumper::TFactory::TRegistrator<TPQDumper>
        TPQDumper::Registrator(TPQDumper::GetTypeName());

    NFrontend::TScheme TPQDumper::GetScheme(const IBaseServer& server) const {
        Y_UNUSED(server);
        NFrontend::TScheme scheme;
        scheme.Add<TFSString>("service_id", "Имя очереди").SetRequired(true);
        scheme.Add<TFSNumeric>("buffer_size", "Размер буффера сообщений (определяет частоту флашинга сообщений)").SetDefault(500u).SetRequired(true);
        scheme.Add<TFSString>("message_id_field", "Название поля, используемого в качестве message_id")
            .SetDefault("history_event_id")
            .SetRequired(true);
        return scheme;
    }

    bool TPQDumper::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        JREAD_STRING(jsonInfo, "service_id", ServiceID);
        if (!TJsonProcessor::Read(jsonInfo, "buffer_size", BufferSize)) {
            return false;
        }
        JREAD_STRING(jsonInfo, "message_id_field", MessageIDField);
        return true;
    }

    NJson::TJsonValue TPQDumper::SerializeToJson() const {
        NJson::TJsonValue result;
        JWRITE(result, "service_id", ServiceID);
        JWRITE(result, "buffer_size", BufferSize);
        JWRITE(result, "message_id_field", MessageIDField);
        return result;
    }

    bool TPQDumper::DoProcessRecords(const TRecordsSetWT& records, const ITableFieldViewer& tableViewer,
                                     const NYT::TTableSchema& tableYtSchema, const bool dryRun) const {
        Y_UNUSED(tableYtSchema);
        Y_UNUSED(tableViewer);
        Y_UNUSED(dryRun);

        const auto clientPtr = tableViewer.GetServer().GetQueuePtr(ServiceID);
        if (!clientPtr) {
            TFLEventLog::Error("Client for given service_id is not defined")("service_id", ServiceID).Signal("persistent_queue_dumper")("&code", "incorrect_client");
            return false;
        }
        size_t messageCount = 0;
        for (auto&& record : records) {
            NJson::TJsonValue jsonRecord = NJson::JSON_MAP;
            for (auto&& i : record) {
                IDumperMetaParser::TPtr metaParser = tableViewer.Construct(i.first);
                if (metaParser) {
                    if (metaParser->NeedFullRecord()) {
                        jsonRecord["unpacked_data"] = metaParser->ParseMeta(record);
                    } else {
                        jsonRecord["_unpacked_" + i.first] = metaParser->ParseMeta(i.second);
                    }
                } else {
                    jsonRecord[i.first] = NCS::NStorage::TDBValueOperator::SerializeToJson(i.second);
                }
            }
            auto blobMessage = TBlob::FromString(jsonRecord.GetStringRobust());
            if (blobMessage.Length() > kMessageSizeLimitInBytes) {
                TFLEventLog::Error("Message will be skipped: size is greater than the limit")("current_limit", kMessageSizeLimitInBytes)("message_size", blobMessage.Length())
                    .Signal("persistent_queue_dumper")("&code", "message_size_exceeds_limit");
                continue;
            }
            THolder<NCS::IPQMessage> pqMessage = MakeHolder<NCS::TPQMessageSimple>(std::move(blobMessage), record.GetString(MessageIDField));
            if (!clientPtr->WriteMessage(pqMessage.Release()).IsSuccess()) {
                TFLEventLog::Error("Failed to write message").Signal("persistent_queue_dumper")("&code", "write_failed");
                return false;
            }
            ++messageCount;
            if (messageCount == BufferSize) {
                if (!clientPtr->FlushWritten()) {
                    TFLEventLog::Error("Failed to flush written messages").Signal("persistent_queue_dumper")("&code", "flush_failed");
                    return false;
                };
                messageCount = 0;
            }
        }
        if (!clientPtr->FlushWritten()) {
            TFLEventLog::Error("Failed to flush written messages").Signal("persistent_queue_dumper")("&code", "flush_failed");
            return false;
        };
        return true;
    }

} // namespace NCS
