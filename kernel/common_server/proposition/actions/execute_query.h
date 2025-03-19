#pragma once
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/proposition/actions/abstract.h>

namespace NCS {
    namespace NPropositions {

        class TExecuteQueryAction: public IProposedActionWithDelay {
        private:
            using TBase = IProposedActionWithDelay;
            CSA_DEFAULT(TExecuteQueryAction, TString, DBName);
            CSA_DEFAULT(TExecuteQueryAction, TString, QueryName);
            CSA_DEFAULT(TExecuteQueryAction, TString, QueryText);
            CSA_MUTABLE_DEF(TExecuteQueryAction, TString, QueryResult);

        public:
            TExecuteQueryAction() = default;

            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = TBase::SerializeToJson();
                JWRITE(result, "db_name", DBName);
                JWRITE(result, "query_name", QueryName);
                JWRITE(result, "query_text", QueryText);
                return result;
            }

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) override {
                JREAD_STRING_OPT(jsonData, "db_name", DBName);
                JREAD_STRING_OPT(jsonData, "query_name", QueryName);
                JREAD_STRING_OPT(jsonData, "query_text", QueryText);
                return TBase::DeserializeFromJson(jsonData);
            }

            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override {
                NCS::NScheme::TScheme scheme = TBase::GetScheme(server);
                scheme.Add<TFSVariants>("db_name", "База данных").SetVariants(server.GetDatabaseNames());
                scheme.Add<TFSString>("query_name", "Идентификатор запроса").Required();
                scheme.Add<TFSString>("query_text", "Текст sql запроса").MultiLine().Required();
                return scheme;
            }

            virtual TString GetObjectId() const override {
                return QueryName;
            }

            static TString GetTypeName() {
                return "execute_query";
            }

            virtual TString GetCategoryId() const override {
                return GetTypeName();
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            virtual TString GetResult() const override {
                return QueryResult;
            }

        protected:
            virtual bool DoExecute(const TString& userId, const IBaseServer& server) const override;
        };

    }
}
