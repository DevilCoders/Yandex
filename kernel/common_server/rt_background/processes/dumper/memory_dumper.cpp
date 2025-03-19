#include "memory_dumper.h"

namespace NCS {

    TVector<TString>& GetMemoryDumperStorage() {
        static auto memoryDumperStorage = new TVector<TString>;
        return *memoryDumperStorage;
    }

    TMemoryDumper::TFactory::TRegistrator<TMemoryDumper> TMemoryDumper::Registrator(TMemoryDumper::GetTypeName());

    NFrontend::TScheme TMemoryDumper::GetScheme(const IBaseServer& server) const {
        Y_UNUSED(server);
        NFrontend::TScheme scheme;
        return scheme;
    }

    bool TMemoryDumper::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        Y_UNUSED(jsonInfo);
        return true;
    }

    NJson::TJsonValue TMemoryDumper::SerializeToJson() const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        return result;
    }

    bool TMemoryDumper::DoProcessRecords(const TRecordsSetWT& records, const ITableFieldViewer& tableViewer,
                                         const NYT::TTableSchema& tableYtSchema, const bool dryRun) const {
        Y_UNUSED(tableYtSchema);
        Y_UNUSED(dryRun);
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
            GetMemoryDumperStorage().push_back(jsonRecord.GetStringRobust());
        }
        return true;
    }

} // namespace NCS
