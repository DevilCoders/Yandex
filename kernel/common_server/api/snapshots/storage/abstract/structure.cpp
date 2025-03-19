#include "structure.h"

namespace NCS {
    namespace NSnapshots {

        TSet<TString> TStructure::GetFieldIds() const {
            TSet<TString> result;
            for (auto&& i : Fields) {
                result.emplace(i.GetId());
            }
            return result;
        }

        NCS::NStorage::TColumnInfo& TStructure::AddField(const TString& fieldId) {
            Fields.emplace_back(NStorage::TColumnInfo().SetId(fieldId));
            return Fields.back();
        }

        NCS::NSnapshots::TIndex& TStructure::AddIndex(const TString& indexId) {
            Indexes.emplace_back(TIndex().SetIndexId(indexId));
            return Indexes.back();
        }

        bool TStructure::CheckFields(const TVector<TStringBuf>& sbFields) const {
            TSet<TString> fields;
            for (auto&& i : sbFields) {
                fields.emplace(TString(i.data(), i.size()));
                if (!GetFieldInfo(::ToString(i))) {
                    TFLEventLog::Error("cannot find field")("field_id", i);
                    return false;
                }
            }
            for (auto&& i : Fields) {
                if (!i.IsNullable()) {
                    if (!fields.contains(i.GetId())) {
                        TFLEventLog::Error("field cannot be nullable")("field_id", i.GetId());
                        return false;
                    }
                }
            }
            return true;
        }

        const NCS::NStorage::TColumnInfo* TStructure::GetFieldInfo(const TString& fieldId) const {
            for (auto&& i : Fields) {
                if (i.GetId() == fieldId) {
                    return &i;
                }
            }
            return nullptr;
        }

        TMaybe<NCS::NSnapshots::TIndex> TStructure::SearchIndex(const NStorage::TTableRecordWT& tr, const bool uniqueOnly, NStorage::TTableRecordWT* filteredTr) const {
            for (auto&& i : Indexes) {
                if (uniqueOnly && !i.IsUnique()) {
                    continue;
                }
                if (i.Match(tr, filteredTr)) {
                    return i;
                }
            }
            return Nothing();
        }

        bool TStructure::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::ReadObjectsContainer(jsonInfo, "fields", Fields, true, false, false)) {
                return false;
            }
            if (!TJsonProcessor::ReadObjectsContainer(jsonInfo, "normalizers", Normalizers)) {
                return false;
            }
            if (!TJsonProcessor::ReadObjectsContainer(jsonInfo, "indexes", Indexes, true, false, false)) {
                return false;
            }
            const auto pred = [](const TIndex& i1, const TIndex& i2) {
                return std::tuple(i1.IsUnique() ? 0 : 1, i1.GetOrderedFields().size()) < std::tuple(i2.IsUnique() ? 0 : 1, i2.GetOrderedFields().size());
            };
            TSet<TString> fields;
            for (auto&& i : Fields) {
                fields.emplace(i.GetId());
            }
            std::sort(Indexes.begin(), Indexes.end(), pred);
            for (auto&& i : Indexes) {
                for (auto&& f : i.GetOrderedFields()) {
                    if (!fields.contains(f)) {
                        TFLEventLog::Error("incorrect field in index")("index_id", i.GetIndexId())("field", f);
                        return false;
                    }
                }
            }
            for (auto&& i : Normalizers) {
                if (!fields.contains(i.GetColumnId())) {
                    TFLEventLog::Error("incorrect field in normalizer")("column_id", i.GetColumnId());
                    return false;
                }
            }
            return true;
        }

        NJson::TJsonValue TStructure::SerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TJsonProcessor::WriteObjectsContainer(result, "fields", Fields);
            TJsonProcessor::WriteObjectsContainer(result, "indexes", Indexes);
            TJsonProcessor::WriteObjectsContainer(result, "normalizers", Normalizers);
            return result;
        }

        NCS::NScheme::TScheme TStructure::GetScheme(const IBaseServer& server) {
            NCS::NScheme::TScheme result;
            result.Add<TFSArray>("fields").SetElement<TFSStructure>(NCS::NStorage::TColumnInfo::GetScheme());
            result.Add<TFSArray>("indexes").SetElement<TFSStructure>(TIndex::GetScheme(server));
            result.Add<TFSArray>("normalizers").SetElement<TFSStructure>(TFieldNormalization::GetScheme(server));
            return result;
        }

    }
}
