#include "operations.h"
#include <util/string/split.h>
#include <kernel/common_server/util/json_processing.h>

namespace NCS {
    namespace NScheme {

        NJson::TJsonValue TConstantOperationInfo::SerializeToJson() const {
            NJson::TJsonValue report = NJson::JSON_MAP;
            report.InsertValue("entity_id", EntityId);
            report.InsertValue("sort_priority", SortPriority);
            if (!!Title) {
                auto& jsonTitle = report.InsertValue("title", NJson::JSON_ARRAY);
                for (auto&& i : StringSplitter(Title).Split('.').SkipEmpty().ToList<TString>()) {
                    jsonTitle.AppendValue(i);
                }
            }
            if (!!ObjectId || !!Title) {
                auto& jsonTitle = report.InsertValue("objectId", NJson::JSON_ARRAY);
                for (auto&& i : StringSplitter(ObjectId ? ObjectId : Title).Split('.').SkipEmpty().ToList<TString>()) {
                    jsonTitle.AppendValue(i);
                }
            }
            if (!!Scheme) {
                report.InsertValue("scheme", Scheme->SerializeToJson());
            }
            if (!!SearchScheme) {
                report.InsertValue("searchScheme", SearchScheme->SerializeToJson());
            }
            if (!!HandlersPrefix) {
                report.InsertValue("handlersPrefix", HandlersPrefix);
            }
            TJsonProcessor::WriteContainerArray(report, "links", Links, false);
            report.InsertValue("searchSchemeRequired", SearchSchemeRequired);
            if (Activities.size()) {
                TJsonProcessor::WriteObjectsArray(report, "activities", Activities);
            } else {
                TVector<TOperationActivity> activitiesDefault;
                activitiesDefault.emplace_back(TOperationActivity().SetUriSuffix("remove").SetDisplayText("Удалить"));
                activitiesDefault.emplace_back(TOperationActivity().SetUriSuffix("upsert").SetDisplayText("Сохранить"));
                TJsonProcessor::WriteObjectsArray(report, "activities", activitiesDefault);
            }

            return report;
        }

    }
}
