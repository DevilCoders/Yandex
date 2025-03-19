#pragma once
#include <kernel/common_server/library/scheme/scheme.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/storage/records/t_record.h>

namespace NCS {
    namespace NSnapshots {
        class TIndex {
        private:
            CSA_DEFAULT(TIndex, TString, IndexId);
            CSA_FLAG(TIndex, Unique, false);
            CSA_DEFAULT(TIndex, TVector<TString>, OrderedFields);
        public:
            TSet<TString> GetFieldsSet() const;
            static NCS::NScheme::TScheme GetScheme(const IBaseServer& /*server*/);

            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);

            NJson::TJsonValue SerializeToJson() const;

            bool operator< (const TIndex& item) const {
                return IndexId < item.GetIndexId();
            }
            bool Build(const NStorage::TTableRecordWT& base, NStorage::TTableRecordWT& result) const;
            bool Match(const NStorage::TTableRecordWT& tr, NStorage::TTableRecordWT* filteredTr) const;
        };
    }
}
