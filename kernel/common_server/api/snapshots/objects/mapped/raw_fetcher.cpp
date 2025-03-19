#include "raw_fetcher.h"
#include <kernel/common_server/api/snapshots/abstract.h>
#include <util/string/vector.h>
#include "object.h"

namespace NCS {
    namespace NSnapshots {
        TRecordsSnapshotFetcher::TFactory::TRegistrator<TRecordsSnapshotFetcher> TRecordsSnapshotFetcher::Registrator(TRecordsSnapshotFetcher::GetTypeName());

        bool TRecordsSnapshotFetcher::DoFetchData(const TDBSnapshot& snapshotInfo, const ISnapshotsController& controller, const IBaseServer& /*server*/, const TString& userId) const {
            {
                auto session = controller.GetSnapshotsManager().BuildNativeSession(false);
                if (!controller.GetSnapshotsManager().UpdateForConstruction(snapshotInfo.GetSnapshotId(), "raw_fetcher", session) || !session.Commit()) {
                    TFLEventLog::Error("cannot set snapshot status as ready");
                    return false;
                }
            }
            TObjectsManagerContainer manager = controller.GetSnapshotObjectsManager(snapshotInfo);
            if (!manager) {
                TFLEventLog::Error("incorrect snapshot objects manager");
                return false;
            }
            if (!UpsertDiffMode) {
                if (!manager->RemoveSnapshot(snapshotInfo.GetSnapshotId())) {
                    TFLEventLog::Error("cannot remove snapshot");
                    return false;
                }
                if (!manager->CreateSnapshot(snapshotInfo.GetSnapshotId())) {
                    TFLEventLog::Error("cannot create snapshot");
                    return false;
                }
            }

            TStringBuf sb(RawData.AsCharPtr(), RawData.Length());

            TStringBuf firstLine;
            TStringBuf sbData;
            if (!sb.TrySplit("\r\n", firstLine, sbData) && !sb.TrySplit('\n', firstLine, sbData)) {
                TFLEventLog::Error("cannot read first line");
                return false;
            }
            TVector<TStringBuf> sbFields;
            {
                sbFields = StringSplitter(firstLine).SplitByString(Delimiter).ToList<TStringBuf>();
                if (!manager->GetStructure().CheckFields(sbFields)) {
                    TFLEventLog::Error("cannot map key/values indexes by first line")("first_line", firstLine);
                    return false;
                }
            }

            TStringBuf currentLine;
            TStringBuf sbDataNext;
            ui32 idx = 0;
            TVector<TMappedObject> objects;
            while (!!sbData) {
                if (!sbData.TrySplit("\r\n", currentLine, sbDataNext) && !sbData.TrySplit('\n', currentLine, sbDataNext)) {
                    currentLine = sbData;
                    sbDataNext = TStringBuf();
                }
                ++idx;
                auto gLogging = TFLRecords::StartContext()("line index", idx)("line", currentLine)("first_line", firstLine);
                const TVector<TStringBuf> sbVector = StringSplitter(currentLine).SplitByString(Delimiter).ToList<TStringBuf>();
                if (sbVector.size() != sbFields.size()) {
                    TFLEventLog::Error("no full data in records");
                    return false;
                }
                NStorage::TTableRecordWT tr;
                for (ui32 i = 0; i < sbVector.size(); ++i) {
                    TString normalized;
                    if (!manager->GetStructure().Normalize(sbFields[i], sbVector[i], normalized)) {
                        return false;
                    }
                    auto columnInfo = manager->GetStructure().GetFieldInfo(TString(sbFields[i]));
                    if (!columnInfo) {
                        TFLEventLog::Error("incorrect column on parsing")("column_id", sbFields[i]);
                        return false;
                    }
                    if (!columnInfo->WriteValue(tr, normalized)) {
                        TFLEventLog::Error("incorrect column value")("column_id", sbFields[i])("value", normalized);
                        return false;
                    }
                }
                objects.emplace_back(tr);
                sbData = sbDataNext;
            }
            if (!UpsertDiffMode) {
                if (!manager->PutObjects(objects, snapshotInfo.GetSnapshotId(), userId)) {
                    TFLEventLog::Error("cannot put objects");
                    return false;
                }
            } else {
                if (!manager->UpsertObjects(objects, snapshotInfo.GetSnapshotId(), userId)) {
                    TFLEventLog::Error("cannot upsert objects");
                    return false;
                }
            }
            return true;
        }

    }
}
