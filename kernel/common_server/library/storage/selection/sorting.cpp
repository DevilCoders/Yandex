#include "sorting.h"
#include <kernel/common_server/library/storage/records/db_value.h>

namespace NCS {
    namespace NSelection {
        namespace NSorting {

            NCS::NSelection::NSorting::TLinear& TLinear::RegisterField(const TString& fieldId, const bool ascending /*= true*/) {
                NStorage::NRequest::TFieldOrder fOrder;
                fOrder.SetFieldId(fieldId).SetAscending(ascending);
                Fields.emplace_back(std::move(fOrder));
                return *this;
            }


            NCS::NSelection::NSorting::TLinear& TLinear::RegisterField(const NStorage::NRequest::TFieldOrder& fOrder) {
                Fields.emplace_back(fOrder);
                return *this;
            }

            void TLinear::FillScheme(NCS::NScheme::TScheme& scheme) const {
                NCS::NScheme::TScheme currentFields;
                for (auto&& i : Fields) {
                    currentFields.Add<TFSStructure>(i.GetFieldId()).SetStructure(i.GetScheme());
                }
                if (!currentFields.IsEmpty()) {
                    scheme.Add<TFSStructure>("sort_fields").SetStructure(currentFields);
                }
            }

            bool TLinear::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                TMap<TString, NStorage::NRequest::TFieldOrder> fieldsMap;
                if (jsonInfo["sort_fields"].IsArray()) {
                    TVector<NStorage::NRequest::TFieldOrder> fields;
                    if (!TJsonProcessor::ReadObjectsContainer(jsonInfo, "sort_fields", fields)) {
                        return false;
                    }
                    for (auto&& i : fields) {
                        fieldsMap[i.GetFieldId()] = i;
                    }
                } else {
                    if (!TJsonProcessor::ReadObjectsMapSimple(jsonInfo, "sort_fields", fieldsMap)) {
                        return false;
                    }
                }
                TVector<NStorage::NRequest::TFieldOrder> fields;
                size_t matchesCount = 0;
                for (auto&& i : Fields) {
                    auto it = fieldsMap.find(i.GetFieldId());
                    if (it == fieldsMap.end()) {
                        fields.emplace_back(i);
                    } else {
                        ++matchesCount;
                        fields.emplace_back(it->second);
                    }
                }
                if (fieldsMap.size() != matchesCount) {
                    TFLEventLog::Error("invalid lists for fields")("available_fields", MakeSet(NContainer::Keys(fieldsMap)));
                    return false;
                }
                std::swap(Fields, fields);
                return true;
            }

            void TLinear::FillSorting(TSRSelect& srSelect) const {
                if (Fields.empty()) {
                    return;
                }
                TSROrder& srOrder = srSelect.RetOrderBy<TSROrder>();
                srOrder.SetFields(Fields);
            }
        }
    }
}
