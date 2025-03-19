#include "execute_query.h"

namespace NCS {
    namespace NPropositions {
        IProposedAction::TFactory::TRegistrator<TExecuteQueryAction> RegistratorExecuteQuery(TExecuteQueryAction::GetTypeName());

        bool TExecuteQueryAction::DoExecute(const TString& userId, const IBaseServer& server) const {
            Y_UNUSED(userId);
            NStorage::IDatabase::TPtr dataBase = server.GetDatabase(GetDBName());
            auto transaction = dataBase->CreateTransaction(false);
            TRecordsSetWT records;
            if (!transaction->Exec(GetQueryText(), &records)) {
                return false;
            }
            if (!transaction->Commit()) {
                return false;
            }
            NJson::TJsonValue jsonResult = NJson::JSON_ARRAY;
            for (auto&& record : records) {
                jsonResult.AppendValue(record.SerializeToJson());
            }
            SetQueryResult(jsonResult.GetStringRobust());
            return true;
        }

    }
}
