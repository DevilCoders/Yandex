#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/util/cgi_processing.h>
#include <kernel/common_server/library/storage/records/db_value.h>
#include <kernel/common_server/library/storage/records/record.h>
#include <kernel/common_server/util/algorithm/container.h>
#include <library/cpp/digest/md5/md5.h>

namespace NCS {
    namespace NSelection {
        namespace NSorting {
            class TLinear {
            private:
                CSA_READONLY_DEF(TVector<NStorage::NRequest::TFieldOrder>, Fields);
            public:
                TLinear& RegisterField(const TString& fieldId, const bool ascending = true);
                TLinear& RegisterField(const NStorage::NRequest::TFieldOrder& fOrder);
                void FillScheme(NCS::NScheme::TScheme& scheme) const;
                bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
                void FillSorting(TSRSelect& srSelect) const;
                template <class TObject>
                bool Compare(const TObject& l, const TObject& r) const {
                    auto trL = l.SerializeToTableRecord().BuildWT();
                    auto trR = r.SerializeToTableRecord().BuildWT();
                    for (auto&& i : Fields) {
                        auto lValue = trL.GetValue(i.GetFieldId());
                        auto rValue = trR.GetValue(i.GetFieldId());
                        if (lValue == rValue) {
                            continue;
                        }
                        if (i.IsAscending()) {
                            return lValue < rValue;
                        } else {
                            return rValue  < lValue;
                        }
                    }
                    return false;
                }
            };
        }
    }
}
