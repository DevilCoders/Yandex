#include "yt_dumper.h"

#include <library/cpp/yson/node/node_io.h>
#include <mapreduce/yt/interface/client.h>

#include <kernel/common_server/library/yt/common/writer.h>

namespace NCS {

    TYTDumper::TFactory::TRegistrator<TYTDumper> TYTDumper::Registrator(TYTDumper::GetTypeName());

    NFrontend::TScheme TYTDumper::GetScheme(const IBaseServer& server) const {
        Y_UNUSED(server);
        NFrontend::TScheme scheme;
        TYtProcessTraits::FillScheme(scheme);
        scheme.Add<TFSString>("yt_cluster", "Кластер YT").SetDefault("hahn").SetRequired(true);
        scheme.Add<TFSString>("yt_dir", "Директория для экспорта").SetRequired(true);
        scheme.Add<TFSBoolean>("optimize_for_scan", "Устанавливать параметр optimize for в scan");
        return scheme;
    }

    bool TYTDumper::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        if (!TYtProcessTraits::Deserialize(jsonInfo)) {
            return false;
        }
        JREAD_STRING(jsonInfo, "yt_cluster", YTCluster);
        JREAD_STRING(jsonInfo, "yt_dir", YTDir);
        JREAD_BOOL_OPT(jsonInfo, "optimize_for_scan", OptimizeForScanFlag);
        return true;
    }

    NJson::TJsonValue TYTDumper::SerializeToJson() const {
        NJson::TJsonValue result;
        SerializeToJson(result);
        return result;
    }

    void TYTDumper::SerializeToJson(NJson::TJsonValue& result) const {
        TYtProcessTraits::Serialize(result);
        JWRITE(result, "yt_dir", YTDir);
        JWRITE(result, "yt_cluster", YTCluster);
        JWRITE(result, "optimize_for_scan", OptimizeForScanFlag);
    }

    bool TYTDumper::DoProcessRecords(const TRecordsSetWT& records, const ITableFieldViewer& tableViewer,
                                     const NYT::TTableSchema& tableYtSchema, const bool dryRun) const {
        NYT::IClientPtr ytClient = NYT::CreateClient(YTCluster);
        TYTWritersSet<TTableSelectorDay> writers(ytClient, YTDir, tableYtSchema, OptimizeForScanFlag);
        for (auto&& record : records) {
            NYT::TNode recordNode;
            NCS::NHelpers::DumpToYtNode(record, recordNode, tableViewer);
            recordNode = Schematize(std::move(recordNode), tableYtSchema);
            writers.GetWriter(Now())->AddRow(recordNode);
        }
        TFLEventLog::Info("dryRun value")("dry_run_enabled", dryRun);
        if (!dryRun) {
            writers.Finish();
        }
        return true;
    }

} // namespace NCS
