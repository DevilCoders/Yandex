#include "test_base.h"

namespace NServerTest {

    void TTestWithDatabase::BuildDatabase(TDatabasePtr db) {
        CHECK_WITH_LOG(!!db);
        {
            auto lock = db->Lock("clear_old", true, TDuration::Zero());
            if (!!lock) {
                TRecordsSetWT records;
                {
                    NStorage::ITransaction::TPtr transaction = db->CreateTransaction(false);
                    CHECK_WITH_LOG(transaction->Exec("SET search_path=public")->IsSucceed());
                    CHECK_WITH_LOG(transaction->Exec("select nspname from pg_catalog.pg_namespace; ", &records)->IsSucceed());
                }
                TVector<TString> schemasForRemove;
                ui32 idx = 0;
                for (auto&& i : records) {
                    if (i.GetString("nspname").StartsWith("t")) {
                        const TVector<TString> parts = StringSplitter(i.GetString("nspname")).SplitBySet("_").SkipEmpty().ToList<TString>();
                        CHECK_WITH_LOG(parts.size() >= 3);
                        ui64 secondsTs;
                        CHECK_WITH_LOG(TryFromString(parts[0].substr(1), secondsTs));
                        if (Now() - TInstant::Seconds(secondsTs) > TDuration::Hours(3)) {
                            schemasForRemove.emplace_back(i.GetString("nspname"));
                        }
                    }
                    if (schemasForRemove.size() >= 50 || ++idx == records.size()) {
                        NStorage::ITransaction::TPtr transaction = db->CreateTransaction(false);
                        WARNING_LOG << schemasForRemove.size() << " schemas removing..." << Endl;
                        TStringStream ss;
                        ss << "SET search_path=public;";
                        for (auto&& iRequestScheme : schemasForRemove) {
                            ss << "DROP SCHEMA \"" << iRequestScheme << "\" CASCADE;";
                        }

                        if (!transaction->Exec(ss.Str())->IsSucceed() || !transaction->Commit()) {
                            ERROR_LOG << transaction->GetStringReport() << Endl;
                        } else {
                            WARNING_LOG << schemasForRemove.size() << " schemas removed OK" << Endl;
                        }
                        schemasForRemove.clear();
                    }
                }
            }
        }
    }
}
