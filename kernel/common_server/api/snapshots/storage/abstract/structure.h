#pragma once
#include "index.h"
#include <kernel/common_server/library/storage/query/create_table.h>
#include <kernel/common_server/api/normalization/abstract/normalizer.h>

namespace NCS {
    namespace NSnapshots {
        class TFieldNormalization {
        private:
            CSA_DEFAULT(TFieldNormalization, TString, ColumnId);
            CSA_DEFAULT(TFieldNormalization, NNormalizer::TStringNormalizerContainer, Normalizer);
        public:
            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                if (!TJsonProcessor::Read(jsonInfo, "column_id", ColumnId)) {
                    return false;
                }
                if (!TJsonProcessor::ReadObject(jsonInfo, "normalizer", Normalizer)) {
                    return false;
                }
                return true;
            }

            NJson::TJsonValue SerializeToJson() const {
                NJson::TJsonValue result = NJson::JSON_MAP;
                TJsonProcessor::Write(result, "column_id", ColumnId);
                TJsonProcessor::WriteObject(result, "normalizer", Normalizer);
                return result;
            }

            static NCS::NScheme::TScheme GetScheme(const IBaseServer& server) {
                NCS::NScheme::TScheme result;
                result.Add<TFSString>("column_id");
                result.Add<TFSStructure>("normalizer").SetStructure(NNormalizer::TStringNormalizerContainer::GetScheme(server));
                return result;
            }

        };

        class TStructure {
        private:
            CSA_DEFAULT(TStructure, TVector<NStorage::TColumnInfo>, Fields);
            CSA_DEFAULT(TStructure, TVector<TFieldNormalization>, Normalizers);
            CSA_DEFAULT(TStructure, TVector<TIndex>, Indexes);
        public:
            TSet<TString> GetFieldIds() const;
            NStorage::TTableRecordWT FilterColumns(NStorage::TTableRecordWT&& tr) const {
                tr.FilterColumns(GetFieldIds());
                return std::move(tr);
            }

            const TFieldNormalization* GetNormalizerInfo(const TStringBuf sbFieldName) const {
                for (auto&& i : Normalizers) {
                    if (i.GetColumnId() == sbFieldName) {
                        return &i;
                    }
                }
                return nullptr;
            }

            bool Normalize(const TStringBuf sbFieldName, const TStringBuf baseFieldValue, TString& result) const {
                auto info = GetNormalizerInfo(sbFieldName);
                if (!info) {
                    result = ::ToString(baseFieldValue);
                    return true;
                } else {
                    return info->GetNormalizer().Normalize(baseFieldValue, result);
                }
            }

            NStorage::TColumnInfo& AddField(const TString& fieldId);
            TIndex& AddIndex(const TString& indexId);
            bool CheckFields(const TVector<TStringBuf>& sbFields) const;
            const NStorage::TColumnInfo* GetFieldInfo(const TString& fieldId) const;

            TMaybe<TIndex> SearchIndex(const NStorage::TTableRecordWT& tr, const bool uniqueOnly, NStorage::TTableRecordWT* filteredTr) const;

            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);

            NJson::TJsonValue SerializeToJson() const;

            static NCS::NScheme::TScheme GetScheme(const IBaseServer& server);
        };
    }
}
