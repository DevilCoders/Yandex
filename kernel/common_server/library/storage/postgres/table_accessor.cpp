#include "table_accessor.h"
#include "postgres_conn_pool.h"
#include <util/random/random.h>
#include <util/string/subst.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/library/logging/events.h>
#include <util/string/type.h>
#include <kernel/common_server/library/storage/reply/abstract.h>
#include <kernel/common_server/library/storage/balancing/configured.h>
#include <kernel/common_server/library/storage/balancing/no_balancing.h>
#include "postgres_storage.h"

namespace NCS {
    namespace NStorage {
        IDatabaseConfig::TFactory::TRegistrator<TPostgresConfig> TPostgresConfig::Registrator("Postgres");

        namespace {
            using namespace NStorage;

            template <
                pqxx::isolation_level ISOLATIONLEVEL = pqxx::read_committed,
                pqxx::readwrite_policy READWRITE = pqxx::read_write
            >
                class TDBInternalTransaction: public pqxx::transaction<ISOLATIONLEVEL, READWRITE> {
                protected:
                    using base = pqxx::transaction<ISOLATIONLEVEL, READWRITE>;

                public:
                    TDBInternalTransaction(pqxx::connection_base& connection)
                        : pqxx::internal::namedclass("custom_transaction")
                        , base(connection) {

                    }
            };

            class TNonInternalTransaction: public pqxx::nontransaction {
            private:
                using TBase = pqxx::nontransaction;
            public:
                TNonInternalTransaction(pqxx::connection_base& connection)
                    : pqxx::internal::namedclass("custom_transaction")
                    , TBase(connection) {

                }
            };

            static const TSet<TString> PostgressFunctions = { "now", "uuid_generate_v4", "nextval", "least", "concat" };

            template <class TTransactionType>
            class TPostgresTransaction: public NSQL::TTransaction {
            private:
                using TSelfTransaction = TPostgresTransaction<TTransactionType>;
            protected:
                class TPSValueQuoter {
                    const TTransactionType& PQXXTransaction;
                public:
                    TPSValueQuoter(const TTransactionType& transaction)
                        : PQXXTransaction(transaction) {

                    }

                    template <class T>
                    TString operator()(const T& value) const {
                        return ::ToString(value);
                    }
                    TString operator()(const TString& value) const {
                        if (value.EndsWith(')')) {
                            TStringBuf tmp(value);
                            TStringBuf funcName = tmp.Before('(');
                            if (PostgressFunctions.contains(::ToLowerUTF8(funcName))) {
                                return value;
                            }
                        }
                        std::string input(value.data(), value.size());
                        std::string str = PQXXTransaction.quote(input);
                        return TString(str.data(), str.size());
                    }
                    TString operator()(const bool value) const {
                        return value ? "true" : "false";
                    }
                    TString operator()(const TGUID& value) const {
                        return "'" + value.AsUuidString() + "'::uuid";
                    }
                    TString operator()(const TBinaryData& value) const {
                        if (value.IsNullData()) {
                            return "null::bytea";
                        } else if (value.GetDeprecated()) {
                            return "'" + value.GetEncodedData() + "'";
                        } else {
                            return "decode('" + value.GetEncodedData() + "', 'hex')";
                        }
                    }
                    TString operator()(const TNull& /*value*/) const {
                        return "NULL";
                    }
                };

                virtual TString QuoteImpl(const TDBValueInput& v) const override {
                    TPSValueQuoter pred(PQXXTransaction);
                    return std::visit(pred, v);
                }

            public:
                TPostgresTransaction(TPostgresConnectionsPool::TActiveConnection&& connection, const IDatabase* database,
                    const NSQL::TAccessLogger* logger, const TMaybe<TDuration> lockTimeout, const TMaybe<TDuration> statementTimeout, const TString& namespaces)
                    : Connection(std::move(connection))
                    , Database(database)
                    , PQXXTransaction(*Connection)
                    , Logger(logger) {
                    if (lockTimeout) {
                        PQXXTransaction.set_variable("lock_timeout", ToString(lockTimeout->MilliSeconds()));
                    }
                    if (statementTimeout) {
                        PQXXTransaction.set_variable("statement_timeout", ToString(statementTimeout->MilliSeconds()));
                    }
                    if (namespaces && namespaces != "public") {
                        PQXXTransaction.set_variable("search_path", namespaces);
                    }
                }

                class TPostgresQueryContainer: public IOriginalContainer {
                private:
                    TVector<TString> StringsStorage;
                public:
                    void ReserveVector(const ui32 size) {
                        StringsStorage.reserve(size);
                    }

                    TStringBuf DecodeBinary(const pqxx::field& field) {
                        StringsStorage.emplace_back(HexDecode(field.c_str() + 2, field.size() - 2));
                        return StringsStorage.back();
                    }

                    TPostgresQueryContainer(pqxx::result&& data)
                        : Data(data) {
                    }

                    const pqxx::result& GetData() const {
                        return Data;
                    }

                    TVector<TOrderedColumn> GetOrderedColumns() const {
                        TVector<TOrderedColumn> result;
                        for (ui32 col = 0; col < Data.columns(); ++col) {
                            EColumnType parsingColumnType = EColumnType::Text;
                            const auto oid = Data.column_type(col);
                            if (oid == 17) {
                                parsingColumnType = EColumnType::Binary;
                            } else if (oid == 16) {
                                parsingColumnType = EColumnType::Boolean;
                            } else if (oid == 20 || oid == 23 || oid == 21) {
                                parsingColumnType = EColumnType::I64;
                            } else if (oid == 700 || oid == 701) {
                                parsingColumnType = EColumnType::Double;
                            } else {
                                parsingColumnType = EColumnType::Text;
                            }
                            result.emplace_back(Data.column_name(col), parsingColumnType);
                        }
                        return result;
                    }

                private:
                    pqxx::result Data;
                };

                void ParseResult(THolder<TPostgresQueryContainer>&& container, IRecordsSetWT* records) const {
                    if (container && records) {
                        TSimpleDecoder bDecoder;
                        const TVector<TOrderedColumn> orderedColumns = container->GetOrderedColumns();
                        bDecoder.TakeColumnsInfo(orderedColumns);
                        const auto& result = container->GetData();
                        records->Reserve(result.size());
                        TVector<TStringBuf> values;
                        values.resize(orderedColumns.size());
                        TVector<bool> binaryFields;
                        ui32 binFieldsCount = 0;
                        for (auto&& i : orderedColumns) {
                            if (i.GetType() == EColumnType::Binary) {
                                binaryFields.emplace_back(true);
                                ++binFieldsCount;
                            } else {
                                binaryFields.emplace_back(false);
                            }
                        }
                        TVector<TString> binaryUnpack;
                        binaryUnpack.reserve(binFieldsCount);
                        for (const auto& row : result) {
                            for (auto&& field : row) {
                                if (binaryFields[field.num()] && field.size() >= 2) {
                                    binaryUnpack.emplace_back(HexDecode(field.c_str() + 2, field.size() - 2));
                                    values[field.num()] = TStringBuf(binaryUnpack.back());
                                } else {
                                    values[field.num()] = TStringBuf(field.c_str(), field.size());
                                }
                            }
                            TTableRecordWT record;
                            if (record.ReadThroughDecoder(bDecoder, values)) {
                                records->AddRow(std::move(record));
                            } else {
                                records->AddError();
                            }
                            binaryUnpack.clear();
                        }
                    }
                }

                void ParseResult(THolder<TPostgresQueryContainer>&& container, IPackedRecordsSet* records) const {
                    if (container && records) {
                        const auto& result = container->GetData();
                        TVector<TOrderedColumn> orderedColumns = container->GetOrderedColumns();

                        records->Initialize(result.size(), orderedColumns);
                        TVector<bool> binaryFields;
                        ui32 binFieldsCount = 0;
                        for (auto&& i : orderedColumns) {
                            if (i.GetType() == EColumnType::Binary) {
                                binaryFields.emplace_back(true);
                                ++binFieldsCount;
                            } else {
                                binaryFields.emplace_back(false);
                            }
                        }
                        TVector<TStringBuf> record;
                        record.resize(orderedColumns.size());
                        container->ReserveVector(binFieldsCount * result.size());
                        for (const auto& row : result) {
                            for (auto&& field : row) {
                                if (binaryFields[field.num()] && field.size() >= 2) {
                                    record[field.num()] = container->DecodeBinary(field);
                                } else {
                                    record[field.num()] = TStringBuf(field.c_str(), field.size());
                                }
                            }
                            records->AddRow(record);
                        }
                        records->StoreOriginalData(std::move(container));
                    }
                }

                void ParseResult(THolder<TPostgresQueryContainer>&& container, const TStatement& statement) const {
                    if (!statement.GetRecords()) {
                        return;
                    } else if (IRecordsSetWT* rs = statement.GetRecords()->GetAs<IRecordsSetWT>()) {
                        ParseResult(std::move(container), rs);
                    } else if (IPackedRecordsSet* rs = statement.GetRecords()->GetAs<IPackedRecordsSet>()) {
                        ParseResult(std::move(container), rs);
                    } else {
                        S_FAIL_LOG << "Incorrect records storage class (not implemented in code)" << Endl;
                    }
                }

                IQueryResult::TPtr DoExec(const TStatement& statement) override try {
                    auto container = MakeHolder<TPostgresQueryContainer>(PQXXTransaction.exec(statement.GetQuery()));
                    auto affected = container->GetData().affected_rows();
                    ParseResult(std::move(container), statement);
                    if (Logger) {
                        Logger->RowsCountSignal(affected);
                        Logger->SuccessSignal();
                    }
                    return new TQueryResult(true, affected);
                } catch (...) {
                    if (Logger) {
                        Logger->ErrorSignal();
                    }
                    throw;
                }

                class TPostgresFutureQueryResult: public IFutureQueryResult {
                private:
                    THolder<pqxx::pipeline> Pipeline;
                    TTransactionType& PQXXTransaction;
                    TSelfTransaction* Transaction;
                    TVector<pqxx::pipeline::query_id> QueryIds;
                    TVector<TStatement::TPtr> Statements;
                    IQueryResult::TPtr Result = nullptr;
                    TVector<TTransactionQueryResult> QueryResults;
                    TTransactionQueryResult FetchQuery(const ui32 idx) {
                        if (!QueryResults[idx].Initialized()) {
                            try {
                                auto qid = QueryIds[idx];
                                auto container = MakeHolder<TPostgresQueryContainer>(Pipeline->retrieve(qid));
                                const ui32 affected = container->GetData().affected_rows();
                                Transaction->ParseResult(std::move(container), *Statements[idx]);
                                QueryResults[idx] = MakeAtomicShared<TQueryResult>(true, affected);
                            } catch (...) {
                                QueryResults[idx] = MakeAtomicShared<TQueryResult>(false, 0u);
                            }
                        }
                        return QueryResults[idx];
                    }
                    TTransactionQueryResult InitResult(const ui32* requestAddress) {
                        if (!requestAddress) {
                            ui32 count = 0;
                            for (size_t i = 0; i < QueryIds.size(); ++i) {
                                auto localResult = FetchQuery(i);
                                if (!localResult) {
                                    return localResult;
                                }
                                count += localResult->GetAffectedRows();
                            }
                            return MakeAtomicShared<TQueryResult>(true, count);
                        } else if (*requestAddress < QueryIds.size()) {
                            return FetchQuery(*requestAddress);
                        } else {
                            TFLEventLog::Error("incorrect request index for fetching async data")("requests_count", QueryIds.size())("idx", *requestAddress);
                            return MakeAtomicShared<TQueryResult>(false, 0u);
                        }
                    }
                protected:
                    virtual bool DoAddRequest(TStatement::TPtr statement, ui32* requestAddress) override {
                        if (requestAddress) {
                            *requestAddress = QueryIds.size();
                        }
                        QueryIds.emplace_back(Pipeline->insert(statement->GetQuery()));
                        Statements.emplace_back(statement);
                        QueryResults.emplace_back(TTransactionQueryResult::BuildEmpty());
                        return true;
                    }
                    virtual TTransactionQueryResult DoFetch(const ui32* requestAddress) override {
                        return InitResult(requestAddress);
                    }

                public:
                    TPostgresFutureQueryResult(TSelfTransaction* transaction, TTransactionType& pqxxTransaction)
                        : Pipeline(MakeHolder<pqxx::pipeline>(pqxxTransaction))
                        , PQXXTransaction(pqxxTransaction)
                        , Transaction(transaction)
                    {
                        Pipeline->retain(0);
                    }
                };

                virtual IFutureQueryResult::TPtr DoAsyncMultiExec(const TVector<TStatement::TPtr>& statements) override {
                    IFutureQueryResult::TPtr result = MakeAtomicShared<TPostgresFutureQueryResult>(this, PQXXTransaction);
                    for (auto&& statement : statements) {
                        result->AddRequest(statement);
                    }

                    return result;
                }


                IQueryResult::TPtr DoMultiExec(TConstArrayRef<TStatement> statements) override try {
                    pqxx::pipeline pipeline(PQXXTransaction);
                    pipeline.retain(statements.size());

                    TVector<pqxx::pipeline::query_id> qids;
                    qids.reserve(statements.size());
                    for (auto&& statement : statements) {
                        qids.emplace_back(pipeline.insert(statement.GetQuery()));
                    }

                    pipeline.complete();
                    ui32 totalAffected = 0;
                    for (size_t i = 0; i < qids.size(); ++i) {
                        auto qid = qids[i];

                        auto container = MakeHolder<TPostgresQueryContainer>(pipeline.retrieve(qid));
                        auto affected = container->GetData().affected_rows();
                        totalAffected += affected;
                        ParseResult(std::move(container), statements[i]);
                        if (Logger) {
                            Logger->RowsCountSignal(affected);
                        }
                    }
                    if (Logger) {
                        Logger->SuccessSignal();
                    }
                    return MakeAtomicShared<TQueryResult>(true, totalAffected);
                } catch (...) {
                    if (Logger) {
                        Logger->ErrorSignal();
                    }
                    throw;
                }

                bool DoCommit() override {
                    PQXXTransaction.commit();
                    return true;
                }

                bool DoRollback() override {
                    PQXXTransaction.abort();
                    return true;
                }

                const IDatabase& GetDatabase() override {
                    CHECK_WITH_LOG(Database);
                    return *Database;
                }

            private:
                TPostgresConnectionsPool::TActiveConnection Connection;
                const IDatabase* Database = nullptr;
                TTransactionType PQXXTransaction;
                const NSQL::TAccessLogger* Logger = nullptr;
            };
        }

        TPostgresDB::TPostgresDB(const TPostgresStorageOptionsImpl& config, const TString& dbInternalId, const TString& dbName, TBalancingPolicyOperator::TPtr bPolicy)
            : TBase(dbInternalId)
            , Config(config)
            , LocksTableName(config.GetLocksTableName())
            , DBName(dbName)
            , Balancer(MakeHolder<TPostgresBalancer>(Config, bPolicy, dbName)) {
            TMap<TString, TString> params;
            NUtil::TTSKVRecordParser::Parse<' ', '='>(Config.GetFullConnectionString(true), params);
            if (params.contains("dbname")) {
                Logger = MakeHolder<NSQL::TAccessLogger>(params["dbname"]);
            } else {
                Logger = MakeHolder<NSQL::TAccessLogger>("undefined");
            }
            if (LocksTableName) {
                auto transaction = CreateTransaction();
                auto createTableResult = transaction->Exec(Sprintf(
                    "CREATE TABLE IF NOT EXISTS %s (key varchar(200) PRIMARY KEY, started timestamp DEFAULT CURRENT_TIMESTAMP)",
                    LocksTableName.c_str()
                ));
                CHECK_WITH_LOG(!!createTableResult);
                CHECK_WITH_LOG(transaction->Commit());
            }
        }

        TVector<TString> TPostgresDB::GetAllTableNames() const {
            auto transaction = CreateTransaction();
            TVector<TString> names;
            TRecordsSetWT records;
            transaction->Exec("SELECT table_name FROM information_schema.tables WHERE table_schema='public'", &records);
            transaction->Commit();
            for (const auto& rec : records) {
                names.push_back(rec.GetString("table_name"));
            }
            return names;
        }

        ITransaction::TPtr TPostgresDB::DoCreateTransaction(const TTransactionFeatures& features) const {
            const TInstant start = Now();
            const TString& currentNamespace = Singleton<TDatabaseNamespace>()->GetNamespace(GetName(), features.GetNamespaces());
            auto gLogging = TFLRecords::StartContext().SignalId("pg_database")("&db_name", DBName)("&read_only", features.IsReadOnly())("&repeatable_read", features.IsRepeatableRead())("&non_transaction", features.IsNonTransaction());
            while (Now() - start < Config.GetActivateTimeout()) {
                try {
                    auto connect = features.IsReadOnly() ? Balancer->GetReadOnlyConnection() : Balancer->GetWritableConnection();
                    Y_ENSURE(!!connect, "Cannot create transaction : cannot connect to database");
                    ITransaction::TPtr result;
                    const auto lockTimeout = features.GetLockTimeoutMaybeDetach();
                    const auto statementTimeout = features.GetStatementTimeoutMaybeDetach();
                    if (features.IsNonTransaction()) {
                        result = new TPostgresTransaction<TNonInternalTransaction>(std::move(connect), this, Logger.Get(), lockTimeout, statementTimeout, currentNamespace);
                    } else {
                        if (features.IsRepeatableRead()) {
                            if (features.IsReadOnly()) {
                                result = new TPostgresTransaction<TDBInternalTransaction<pqxx::repeatable_read, pqxx::read_only>>(std::move(connect), this, Logger.Get(), lockTimeout, statementTimeout, currentNamespace);
                            } else {
                                result = new TPostgresTransaction<TDBInternalTransaction<pqxx::repeatable_read, pqxx::read_write>>(std::move(connect), this, Logger.Get(), lockTimeout, statementTimeout, currentNamespace);
                            }
                        } else {
                            if (features.IsReadOnly()) {
                                result = new TPostgresTransaction<TDBInternalTransaction<pqxx::read_committed, pqxx::read_only>>(std::move(connect), this, Logger.Get(), lockTimeout, statementTimeout, currentNamespace);
                            } else {
                                result = new TPostgresTransaction<TDBInternalTransaction<pqxx::read_committed, pqxx::read_write>>(std::move(connect), this, Logger.Get(), lockTimeout, statementTimeout, currentNamespace);
                            }
                        }
                    }
                    TFLEventLog::JustSignal()("&code", "success");
                    return result;
                } catch (...) {
                    TFLEventLog::Error(CurrentExceptionMessage())("&code", "exception");
                    ERROR_LOG << "Cannot create transaction : " << CurrentExceptionMessage() << Endl;
                    Sleep(Config.GetActivationSleep());
                }
            }
            CHECK_WITH_LOG(!Config.GetPanicOnDisconnect()) << "Cannot make transaction" << Endl;
            return new TFailedTransaction(*this);
        }

        TAbstractLock::TPtr TPostgresDB::Lock(const TString& lockName, const bool writeLock, const TDuration timeout, const TString& namespaces) const {
            const TString& currentNamespace = Singleton<TDatabaseNamespace>()->GetNamespace(GetName(), namespaces);
            const TInstant deadline = Now() + timeout;
            do {
                try {
                    auto connect = Balancer->GetWritableConnection();
                    Y_ENSURE(!!connect, "Cannot take lock : cannot connect to database");
                    auto result = MakeHolder<TPostgresLock>(std::move(connect), LocksTableName, currentNamespace + "_" + lockName, timeout, writeLock);
                    if (result->IsLockTaken()) {
                        return result.Release();
                    }
                    NOTICE_LOG << "Cannot take lock : " << currentNamespace + "_" + lockName << Endl;
                } catch (...) {
                    ERROR_LOG << "Cannot create lock : " << CurrentExceptionMessage() << Endl;
                }
                Sleep(Min(Config.GetActivationSleep(), deadline - Now()));
            } while (Now() < deadline);
            return nullptr;
        }

        IDatabase::TPtr TPostgresConfig::ConstructDatabase(const IDatabaseConstructionContext* context) const {
            const TBalancingDatabaseConstructionContext* pgContext = dynamic_cast<const TBalancingDatabaseConstructionContext*>(context);
            TBalancingPolicyOperator::TPtr bPolicy;
            if (pgContext) {
                bPolicy = pgContext->GetBalancingPolicy();
            }
            auto db = MakeHolder<TPostgresDB>(*this, GetDBInternalId(), context->GetDBName(), bPolicy);
            if (GetMainNamespace()) {
                NStorage::ITransaction::TPtr transaction = db->CreateTransaction(false);
                CHECK_WITH_LOG(transaction->Exec("CREATE SCHEMA IF NOT EXISTS " + GetMainNamespace(), nullptr)->IsSucceed());
                CHECK_WITH_LOG(transaction->Commit()) << transaction->GetStringReport() << Endl;
                Singleton<TDatabaseNamespace>()->SetNamespace(db->GetName(), GetMainNamespace() + ",public");
            }
            return db;
        }

        void TPostgresConfig::DoInit(const TYandexConfig::Section* section) {
            TPostgresStorageOptionsImpl::Init(section);
        }

        void TPostgresConfig::DoToString(IOutputStream& os) const {
            TPostgresStorageOptionsImpl::ToString(os);
        }

        TString TPostgresBalancer::GetHostsString(const TString& hostName) const {
            TString result = hostName;
            //    for (auto&& i : Config.GetHosts()) {
            //        if (i != hostName) {
            //            result += "," + i;
            //        }
            //    }
            return result;
        }

        void TPostgresBalancer::RebuildReadOnlyPools() {
            auto gLogging = TFLRecords::StartContext().SignalId("pg_balancer.rebuild")("&db_name", DBName);
            TMap<TString, TWeightedHost> weightedHosts;
            {
                TReadGuard rg(RWMutex);
                weightedHosts = WeightedHosts;
            }
            for (auto&& h : Config.GetHosts()) {
                auto it = weightedHosts.find(h);
                if (it == weightedHosts.end()) {
                    TWeightedHost pool;
                    pool.SetHostName(h);
                    it = weightedHosts.emplace(h, std::move(pool)).first;
                }
                auto& wHost = it->second;
                if (!!wHost.GetPool()) {
                    continue;
                }
                TPostgresStorageOptionsImpl configLocal = Config;
                configLocal.SwitchHosts(GetHostsString(wHost.GetHostName()));
                wHost.SetPool(configLocal.ConstructRPool());
            }
            TVector<TWeightedHost*> bHosts;
            for (auto&& i : weightedHosts) {
                TWeightedHost* wHost = &i.second;
                wHost->SetEnabled(true);
                wHost->SetReplicationLag(TDuration::Zero());
                wHost->SetPingDuration(TDuration::Seconds(10));
                bHosts.emplace_back(&i.second);
            }

            for (auto&& wHost : bHosts) {
                auto gLoggingHost = TFLRecords::StartContext()("&replica", wHost->GetHostName());
                try {
                    auto connect = wHost->GetPool()->GetConnection();
                    if (!connect) {
                        wHost->SetEnabled(false);
                        TFLEventLog::Alert("cannot connect to replica").Signal()("&code", "connection_failed");
                        continue;
                    }
                    TRecordsSetWT recordsReplicationLags;
                    const TInstant start = Now();
                    {
                        TRecordsSetWT recordsReplicas;
                        ITransaction::TPtr transaction = new TPostgresTransaction<TDBInternalTransaction<pqxx::read_committed, pqxx::read_only>>(std::move(connect), nullptr, nullptr, Nothing(), Nothing(), "");
                        {
                            auto qResult = transaction->Exec("select pg_is_in_recovery() a;", &recordsReplicas);
                            if (!qResult || !qResult->IsSucceed()) {
                                wHost->SetEnabled(false);
                                TFLEventLog::Alert("cannot ask replica about lag").Signal()("&code", "query_failed");
                                continue;
                            }
                        }
                        wHost->SetPingDuration(Now() - start);
                        if (recordsReplicas.size()) {
                            const TString replicationInfo = recordsReplicas.begin()->GetString("a");
                            if (!IsTrue(replicationInfo)) {
                                continue;
                            }
                        }
                        {
                            auto qResult = transaction->Exec("select EXTRACT(epoch FROM now() - pg_last_xact_replay_timestamp()) * 1000000 a", &recordsReplicationLags);
                            if (!qResult || !qResult->IsSucceed()) {
                                wHost->SetEnabled(false);
                                TFLEventLog::Alert("cannot ask replica about lag").Signal()("&code", "query_failed");
                                continue;
                            }
                        }
                    }
                    if (recordsReplicationLags.size()) {
                        const TString lagInfo = recordsReplicationLags.begin()->GetString("a");
                        if (!!lagInfo) {
                            double d;
                            if (!TryFromString(lagInfo, d)) {
                                TFLEventLog::Alert("cannot parse replication lag")("lag", lagInfo).Signal()("&code", "lag_parsing_failed");
                                wHost->SetEnabled(false);
                            } else {
                                d = (d > 0) ? d : 0;
                                wHost->SetReplicationLag(TDuration::MicroSeconds(d));
                            }
                        }
                    }
                } catch (...) {
                    wHost->SetEnabled(false);
                    TFLEventLog::Alert("exception").Signal()("message", CurrentExceptionMessage())("&code", "exception");
                }
            }

            auto bPolicy = BalancingPolicy->ConstructBalancingPolicy(
                Config.GetUseBalancing() ? NBalancing::TConfiguredBalancingPolicy::GetTypeName() : NBalancing::TNoBalancingPolicy::GetTypeName());
            CHECK_WITH_LOG(bPolicy->CalculateObjectFeatures(bHosts));
            if (AtomicGet(IsActive) == 1) {
                TWriteGuard rg(RWMutex);
                WeightedHosts = std::move(weightedHosts);
            }
        }

        TPostgresConnectionsPool::TActiveConnection TPostgresBalancer::GetReadOnlyConnection() {
            auto gLogging = TFLRecords::StartContext().SignalId("pg_balancer")("&db_name", DBName);
            TMap<TString, TWeightedHost> weightedHosts;
            {
                TReadGuard rg(RWMutex);
                weightedHosts = WeightedHosts;
            }
            CHECK_WITH_LOG(weightedHosts.size());
            TVector<const TWeightedHost*> bHosts;
            for (auto&& i : weightedHosts) {
                bHosts.emplace_back(&i.second);
            }
            auto bPolicy = BalancingPolicy->GetBalancingPolicy();
            const TWeightedHost* wHost = bPolicy->ChooseObject<TWeightedHost>(bHosts);
            if (!wHost) {
                TFLEventLog::Alert("cannot balance connect")("connection_string", Config.GetFullConnectionString(true, true));
                return GetWritableConnection();
            } else {
                return wHost->GetPool()->GetConnection();
            }
        }
    }
}
