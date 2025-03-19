#include "group.h"
#include "controller.h"

namespace NCS {

    bool TDBSnapshotsGroup::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        READ_DECODER_VALUE(decoder, values, GroupId);
        if (!GroupId) {
            return false;
        }
        READ_DECODER_VALUE(decoder, values, DefaultSnapshotCode);
        READ_DECODER_VALUE(decoder, values, Revision);
        NJson::TJsonValue jsonData;
        if (!decoder.GetJsonValue(decoder.GetData(), values, jsonData)) {
            return false;
        }
        if (!DeserializeCommonFromJson(jsonData)) {
            return false;
        }
        return true;
    }

    bool TDBSnapshotsGroup::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        if (!TJsonProcessor::Read(jsonInfo, "group_id", GroupId)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "default_snapshot_code", DefaultSnapshotCode)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
            return false;
        }
        if (!DeserializeCommonFromJson(jsonInfo["data"])) {
            return false;
        }
        return true;
    }

    NJson::TJsonValue TDBSnapshotsGroup::SerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_MAP;
        result.InsertValue("group_id", GroupId);
        result.InsertValue("default_snapshot_code", DefaultSnapshotCode);
        result.InsertValue("revision", Revision);
        result.InsertValue("data", SerializeCommonToJson());
        return result;
    }

    NStorage::TTableRecord TDBSnapshotsGroup::SerializeToTableRecord() const {
        NStorage::TTableRecord result;
        result.SetNotEmpty("group_id", GroupId);
        result.SetNotEmpty("default_snapshot_code", DefaultSnapshotCode);
        result.SetNotEmpty("revision", Revision);
        result.Set("data", SerializeCommonToJson());
        return result;
    }

    NFrontend::TScheme TDBSnapshotsGroup::GetScheme(const IBaseServer& server) {
        NFrontend::TScheme result;
        result.Add<TFSString>("group_id");
        result.Add<TFSString>("default_snapshot_code");
        result.Add<TFSNumeric>("revision");
        auto& fsData = result.Add<TFSStructure>("data").SetStructure();
        fsData.Add<TFSStructure>("remove_policy").SetStructure(TSnapshotsGroupRemovePolicyContainer::GetScheme(server), false);
        fsData.Add<TFSStructure>("cleaning_policy").SetStructure(TSnapshotsGroupCleaningPolicyContainer::GetScheme(server), false);
        fsData.Add<TFSStructure>("objects_manager").SetStructure(NSnapshots::TObjectsManagerContainer::GetScheme(server));
        fsData.Add<TFSStructure>("diff_policy").SetStructure(NCS::TSnapshotsDiffPolicyContainer::GetScheme(server)).SetRequired(false);
        return result;
    }

}