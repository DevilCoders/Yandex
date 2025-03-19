#pragma once

#include <chrono>

#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/util/json_processing.h>

namespace NCS::NStartrek {
    class TObjectBrief {
        CSA_READONLY_DEF(TString, SelfUrl);
        CSA_READONLY_DEF(TString, Id);
        CSA_READONLY_DEF(TString, Key);
        CSA_READONLY_DEF(TString, Display);
    public:
        TObjectBrief() = default;
        virtual ~TObjectBrief() = default;

        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& json);
        virtual NJson::TJsonValue SerializeToJson() const;
    };

    class TBaseIssue {
        CSA_PROTECTED_DEF(TBaseIssue, TString, Summary);
        CSA_PROTECTED_DEF(TBaseIssue, TString, Description);
        CSA_PROTECTED_DEF(TBaseIssue, TString, ParentIssue);
        CSA_PROTECTED_DEF(TBaseIssue, TString, Type);
    public:
        TBaseIssue() = default;
        virtual ~TBaseIssue() = default;

        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& json);
        virtual NJson::TJsonValue SerializeToJson() const;
    };

    class TDetailedIssue: public TObjectBrief, public TBaseIssue {
        CSA_READONLY_DEF(TString, CreatedAt);
        CSA_READONLY_DEF(TString, CreatedById);
        CSA_READONLY_DEF(TVector<TString>, ReviewersIds);
        CSA_READONLY_DEF(TString, AssigneeId);
        CSA_READONLY_DEF(TSet<TString>, Tags);
        CSA_READONLY_DEF(TString, StatusKey);
        CSA_READONLY_DEF(TString, QueueKey);
    public:
        TDetailedIssue() = default;
        TString GetIssue() {
            return TObjectBrief::GetKey();
        }
        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& json) override;
        virtual NJson::TJsonValue SerializeToJson() const override;
    };

    class TComment: public TObjectBrief {
        CSA_READONLY_DEF(TString, LongId);
        CSA_READONLY_DEF(TString, Text);
        CSA_READONLY_DEF(TString, CreatedAt);
        CSA_READONLY_DEF(TString, CreatedByLoginId);
        CSA_READONLY_DEF(TVector<TString>, SummoneesLoginIds);
    public:
        TComment() = default;

        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& json) override;
        virtual NJson::TJsonValue SerializeToJson() const override;
    };
}
