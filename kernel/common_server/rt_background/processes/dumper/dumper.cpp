#include "dumper.h"
#include "meta_parser.h"

#include <library/cpp/yson/node/node_io.h>

namespace NCS {

    bool IRTDumper::ProcessRecords(const TRecordsSetWT& records, const ITableFieldViewer& tableViewer, const bool dryRun) const noexcept {
        try {
            NYT::TTableSchema tableYtSchema;
            if (TYtProcessTraits::HasYtSchema()) {
                tableYtSchema = TYtProcessTraits::GetYtSchema();
            } else {
                tableYtSchema = tableViewer.GetYtSchema();
            }
            if (!DoProcessRecords(records, tableViewer, tableYtSchema, dryRun)) {
                TFLEventLog::Error("IRTDumper::DoProcessRecords returned false").Signal("rt_dumper")("&code", "process_records_failed");
                return false;
            }
            TFLEventLog::JustSignal("rt_dumper", records.size())("&code", "processed_records");
        } catch (...) {
            TFLEventLog::Error("Failed to process records by RTDumper")("exception_message", CurrentExceptionMessage()).Signal("rt_dumper")("&code", "process_records_failed");
            return false;
        }
        return true;
    }

    namespace NHelpers {
        void DumpToYtNode(const NStorage::TTableRecordWT& tableRecord, NYT::TNode& recordNode,
                          const ITableFieldViewer& tableViewer) {
            for (auto&& [key, value] : tableRecord) {
                TString valueStr;
                if (NCS::NStorage::TDBValueOperator::TryGetString(value, valueStr)) {
                    const char first = valueStr ? valueStr[0] : '\0';
                    if (first == '{' || first == '[') {
                        NJson::TJsonValue json;
                        if (NJson::ReadJsonFastTree(valueStr, &json)) {
                            try {
                                recordNode[key] = NYT::NodeFromJsonValue(json);
                                continue;
                            } catch (...) {
                                TFLEventLog::Warning("Cannot convert Json to Yson")("json_value", json)("exception_message", CurrentExceptionMessage());
                            }
                        }
                    }
                }
                IDumperMetaParser::TPtr metaParser = tableViewer.Construct(key);
                if (metaParser) {
                    if (metaParser->NeedFullRecord()) {
                        recordNode["unpacked_data"] = NYT::NodeFromJsonValue(metaParser->ParseMeta(tableRecord));
                    } else {
                        recordNode["_unpacked_" + key] = NYT::NodeFromJsonValue(metaParser->ParseMeta(value));
                        recordNode["unpacked_data"] = NYT::NodeFromJsonValue(metaParser->ParseMeta(value));
                    }
                } else {
                    recordNode[key] = NYT::NodeFromJsonValue(NCS::NStorage::TDBValueOperator::SerializeToJson(value));
                }
            }
        }
    } // namespace NHelpers

} // namespace NCS
