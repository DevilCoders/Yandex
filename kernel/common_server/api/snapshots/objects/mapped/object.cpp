#include "object.h"

namespace NCS {
    namespace NSnapshots {

        void TMappedObject::FilterColumns(const TSet<TString>& fieldIds) {
            Values.FilterColumns(fieldIds);
        }

        NCS::NScheme::TScheme TMappedObject::GetScheme(const IBaseServer& /*server*/) {
            NCS::NScheme::TScheme result;
            result.Add<TFSString>("values").SetVisual(TFSString::EVisualType::Json);
            return result;
        }

        TMappedObject::TMappedObject(const NStorage::TTableRecord& tRecord) {
            Values = tRecord.BuildWT();
        }

        TMappedObject::TMappedObject(const NStorage::TTableRecordWT& tRecord) {
            Values = tRecord;
        }

        bool TMappedObject::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            for (ui32 i = 0; i < decoder.GetColumns().size(); ++i) {
                if (i < (ui32)values.size()) {
                    if (!Values.Set(decoder.GetColumns()[i].GetName(), values[i], decoder.GetColumns()[i].GetType(), true)) {
                        TFLEventLog::Error("incompatible types")("index", i)("column_id", decoder.GetColumns()[i].GetName())("expected_type", decoder.GetColumns()[i].GetType())("real_value", values[i]);
                        return false;
                    }
                } else {
                    TFLEventLog::Error("incompatible value index")("index", i)("column_id", decoder.GetColumns()[i].GetName());
                    return false;
                }
            }
            return true;
        }

        NStorage::TTableRecord TMappedObject::SerializeToTableRecord() const {
            return Values.BuildTableRecord();
        }

        bool TMappedObject::DeserializeFromYTNode(const NYT::TNode& row, const TMap<TString, TString>& factorNamesRemapping, const TStructure& structure) {
            if (!row.IsMap()) {
                TFLEventLog::Error("yt row is not map");
                return false;
            }
            TMap<TString, TString> factorNamesRemappingBack;
            for (auto&& i : factorNamesRemapping) {
                if (!factorNamesRemappingBack.emplace(i.second, i.first).second) {
                    TFLEventLog::Error("incorrect factors remapper")("duplication", i.second);
                    return false;
                }
            }
            const auto& fields = row.AsMap();
            NStorage::TTableRecordWT values;
            for (auto&& i : structure.GetFields()) {
                auto itRemap = factorNamesRemappingBack.find(i.GetId());
                auto it = fields.find((itRemap == factorNamesRemappingBack.end()) ? i.GetId() : itRemap->second);
                if (it == fields.end()) {
                    TFLEventLog::Error("field not exists")("field_id", i.GetId());
                    return false;
                }
                TString normalized;
                if (i.IsNullable() && it->second.IsNull()) {
                    continue;
                }
                if (!structure.Normalize(i.GetId(), it->second.ConvertTo<TString>(), normalized)) {
                    TFLEventLog::Error("field cannot be normalized")("field_id", i.GetId())("value", it->second.ConvertTo<TString>());
                    return false;
                }
                auto dbValue = i.GetDBValue(normalized);
                if (!dbValue) {
                    return false;
                }
                values.Set(i.GetId(), *dbValue);
            }
            std::swap(values, Values);
            return true;
        }

    }
}
