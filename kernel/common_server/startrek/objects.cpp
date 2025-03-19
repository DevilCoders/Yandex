#include "objects.h"

namespace NCS::NStartrek {
    bool TObjectBrief::DeserializeFromJson(const NJson::TJsonValue& json) {
        JREAD_STRING_OPT(json, "self", SelfUrl);
        JREAD_STRING_OPT(json, "key", Key);
        JREAD_STRING_OPT(json, "display", Display);
        if (json.Has("id") && (json["id"].IsString() || json["id"].IsUInteger())) {
            Id = json["id"].GetStringRobust();
        }
        return true;
    }

    NJson::TJsonValue TObjectBrief::SerializeToJson() const {
        NJson::TJsonValue result = NJson::TJsonMap();
        JWRITE(result, "self", SelfUrl);
        if (Id) {
            JWRITE(result, "id", Id);
        }
        if (Key) {
            JWRITE(result, "key", Key);
        }
        if (Display) {
            JWRITE(result, "display", Display);
        }
        return result;
    }

    NJson::TJsonValue TBaseIssue::SerializeToJson() const {
        NJson::TJsonValue result = NJson::TJsonMap();
        JWRITE(result, "summary", Summary);
        JWRITE(result, "description", Description);
        JWRITE(result, "parent", ParentIssue);
        JWRITE(result, "type", Type);
        return result;
    }


    bool TBaseIssue::DeserializeFromJson(const NJson::TJsonValue& json) {
        JREAD_STRING_OPT(json, "summary", Summary);
        JREAD_STRING_OPT(json, "description", Description);
        JREAD_STRING_OPT(json, "parent", ParentIssue);
        JREAD_STRING_OPT(json, "type", Type);
        return true;
    }

    bool TDetailedIssue::DeserializeFromJson(const NJson::TJsonValue& json) {
        JREAD_STRING_OPT(json, "summary", Summary);
        JREAD_STRING_OPT(json, "description", Description);
        JREAD_STRING_OPT(json, "createdAt", CreatedAt);
        if (json.Has("createdBy")) {
            JREAD_STRING_OPT(json["createdBy"], "id", AssigneeId);
        }
        TVector<TObjectBrief> reviewers;
        if (!TJsonProcessor::ReadObjectsContainer(json, "reviewers", reviewers)) {
            return false;
        }
        for (auto&& reviewer : reviewers) {
            if (auto& login = reviewer.GetId()){
                ReviewersIds.emplace_back(login);
            }
        }
        JREAD_CONTAINER_OPT(json, "tags", Tags);
        if (json.Has("assignee")) {
            JREAD_STRING_OPT(json["assignee"], "id", AssigneeId)
        }
        if (json.Has("status")) {
            JREAD_STRING_OPT(json["status"], "key", StatusKey)
        }
        if (json.Has("queue")) {
            JREAD_STRING_OPT(json["queue"], "key", QueueKey)
        }
        return TObjectBrief::DeserializeFromJson(json) && TBaseIssue::DeserializeFromJson(json);
    }

    NJson::TJsonValue TDetailedIssue::SerializeToJson() const {
        NJson::TJsonValue result = TObjectBrief::SerializeToJson();
        JWRITE(result, "summary", TBaseIssue::GetSummary());
        JWRITE(result, "description", TBaseIssue::GetDescription());
        JWRITE(result, "type", TBaseIssue::GetType())
        JWRITE(result, "parent", TBaseIssue::GetParentIssue())

        JWRITE(result, "createdAt", CreatedAt);
        JWRITE(result, "resolvedBy", CreatedById);
        JWRITE(result, "assigneeId", AssigneeId);
        JWRITE(result, "statusKey", StatusKey);
        JWRITE(result, "queueKey", QueueKey);
        TJsonProcessor::WriteContainerString(result, "reviewersIds", ReviewersIds);
        TJsonProcessor::WriteContainerString(result, "tags", Tags);
        return result;
    }

    bool TComment::DeserializeFromJson(const NJson::TJsonValue& json) {
        JREAD_STRING_OPT(json, "longId", LongId);
        JREAD_STRING_OPT(json, "text", Text);
        JREAD_STRING_OPT(json, "createdAt", CreatedAt);
        if (json.Has("createdBy")) {
            JREAD_STRING_OPT(json["createdBy"], "id", CreatedByLoginId)
        }
        TVector<TObjectBrief> summonees;
        if (!TJsonProcessor::ReadObjectsContainer(json, "summonees", summonees)) {
            return false;
        }
        for (auto&& summonee : summonees) {
            if (auto& login = summonee.GetId()) {
                SummoneesLoginIds.emplace_back(login);
            }
        }
        return TObjectBrief::DeserializeFromJson(json);
    }

    NJson::TJsonValue TComment::SerializeToJson() const {
        NJson::TJsonValue result = TObjectBrief::SerializeToJson();
        JWRITE(result, "longId", LongId);
        JWRITE(result, "createdAt", CreatedAt);
        JWRITE(result, "createdById", CreatedByLoginId);
        JWRITE(result, "text", Text);

        TJsonProcessor::WriteContainerString(result, "summonees", SummoneesLoginIds);
        return result;
    }
}
