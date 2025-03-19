#include "set.h"
#include <util/string/join.h>

namespace NCS {
    namespace NStorage {
        TRecordsSet TRecordsSet::FromVector(const TVector<TTableRecord>& records) {
            TRecordsSet result;
            for (auto&& i : records) {
                result.AddRow(i);
            }
            return result;
        }

        TRecordsSet TRecordsSet::FromVector(const TVector<TTableRecordWT>& records) {
            TRecordsSet result;
            for (auto&& i : records) {
                result.AddRow(i.BuildTableRecord());
            }
            return result;
        }

        TSet<TString> TRecordsSet::GetAllFieldNames() const {
            TSet<TString> result;
            for (auto&& i : Records) {
                for (auto&& f : i) {
                    result.emplace(f.first);
                }
            }
            return result;
        }

        TString TRecordsSet::BuildCondition(const NRequest::IExternalMethods& transaction) const {
            TVector<TString> conditions;

            for (auto&& i : Records) {
                conditions.emplace_back(i.BuildCondition(transaction));
            }

            if (conditions.empty()) {
                return "";
            }
            return "(" + JoinSeq(") OR (", conditions) + ")";
        }

        TSet<TString> TRecordsSetWT::SelectSet(const TString& fieldId) const {
            TSet<TString> result;
            for (auto&& i : Records) {
                result.emplace(i.GetString(fieldId));
            }
            return result;
        }

    }
}
