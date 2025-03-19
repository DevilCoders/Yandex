#pragma once
#include "scheme.h"

namespace NCS {
    namespace NScheme {
        class TOperationActivity {
        private:
            CSA_DEFAULT(TOperationActivity, TString, UriSuffix);
            CSA_DEFAULT(TOperationActivity, TString, DisplayText);
        public:
            TOperationActivity() = default;
            NJson::TJsonValue SerializeToJson() const {
                NJson::TJsonValue result = NJson::JSON_MAP;
                result.InsertValue("uri_suffix", UriSuffix);
                result.InsertValue("display_text", DisplayText);
                return result;
            }
        };

        class TConstantOperationInfo {
        private:
            CSA_DEFAULT(TConstantOperationInfo, TString, EntityId);
            CSA_MAYBE(TConstantOperationInfo, TScheme, Scheme);
            CSA_MAYBE(TConstantOperationInfo, TScheme, SearchScheme);
            CS_ACCESS(TConstantOperationInfo, bool, SearchSchemeRequired, false);
            CS_ACCESS(TConstantOperationInfo, i32, SortPriority, 0);
            CSA_DEFAULT(TConstantOperationInfo, TString, Title);
            CSA_DEFAULT(TConstantOperationInfo, TString, ObjectId);
            CSA_DEFAULT(TConstantOperationInfo, TString, HandlersPrefix);
            CSA_READONLY_DEF(TSet<TString>, Links);
            CSA_READONLY_DEF(TVector<TOperationActivity>, Activities);
        public:
            TConstantOperationInfo& InitDefaultActivities() {
                Activities.clear();
                AddActivity("Удалить", "remove");
                AddActivity("Сохранить", "upsert");
                return *this;
            }

            TOperationActivity& AddActivityRet(const TString& name, const TString& uriSuffix) {
                TOperationActivity act;
                act.SetDisplayText(name).SetUriSuffix(uriSuffix);
                Activities.emplace_back(act);
                return Activities.back();
            }

            TConstantOperationInfo& AddActivity(const TString& name, const TString& uriSuffix) {
                AddActivityRet(name, uriSuffix);
                return *this;
            }

            TConstantOperationInfo& AddLink(const TString& linkedObjectId) {
                Links.emplace(linkedObjectId);
                return *this;
            }

            bool operator!() const {
                return !Scheme;
            }

            template <class T, class TServer>
            TConstantOperationInfo& InitScheme(const TServer& server) {
                Scheme = T::GetScheme(server);
                return *this;
            }

            template <class T, class TServer>
            TConstantOperationInfo& InitSearchScheme(const TServer& server) {
                SearchScheme = T::GetSearchScheme(server);
                return *this;
            }

            NJson::TJsonValue SerializeToJson() const;
        };

        class TConstantsInfoReport {
        private:
            TMap<TString, TConstantOperationInfo> EntityOperationInfo;
        public:
            TConstantOperationInfo& Register(const TString& entityId, const i32 priority = 0) {
                auto it = EntityOperationInfo.find(entityId);
                if (it != EntityOperationInfo.end()) {
                    return it->second;
                }
                TConstantOperationInfo newOperationInfo;
                newOperationInfo.SetEntityId(entityId);
                newOperationInfo.SetSortPriority(priority);
                auto insertInfo = EntityOperationInfo.emplace(entityId, newOperationInfo);
                CHECK_WITH_LOG(insertInfo.second);
                return insertInfo.first->second;
            }

            bool Has(const TString& entityId) const {
                return EntityOperationInfo.contains(entityId);
            }

            NJson::TJsonValue GetReportMap(const TSet<TString>& entityIds = {}) const {
                NJson::TJsonValue jsonOperationInfo = NJson::JSON_MAP;
                for (auto&& i : EntityOperationInfo) {
                    if (!i.second) {
                        continue;
                    }
                    if (entityIds.size() && !entityIds.contains(i.first)) {
                        continue;
                    }
                    jsonOperationInfo.InsertValue(i.first, i.second.SerializeToJson());
                }
                return jsonOperationInfo;
            }

            NJson::TJsonValue GetReportArray(const TSet<TString>& entityIds = {}) const {
                NJson::TJsonValue jsonOperationInfo = NJson::JSON_ARRAY;
                TVector<const TConstantOperationInfo*> operations;
                for (auto&& i : EntityOperationInfo) {
                    operations.emplace_back(&i.second);
                }
                const auto pred = [](const TConstantOperationInfo* l, const TConstantOperationInfo* r) {
                    return l->GetSortPriority() < r->GetSortPriority();
                };
                std::sort(operations.begin(), operations.end(), pred);
                for (auto&& i : operations) {
                    if (!i || !*i) {
                        continue;
                    }
                    if (entityIds.size() && !entityIds.contains(i->GetEntityId())) {
                        continue;
                    }
                    jsonOperationInfo.AppendValue(i->SerializeToJson());
                }
                return jsonOperationInfo;
            }
        };
    }
}
