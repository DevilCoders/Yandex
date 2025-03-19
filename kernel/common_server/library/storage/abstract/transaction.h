#pragma once
#include "query_result.h"
#include "statement.h"
#include <util/datetime/base.h>
#include <util/generic/guid.h>
#include <util/generic/array_ref.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <kernel/common_server/util/accessor.h>
#include <util/generic/vector.h>
#include <kernel/common_server/library/storage/query/remove_rows.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/util/destroyer.h>

namespace NCS {
    namespace NStorage {
        class IDatabase;

        enum class ETransactionStatus {
            InProgress,
            Failed,
            Commited,
            Cancelled,
        };

        class ITransaction: public NRequest::IExternalMethods {
        private:
            using TBase = NRequest::IExternalMethods;
        public:
            static bool NeedAssertOnTransactionFail;
            static TDestroyer<ITransaction> TransactionsDestroyer;
            using TStatement = NCS::NStorage::TStatement;
            using TPtr = TAtomicSharedPtr<ITransaction>;

        public:
            CSA_READONLY(TInstant, FinishInstant, TInstant::Zero());
            CSA_READONLY(TInstant, StartInstant, Now());
            CSA_FLAG(ITransaction, ExpectFail, false);
            CSA_READONLY(TString, TransactionId, TGUID::CreateTimebased().AsGuidString());
            CSA_READONLY(ETransactionStatus, Status, ETransactionStatus::InProgress);

        protected:
            TVector<TString> QueriesCollector;
        protected:
            virtual bool DoCommit() = 0;
            virtual bool DoRollback() = 0;
            virtual IQueryResult::TPtr DoExec(const TStatement& statement) = 0;
            virtual IQueryResult::TPtr DoMultiExec(TConstArrayRef<TStatement> statements);
            virtual IFutureQueryResult::TPtr DoAsyncMultiExec(const TVector<TStatement::TPtr>& statements);
            virtual TStatement PrepareStatement(const TStatement& statementExt) const;
            virtual bool DoIsValidFieldName(const TString& f) const override;
            virtual bool DoIsValidTableName(const TString& t) const override {
                return DoIsValidFieldName(t);
            }

        public:
            using TBase::Quote;
            virtual TString BuildRequest(const NCS::NStorage::TRemoveRowsQuery& query) const = 0;

            TFLEventLog::TLogWriterContext ProblemLog(const TString& text = "") const;

            static void RegisterToDestroy(ITransaction::TPtr&& transaction);
            virtual ~ITransaction();

            TString GetStringReport() const;

            const TVector<TString>& GetQueriesCollector() const {
                return QueriesCollector;
            }

            TDuration GetDuration() const {
                if (FinishInstant == TInstant::Zero()) {
                    return Now() - StartInstant;
                } else {
                    return FinishInstant - StartInstant;
                }
            }

            virtual const IDatabase& GetDatabase() = 0;
            virtual TString PrintStatement(const TStatement& statement) const;
            virtual TString PrintStatements(TConstArrayRef<TStatement> statements) const;
            virtual TString PrintStatements(const TVector<TStatement::TPtr>& statements) const final;

            template <class TContainer>
            TString QuoteContainer(const TContainer& values) const {
                TStringBuilder result;
                ui32 idx = 0;
                for (auto&& i : values) {
                    if (idx) {
                        result << ",";
                    }
                    result << Quote(i);
                    ++idx;
                }
                return result;
            }

            template <class T>
            TString Quote(const TSet<T>& values) const {
                return QuoteContainer(values);
            }

            template <class T>
            TString Quote(const TVector<T>& values) const {
                return QuoteContainer(values);
            }

            TTransactionQueryResult Exec(const TStatement& statement) noexcept;
            TTransactionQueryResult Exec(const TString& query, IBaseRecordsSet* records) noexcept;
            virtual TTransactionQueryResult ExecRequest(const NRequest::INode& request) noexcept;
            template <class TQuery>
            TTransactionQueryResult ExecQuery(const TQuery& query, IBaseRecordsSet* records) noexcept {
                const bool baseNeedReturn = query.GetNeedReturn();
                query.SetNeedReturn(records);
                auto result = Exec(BuildRequest(query), records);
                query.SetNeedReturn(baseNeedReturn);
                return result;
            }

            template <class TQuery>
            IFutureQueryResult::TPtr AsyncExecQuery(const TQuery& query, IBaseRecordsSet* records) noexcept {
                const bool baseNeedReturn = query.GetNeedReturn();
                query.SetNeedReturn(records);
                TVector<TStatement::TPtr> statements;
                statements.emplace_back(new TStatement(BuildRequest(query), records));
                auto result = AsyncMultiExec(statements);
                query.SetNeedReturn(baseNeedReturn);
                return result;
            }

            TTransactionQueryResult Exec(const TString& query) noexcept;

            TTransactionQueryResult MultiExec(TConstArrayRef<TStatement> statements) noexcept;
            TTransactionQueryResult MultiExecRequests(const TSRRequests& queries) noexcept;
            IFutureQueryResult::TPtr AsyncMultiExec(const TVector<TStatement::TPtr>& statements) noexcept;
            IFutureQueryResult::TPtr AsyncMultiExecRequests(const TSRRequests& requests) noexcept;
            IFutureQueryResult::TPtr AsyncMultiExecRequests(const TVector<TSRQuery>& queries) noexcept {
                return AsyncMultiExecRequests(TSRRequests(queries));
            }
            IFutureQueryResult::TPtr AsyncExecRequest(const TSRQuery& query) noexcept {
                TSRRequests requests;
                requests.AddQuery(query);
                return AsyncMultiExecRequests(requests);
            }
            template <class T>
            IFutureQueryResult::TPtr AsyncExecRequest(const T& query) noexcept {
                TSRRequests requests;
                TSRQuery srQuery(new T(query));
                requests.AddQuery(srQuery);
                return AsyncMultiExecRequests(requests);
            }

            virtual bool Commit() final;
            virtual bool Rollback() final;
        };

        class TFailedTransaction: public ITransaction {
        private:
            const IDatabase& Database;

        protected:
            virtual bool DoCommit() override {
                return false;
            }
            virtual bool DoRollback() override {
                return false;
            }

            virtual IQueryResult::TPtr DoExec(const TStatement& /*statement*/) override {
                return MakeAtomicShared<TQueryResult>();
            }

            virtual TString QuoteImpl(const TDBValueInput& v) const override {
                return "'" + TDBValueInputOperator::SerializeToString(v) + "'";
            }
        public:
            TFailedTransaction(const IDatabase& db);

            virtual TString BuildRequest(const NCS::NStorage::TRemoveRowsQuery& /*query*/) const override {
                return "";
            }

            virtual const IDatabase& GetDatabase() override {
                return Database;
            }
        };
    }
}
