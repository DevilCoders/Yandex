#include "ydb_session_storage.h"
#include "eventlog_err.h"

#include <util/string/printf.h>
#include <util/string/strip.h>
#include <util/system/env.h>


namespace NAntiRobot {

namespace {

NYdb::TDriverConfig MakeYdbDriverConfig() {
    NYdb::TDriverConfig result;

    result.SetEndpoint(ANTIROBOT_DAEMON_CONFIG.YdbEndpoint);
    result.SetDatabase(ANTIROBOT_DAEMON_CONFIG.YdbDatabase);
    result.SetAuthToken(GetEnv("YDB_TOKEN"));

    return result;
}

NYdb::NTable::TClientSettings MakeYdbClientSettings() {
    NYdb::NTable::TClientSettings result;
    NYdb::NTable::TSessionPoolSettings sessionPoolSettings;

    sessionPoolSettings.MaxActiveSessions(ANTIROBOT_DAEMON_CONFIG.YdbMaxActiveSessions);

    result.SessionPoolSettings(sessionPoolSettings);
    return result;
}

// Create table query: https://yql.yandex-team.ru/Operations/YLd6A_MBw877XeBEuaAFZXbHl7TMlu7fPs7alp_KOjg=

const char* LOAD_SESSION_DATA_QUERY_TEMPLATE = R"___(
    -- Load session data
    declare $key as UInt64;
    select key from `%s` where key = $key
)___";

const char* STORE_SESSION_DATA_QUERY_TEMPLATE = R"___(
    -- Store session data
    declare $key as UInt64;
    declare $spravka as String;
    declare $timestamp as Timestamp;
    upsert into `%s` (key, spravka, `timestamp`) values ($key, $spravka, $timestamp)
)___";

}

TYdbSessionStorage::TYdbSessionStorage()
    : Driver(MakeYdbDriverConfig())
    , Client(Driver, MakeYdbClientSettings())
    , TablePath("smart-sessions")
    , LoadSessionDataQuery(Sprintf(LOAD_SESSION_DATA_QUERY_TEMPLATE, TablePath.c_str()))
    , StoreSessionDataQuery(Sprintf(STORE_SESSION_DATA_QUERY_TEMPLATE, TablePath.c_str()))
{
}

static TString QueryDescription(const TString& query) {
    // Assuming that first line of the query contains comment with description.

    TString stripped = StripString(query);
    return TString{TStringBuf(stripped).Before('\n')}.Quote();
}

class TYdbRequestError : public yexception {
};

template <typename TParamsBuilderFunc>
static NThreading::TFuture<NYdb::NTable::TDataQueryResult> RunQuery(
    NYdb::NTable::TTableClient& client,
    const TString& query,
    TParamsBuilderFunc paramsBuilderFunc,
    TDuration timeout,
    NYdb::NTable::TTxSettings txSettings = NYdb::NTable::TTxSettings::SerializableRW()
) {
    const TInstant start = Now();

    auto fsresult = client.GetSession(NYdb::NTable::TCreateSessionSettings().ClientTimeout(timeout));
    return fsresult.Apply([query, timeout, paramsBuilderFunc, start, txSettings](const NThreading::TFuture<NYdb::NTable::TCreateSessionResult>& fsresult) {
        const auto& sresult = fsresult.GetValue();
        if (!sresult.IsSuccess()) {
            ythrow TYdbRequestError() << "YDB Error while creating session for query "
                                << QueryDescription(query) << ": " << sresult.GetStatus() << " "
                                << sresult.GetIssues().ToString();
        }
        auto session = sresult.GetSession();

        const auto elapsedAfterGetSession = Now() - start;
        if (elapsedAfterGetSession >= timeout) {
            ythrow TYdbRequestError() << "YDB Error after creating session for query "
                                << QueryDescription(query) << ": Timed out";
        }

        auto fpresult = session.PrepareDataQuery(query, NYdb::NTable::TPrepareDataQuerySettings().ClientTimeout(timeout - elapsedAfterGetSession));
        return fpresult.Apply([query, timeout, paramsBuilderFunc, start, txSettings](const NYdb::NTable::TAsyncPrepareQueryResult& fpresult) {
            const auto& presult = fpresult.GetValue();
            if (!presult.IsSuccess()) {
                ythrow TYdbRequestError() << "YDB Error while preparing query "
                                    << QueryDescription(query) << ": " << presult.GetStatus() << " "
                                    << presult.GetIssues().ToString();
            }

            auto preparedQuery = presult.GetQuery();

            const auto elapsedAfterPrepare = Now() - start;
            if (elapsedAfterPrepare >= timeout) {
                ythrow TYdbRequestError() << "YDB Error after preparing query " << QueryDescription(query) << ": Timed out";
            }

            return preparedQuery.Execute(NYdb::NTable::TTxControl::BeginTx(txSettings).CommitTx(),
                                         paramsBuilderFunc(preparedQuery.GetParamsBuilder()),
                                         NYdb::NTable::TExecDataQuerySettings().ClientTimeout(timeout - elapsedAfterPrepare));
        });
    }).Apply([query](const auto& fqresult) {
        const auto& qresult = fqresult.GetValue();
        if (!qresult.IsSuccess()) {
            ythrow TYdbRequestError() << "YDB Error while executing query "
                                << QueryDescription(query) << ": " << qresult.GetStatus() << " "
                                << qresult.GetIssues().ToString();
        }
        return fqresult;
    });
}

NThreading::TFuture<bool> TYdbSessionStorage::CheckSession(const TRequest& req, const TSpravka& spravka) {
    auto paramBuilder = [spravka](NYdb::TParamsBuilder&& builder) {
        builder.AddParam("$key").Uint64(spravka.Hash()).Build();
        return builder.Build();
    };

    return RunQuery(Client, LoadSessionDataQuery, paramBuilder, ANTIROBOT_DAEMON_CONFIG.YdbSessionReadTimeout).Apply([this, spravka, &req](const auto& future) {
        try {
            auto qresult = future.GetValue();
            Y_ENSURE(qresult.GetResultSets().size() == 1);
            auto parser = qresult.GetResultSetParser(0);

            if (parser.TryNextRow()) {
                return NThreading::MakeFuture(false);
            }

            auto paramBuilder = [spravka](NYdb::TParamsBuilder&& builder) {
                builder.AddParam("$key").Uint64(spravka.Hash()).Build();
                builder.AddParam("$spravka").String(spravka.ToString()).Build();
                builder.AddParam("$timestamp").Timestamp(spravka.Time).Build();
                return builder.Build();
            };
            return RunQuery(Client, StoreSessionDataQuery, paramBuilder, ANTIROBOT_DAEMON_CONFIG.YdbSessionWriteTimeout).Apply([&req, spravka](const auto& future) {
                try {
                    future.GetValue();
                } catch (...) {
                    EVLOG_MSG << req << "Captcha validate error. Error while storing in YDB [session] ("
                              << CurrentExceptionMessage() << ") spravka: " << spravka.ToString();
                }
                return NThreading::MakeFuture(true);
            });
        } catch (...) {
            EVLOG_MSG << req << "Captcha validate error. Error while getting YDB query result [session] ("
                      << CurrentExceptionMessage() << ") spravka approved: " << spravka.ToString();
            return NThreading::MakeFuture(true);
        }
    });
}

} // namespace NAntiRobot
