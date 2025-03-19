#pragma once
#include "context.h"
#include <kernel/common_server/api/snapshots/fetching/abstract/content.h>
#include <mapreduce/yt/interface/common.h>
#include <library/cpp/yson/node/node.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/client_method_options.h>
#include <kernel/common_server/api/snapshots/controller.h>
#include <kernel/common_server/api/snapshots/object.h>
#include <kernel/common_server/api/snapshots/objects/mapped/object.h>

namespace NCS {
    class TYTSnapshotFetcher: public ISnapshotContentFetcher {
    private:
        using TFactorNamesRemapping = TMap<TString, TString>;
        CSA_DEFAULT(TYTSnapshotFetcher, TString, YTCluster);
        CSA_DEFAULT(TYTSnapshotFetcher, TString, YTPath);
        CSA_DEFAULT(TYTSnapshotFetcher, TString, SnapshotSelectorId);
        CSA_DEFAULT(TYTSnapshotFetcher, TString, SnapshotSelectorValue);
        CSA_DEFAULT(TYTSnapshotFetcher, TFactorNamesRemapping, FactorNamesRemapping);
        CS_ACCESS(TYTSnapshotFetcher, ui32, ContextStoringPackSize, 50000);
        static TFactory::TRegistrator<TYTSnapshotFetcher> Registrator;
    protected:
        bool FetchCrawledData(const NSnapshots::TObjectsManagerContainer& soManager, const ISnapshotsController& controller, TVector<NSnapshots::TMappedObject>& objects, ui64& snapshotSize, ui32& errorsCount, const ui32 idx, const TDBSnapshot& snapshot, const TString& userId) const {
            if (objects.empty()) {
                return true;
            }
            if (!soManager->PutObjects(objects, snapshot.GetSnapshotId(), userId)) {
                TFLEventLog::Warning("cannot store snapshot objects")("objects_count", objects.size());
                errorsCount += objects.size();
            } else {
                snapshotSize += objects.size();
            }
            auto context = MakeHolder<TYTSnapshotFetcherContext>();
            context->SetErrorsCount(errorsCount);
            context->SetStartTableIndex(idx + 1);
            context->SetSnapshotSize(snapshotSize);
            if (!controller.StoreSnapshotFetcherContext(snapshot.GetSnapshotCode(), context.Release())) {
                TFLEventLog::Error("cannot store snapshot fetcher context");
                return false;
            }
            objects.clear();
            return true;
        }

        virtual NJson::TJsonValue DoSerializeToJson() const override {
            NJson::TJsonValue result = NJson::JSON_MAP;
            result.InsertValue("yt_cluster", YTCluster);
            result.InsertValue("yt_path", YTPath);
            result.InsertValue("snapshot_selector_id", SnapshotSelectorId);
            result.InsertValue("snapshot_selector_value", SnapshotSelectorValue);
            result.InsertValue("context_storing_pack_size", ContextStoringPackSize);

            TJsonProcessor::Write(result, "factor_names_remapping", FactorNamesRemapping);

            return result;
        }
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
            JREAD_STRING(jsonInfo, "yt_cluster", YTCluster);
            JREAD_STRING(jsonInfo, "yt_path", YTPath);
            JREAD_STRING_OPT(jsonInfo, "snapshot_selector_id", SnapshotSelectorId);
            JREAD_STRING_OPT(jsonInfo, "snapshot_selector_value", SnapshotSelectorValue);
            JREAD_INT_OPT(jsonInfo, "context_storing_pack_size", ContextStoringPackSize);
            if (ContextStoringPackSize == 0) {
                TFLEventLog::Error("incorrect context storing pack size")("context_storing_pack_size", ContextStoringPackSize);
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "factor_names_remapping", FactorNamesRemapping)) {
                return false;
            }
            return true;
        }
        virtual bool DoFetchData(const TDBSnapshot& snapshotInfo, const ISnapshotsController& controller, const IBaseServer& /*server*/, const TString& userId) const override {
            TAtomicSharedPtr<TYTSnapshotFetcherContext> pContext = snapshotInfo.GetFetchingContext().GetPtrAs<TYTSnapshotFetcherContext>();
            if (!!snapshotInfo.GetFetchingContext() && !pContext) {
                TFLEventLog::Error("incorrect context class for fetcher")("incoming_context", snapshotInfo.GetFetchingContext().SerializeToJson());
                return false;
            }

            ui64 startTableIndex = 0;
            ui32 errorsCount = 0;
            ui64 snapshotSize = 0;
            if (!!pContext) {
                startTableIndex = pContext->GetStartTableIndex();
                errorsCount = pContext->GetErrorsCount();
                snapshotSize = pContext->GetSnapshotSize();
            }
            auto soManager = controller.GetSnapshotObjectsManager(snapshotInfo);
            if (!soManager) {
                TFLEventLog::Error("cannot fetch snapshot objects manager")("snapshot_code", snapshotInfo.GetSnapshotCode());
                return false;
            }

            NYT::TCreateClientOptions cco;
            NYT::IClientPtr ytClient = NYT::CreateClient(YTCluster);
            NYT::TRichYPath richYPath(YTPath);
            if (SnapshotSelectorValue) {
                ui64 selectorValue;
                if (!TryFromString<ui64>(SnapshotSelectorValue, selectorValue)) {
                    TFLEventLog::Error("cannot parse value as ui64")("value", SnapshotSelectorValue);
                    richYPath.AddRange(NYT::TReadRange().Exact(NYT::TReadLimit().Key({ SnapshotSelectorValue })));
                } else {
                    richYPath.AddRange(NYT::TReadRange().Exact(NYT::TReadLimit().Key({ selectorValue })));
                }
            }

            auto reader = ytClient->CreateTableReader<NYT::TNode>(richYPath);
            TSnapshotContextContainer sContext = snapshotInfo.GetSnapshotContext();
            ui64 idx = 0;
            ui64 count = 0;
            TVector<NSnapshots::TMappedObject> objectPacks;
            for (; reader->IsValid(); reader->Next()) {
                idx = reader->GetRowIndex();
                if (startTableIndex > reader->GetRowIndex()) {
                    continue;
                }
                try {
                    const NYT::TNode& row = reader->GetRow();
                    if (SnapshotSelectorValue && row[SnapshotSelectorId].ConvertTo<TString>() != SnapshotSelectorValue) {
                        continue;
                    }
                    ++count;
                    NSnapshots::TMappedObject snapshotObject;
                    if (!snapshotObject.DeserializeFromYTNode(row, FactorNamesRemapping, soManager->GetStructure())) {
                        TFLEventLog::Signal("fetch_yt_data")("&code", "not_parsed");
                        ++errorsCount;
                        continue;
                    }

                    objectPacks.emplace_back(std::move(snapshotObject));
                } catch (yexception& e) {
                    TFLEventLog::Log("cannot parse yt node")("reason", CurrentExceptionMessage());
                    return false;
                }
                if (count % ContextStoringPackSize == 0) {
                    if (!FetchCrawledData(soManager, controller, objectPacks, snapshotSize, errorsCount, idx, snapshotInfo, userId)) {
                        return false;
                    }
                }
            }
            TFLEventLog::Error("cannot parse yt node")("rows_count", errorsCount);
            if (!FetchCrawledData(soManager, controller, objectPacks, snapshotSize, errorsCount, idx, snapshotInfo, userId)) {
                return false;
            }
            return true;
        }
    public:
        static TString GetTypeName() {
            return "yt_fetcher";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };
}
