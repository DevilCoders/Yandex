#include "index.h"
#include <kernel/common_server/util/algorithm/container.h>

namespace NCS {
    namespace NSnapshots {

        TSet<TString> TIndex::GetFieldsSet() const {
            return MakeSet(OrderedFields);
        }

        NCS::NScheme::TScheme TIndex::GetScheme(const IBaseServer& /*server*/) {
            NCS::NScheme::TScheme result;
            result.Add<TFSString>("index_id").SetNonEmpty(true);
            result.Add<TFSBoolean>("unique").SetDefault(false);
            result.Add<TFSArray>("ordered_fields").SetElement<TFSString>();
            return result;
        }

        bool TIndex::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::Read(jsonInfo, "index_id", IndexId, true, false)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "unique", UniqueFlag)) {
                return false;
            }
            if (!TJsonProcessor::ReadContainer(jsonInfo, "ordered_fields", OrderedFields, true, false, false)) {
                return false;
            }
            return true;
        }
        NJson::TJsonValue TIndex::SerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TJsonProcessor::Write(result, "index_id", IndexId);
            TJsonProcessor::Write(result, "unique", UniqueFlag);
            TJsonProcessor::WriteContainerArray(result, "ordered_fields", OrderedFields);
            return result;
        }

        bool TIndex::Build(const NStorage::TTableRecordWT& base, NStorage::TTableRecordWT& result) const {
            NStorage::TTableRecordWT resultLocal;
            for (auto&& i : OrderedFields) {
                auto v = base.GetValue(i);
                if (!v) {
                    TFLEventLog::Error("cannot build condition by object table record")("field_id", i);
                    return false;
                }
                resultLocal.Set(i, *v);
            }
            std::swap(resultLocal, result);
            return true;
        }

        bool TIndex::Match(const NStorage::TTableRecordWT& tr, NStorage::TTableRecordWT* filteredTr) const {
            NStorage::TTableRecordWT recordValues;
            for (auto&& i : OrderedFields) {
                auto v = tr.GetValue(i);
                if (!v) {
                    return false;
                }
                if (filteredTr) {
                    recordValues.Set(i, *v);
                }
            }
            if (filteredTr) {
                *filteredTr = std::move(recordValues);
            }
            return true;
        }

    }
}
