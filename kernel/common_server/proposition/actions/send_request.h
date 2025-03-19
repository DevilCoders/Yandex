#pragma once

#include <kernel/common_server/library/json/parse.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/proposition/actions/abstract.h>
#include <kernel/common_server/library/tvm_services/abstract/request/direct.h>

namespace NCS {
    namespace NPropositions {

        using THeadersMap = TMap<TString, TString>;

        class TSendRequestAction: public IJsonProposedAction {
        private:
            CSA_DEFAULT(TSendRequestAction, TString, RequestName);
            CSA_DEFAULT(TSendRequestAction, TString, SenderName);
            CSA_DEFAULT(TSendRequestAction, TString, RequestURI);
            CSA_DEFAULT(TSendRequestAction, TString, RequestType);
            CSA_DEFAULT(TSendRequestAction, TString, RequestBody);
            CSA_MUTABLE_DEF(TSendRequestAction, THeadersMap, RequestHeaders);
            CSA_MUTABLE_DEF(TSendRequestAction, TString, RequestResult);

        public:
            TSendRequestAction() = default;

            virtual NJson::TJsonValue SerializeToJson() const override;

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) override;

            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override;

            virtual TString GetObjectId() const override;

            static TString GetTypeName() {
                return "send_request";
            }

            virtual TString GetCategoryId() const override;

            virtual TString GetClassName() const override;

            virtual bool IsActual(const IBaseServer& server) const override;

            virtual TString GetResult() const override;

            virtual bool TuneAction(NCS::TEntitySession& session, IReplyContext::TPtr requestContext) const override;

        protected:
            virtual bool DoExecute(const TString& userId, const IBaseServer& server) const override;
        };

    }
}
