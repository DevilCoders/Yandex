#include "object.h"
#include "controller.h"

namespace NCS {

    bool TDBSnapshot::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        READ_DECODER_VALUE(decoder, values, SnapshotCode);
        READ_DECODER_VALUE(decoder, values, SnapshotId);
        READ_DECODER_VALUE(decoder, values, SnapshotGroupId);
        READ_DECODER_VALUE(decoder, values, Revision);
        READ_DECODER_VALUE(decoder, values, Enabled);
        READ_DECODER_VALUE(decoder, values, Status);
        READ_DECODER_VALUE_INSTANT(decoder, values, LastStatusModification);

        {
            NJson::TJsonValue jsonInfo;
            if (!decoder.GetJsonValue(decoder.GetSnapshotContext(), values, jsonInfo)) {
                TFLEventLog::Error("cannot read snapshot context");
                return false;
            }
            if (!SnapshotContext.DeserializeFromJson(jsonInfo, true)) {
                TFLEventLog::Error("cannot parse snapshot context")("snapshot_id", SnapshotId)("snapshot_group_id", SnapshotGroupId);
                return false;
            }
        }

        {
            NJson::TJsonValue jsonInfo;
            if (!decoder.GetJsonValue(decoder.GetFetchingContext(), values, jsonInfo)) {
                TFLEventLog::Error("cannot read fetching context for snapshots");
                return false;
            }
            if (!FetchingContext.DeserializeFromJson(jsonInfo, true)) {
                TFLEventLog::Error("cannot parse fetching context for snapshot")("snapshot_id", SnapshotId)("snapshot_group_id", SnapshotGroupId);
                return false;
            }
        }

        {
            NJson::TJsonValue jsonInfo;
            if (!decoder.GetJsonValue(decoder.GetContentFetcher(), values, jsonInfo)) {
                TFLEventLog::Error("cannot read content fetcher for snapshots");
                return false;
            }
            if (!ContentFetcher.DeserializeFromJson(jsonInfo, true)) {
                TFLEventLog::Error("cannot parse content fethcher for snapshot")("snapshot_id", SnapshotId)("snapshot_group_id", SnapshotGroupId);
                return false;
            }
        }

        return true;
    }

    NStorage::TTableRecord TDBSnapshot::SerializeToTableRecord() const {
        NStorage::TTableRecord result;
        result.SetNotEmpty("snapshot_code", SnapshotCode);
        result.SetNotEmpty("snapshot_id", SnapshotId);
        result.SetNotEmpty("snapshot_group_id", SnapshotGroupId);
        result.SetNotEmpty("revision", Revision);
        result.Set("enabled", Enabled);
        result.Set("status", Status);
        result.Set("last_status_modification", LastStatusModification);
        result.Set("fetching_context", FetchingContext.SerializeToJson());
        result.Set("content_fetcher", ContentFetcher.SerializeToJson());
        result.Set("snapshot_context", SnapshotContext.SerializeToJson());
        return result;
    }

    NFrontend::TScheme TDBSnapshot::GetScheme(const IBaseServer& server) {
        NFrontend::TScheme result;
        result.Add<TFSString>("snapshot_code");
        result.Add<TFSNumeric>("snapshot_id");
        result.Add<TFSVariants>("snapshot_group_id").SetVariants(server.GetSnapshotsController().GetGroupsManager().GetObjectNames());
        result.Add<TFSNumeric>("revision").SetReadOnly(true);
        result.Add<TFSBoolean>("enabled");
        result.Add<TFSVariants>("status").InitVariants<ESnapshotStatus>();
        result.Add<TFSNumeric>("last_status_modification").SetVisual(TFSNumeric::EVisualTypes::DateTime);
        result.Add<TFSString>("fetching_context").SetVisual(TFSString::EVisualType::Json);
        result.Add<TFSString>("content_fetcher").SetVisual(TFSString::EVisualType::Json);
        result.Add<TFSString>("snapshot_context").SetVisual(TFSString::EVisualType::Json);
        return result;
    }

    NJson::TJsonValue TDBSnapshot::SerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        result.InsertValue("snapshot_code", SnapshotCode);
        result.InsertValue("snapshot_id", SnapshotId);
        result.InsertValue("snapshot_group_id", SnapshotGroupId);
        result.InsertValue("revision", Revision);
        result.InsertValue("enabled", Enabled);
        result.InsertValue("status", ::ToString(Status));
        result.InsertValue("last_status_modification", LastStatusModification.Seconds());
        result.InsertValue("fetching_context", FetchingContext.SerializeToJson());
        result.InsertValue("content_fetcher", ContentFetcher.SerializeToJson());
        result.InsertValue("snapshot_context", SnapshotContext.SerializeToJson());
        return result;
    }

    class TCompareMappedObject {
    private:
        CSA_READONLY_DEF(TString, ObjectId);
        const NSnapshots::TMappedObject* Object;
    public:
        TCompareMappedObject() = default;

        TCompareMappedObject(const TString& objectId, const NSnapshots::TMappedObject* object)
            : ObjectId(objectId)
            , Object(object)
        {

        }

        const NSnapshots::TMappedObject* operator->() const {
            return Object;
        }

        const NSnapshots::TMappedObject& operator*() const {
            return *Object;
        }

        bool operator< (const TCompareMappedObject& item) const {
            return ObjectId < item.ObjectId;
        }
    };

    bool ISnapshotsDiffPolicy::Compare(const TDBSnapshotsGroup& g, const TVector<NSnapshots::TMappedObject>& oldObjects, const TVector<NSnapshots::TMappedObject>& newObjects, const IBaseServer& bServer) const {
        auto comparator = BuildComparator(g, bServer);
        if (!comparator) {
            return false;
        }
        if (!comparator->Start()) {
            TFLEventLog::Error("cannot start comparator");
            return false;
        }

        TVector<TCompareMappedObject> oldObjectsPtr;
        for (auto& i : oldObjects) {
            oldObjectsPtr.emplace_back(comparator->BuildObjectId(i), &i);
        }
        TVector<TCompareMappedObject> newObjectsPtr;
        for (auto& i : newObjects) {
            newObjectsPtr.emplace_back(comparator->BuildObjectId(i), &i);
        }
        std::sort(oldObjectsPtr.begin(), oldObjectsPtr.end());
        std::sort(newObjectsPtr.begin(), newObjectsPtr.end());

        auto itOld = oldObjectsPtr.begin();
        auto itNew = newObjectsPtr.begin();
        while (itOld != oldObjectsPtr.end() || itNew != newObjectsPtr.end()) {
            if (itOld == oldObjectsPtr.end()) {
                if (!comparator->OnChangeObject(**itNew, ISnapshotsComparator::EObjectChangeEvent::New)) {
                    TFLEventLog::Error("cannot notify new object")("object_id", itNew->GetObjectId());
                    return false;
                }
                ++itNew;
            } else if (itNew == newObjectsPtr.end()) {
                if (!comparator->OnChangeObject(**itOld, ISnapshotsComparator::EObjectChangeEvent::Removed)) {
                    TFLEventLog::Error("cannot notify removed object")("object_id", itOld->GetObjectId());
                    return false;
                }
                ++itOld;
            } else if (*itOld < *itNew) {
                if (!comparator->OnChangeObject(**itOld, ISnapshotsComparator::EObjectChangeEvent::Removed)) {
                    TFLEventLog::Error("cannot notify removed object")("object_id", itOld->GetObjectId());
                    return false;
                }
                ++itOld;
            } else if (*itNew < *itOld) {
                if (!comparator->OnChangeObject(**itNew, ISnapshotsComparator::EObjectChangeEvent::New)) {
                    TFLEventLog::Error("cannot notify new object")("object_id", itNew->GetObjectId());
                    return false;
                }
                ++itNew;
            } else {
                if (!comparator->OnChangeObject(**itNew, ISnapshotsComparator::EObjectChangeEvent::Equal)) {
                    TFLEventLog::Error("cannot notify equal object")("object_id", itNew->GetObjectId());
                    return false;
                }
                ++itOld;
                ++itNew;
            }
        }

        if (!comparator->Finish()) {
            TFLEventLog::Error("cannot finish comparator");
            return false;
        }

        return true;
    }

    ISnapshotsComparator::ISnapshotsComparator(const TDBSnapshotsGroup& g, const IBaseServer& bServer)
        : Server(bServer)
        , SnapshotsGroup(g) {

    }

}
