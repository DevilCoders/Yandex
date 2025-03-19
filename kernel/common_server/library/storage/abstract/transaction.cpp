#include "transaction.h"
#include <kernel/common_server/library/logging/events.h>
#include <util/string/escape.h>

namespace NCS {
    namespace NStorage {
        bool ITransaction::NeedAssertOnTransactionFail = false;
        TDestroyer<ITransaction> ITransaction::TransactionsDestroyer("transaction_destroyer");

        TFLEventLog::TLogWriterContext ITransaction::ProblemLog(const TString& text) const {
            if (GetStatus() == NStorage::ETransactionStatus::Failed) {
                return TFLEventLog::Log(text, IsExpectFail() ? TLOG_DEBUG : TLOG_ERR);
            } else {
                return TFLEventLog::Log(text, TLOG_NOTICE);
            }
        }

        void ITransaction::RegisterToDestroy(ITransaction::TPtr&& transaction) {
            TransactionsDestroyer.Register(std::move(transaction));
        }

        ITransaction::~ITransaction() {
            if (GetDuration() > TDuration::Seconds(1) || (GetStatus() == NStorage::ETransactionStatus::Failed)) {
                TVector<TString> packedStrings;
                for (auto&& i : GetQueriesCollector()) {
                    if (i.size() > 2048) {
                        packedStrings.emplace_back(i.substr(0, 2048) + "...");
                    } else {
                        packedStrings.emplace_back(i);
                    }
                }
                ProblemLog("LongFailedSessionDestroy")
                    ("created", GetStartInstant().Seconds())
                    ("finished", GetFinishInstant().Seconds())
                    ("duration", GetDuration().MilliSeconds())
                    ("committed", GetStatus() == NStorage::ETransactionStatus::Commited)
                    ("transaction_id", GetTransactionId())
                    ("queries", JoinSeq(", ", packedStrings))
                    ("status", ToString(GetStatus()));
            }
        }

        TString ITransaction::GetStringReport() const {
            return "transaction status:" + ::ToString(Status) + " with transaction_id = " + GetTransactionId();
        }

        TString ITransaction::PrintStatement(const TStatement& statement) const {
            TString result = statement.GetQuery();
            StripInPlace(result);
            if (!result.empty() && result.back() != ';') {
                result.append(';');
            }
            return result;
        }

        TString ITransaction::PrintStatements(TConstArrayRef<TStatement> statements) const {
            TString result;
            for (auto&& statement : statements) {
                if (result) {
                    result.append(' ');
                }
                result.append(PrintStatement(statement));
            }
            return result;
        }

        TString ITransaction::PrintStatements(const TVector<TStatement::TPtr>& statements) const {
            TVector<TStatement> arrLocal;
            for (auto&& i : statements) {
                arrLocal.emplace_back(*i);
            }
            TConstArrayRef<TStatement> arrRef(arrLocal.begin(), arrLocal.end());
            return PrintStatements(arrRef);
        }

        ITransaction::TStatement ITransaction::PrepareStatement(const TStatement& statementExt) const {
            return statementExt;
        }

        bool ITransaction::DoIsValidFieldName(const TString& f) const {
            if (!f) {
                TFLEventLog::Error("empty name")("name", f);
                return false;
            }
            if (f[0] >= '0' && f[0] <= '9') {
                TFLEventLog::Error("incorrect start char")("name", f);
                return false;
            }
            for (auto&& c : f) {
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
                    continue;
                }
                TFLEventLog::Error("incorrect name character")("name", f);
                return false;
            }
            return true;
        }

        TTransactionQueryResult NStorage::ITransaction::Exec(const TStatement& statementExt) noexcept {
            auto gLogging = TFLRecords::StartContext().Method("ITransaction::Exec")("db_stage", "query")("transaction_id", TransactionId);
            try {
                const TStatement& statementLocal = PrepareStatement(statementExt);
                TFLEventLog::Lowest(statementLocal.GetQuery());
                QueriesCollector.emplace_back(statementLocal.GetQuery());
                try {
                    auto result = DoExec(statementLocal);
                    if (!result || !result->IsSucceed()) {
                        FinishInstant = Now();
                        Status = ETransactionStatus::Failed;
                        ProblemLog("cannot execute request")("result", "error")("query", EscapeC(statementLocal.GetQuery()), 256);
                        ASSERT_WITH_LOG(IsExpectFail() || !NeedAssertOnTransactionFail) << GetStringReport() << Endl;
                    } else {
//                        TFLEventLog::Debug()("result", "success");
                    }
                    if (!!result) {
                        return result;
                    } else {
                        return MakeAtomicShared<TQueryResult>();
                    }
                } catch (...) {
                    Status = ETransactionStatus::Failed;
                    ProblemLog(CurrentExceptionMessage())("result", "error")("query", EscapeC(statementLocal.GetQuery()));
                    ASSERT_WITH_LOG(IsExpectFail() || !NeedAssertOnTransactionFail) << GetStringReport() << Endl;
                    return MakeAtomicShared<TQueryResult>();
                }
            } catch (...) {
                FinishInstant = Now();
                Status = ETransactionStatus::Failed;
                ProblemLog(CurrentExceptionMessage())("result", "error")("reason", GetStringReport());
                ASSERT_WITH_LOG(IsExpectFail() || !NeedAssertOnTransactionFail) << GetStringReport() << Endl;
                return MakeAtomicShared<TQueryResult>();
            }
        }

        TTransactionQueryResult ITransaction::MultiExecRequests(const TSRRequests& requests) noexcept {
            auto gLogging = TFLRecords::StartContext().Method("MultiExecRequests")("db_stage", "multi_exec")("transaction_id", TransactionId);
            TVector<NCS::NStorage::TStatement> statements;
            if (!requests.BuildStatements(*this, statements)) {
                ProblemLog("cannot construct statements")("result", "error")("async_multi_query", requests.DebugString());
                return MakeAtomicShared<TQueryResult>(false, 0u);
            }
            return MultiExec(statements);
        }

        TTransactionQueryResult ITransaction::MultiExec(TConstArrayRef<TStatement> statements) noexcept {
            auto gLogging = TFLRecords::StartContext().Method("MultiExec")("db_stage", "multi_exec")("transaction_id", TransactionId);
            for (auto&& statement : statements) {
                TFLEventLog::Lowest(statement.GetQuery());
                QueriesCollector.emplace_back(statement.GetQuery());
            }
            try {
                auto result = DoMultiExec(statements);
                if (!result || !result->IsSucceed()) {
                    FinishInstant = Now();
                    Status = ETransactionStatus::Failed;
                    ProblemLog()("result", "error")("multi_query", EscapeC(PrintStatements(statements)));
                    ASSERT_WITH_LOG(IsExpectFail() || !NeedAssertOnTransactionFail) << GetStringReport() << Endl;
                } else {
                    //                TFLEventLog::Log()("result", "success");
                }
                if (!!result) {
                    return result;
                } else {
                    return MakeAtomicShared<TQueryResult>();
                }
            } catch (...) {
                Status = ETransactionStatus::Failed;
                ProblemLog(CurrentExceptionMessage())("result", "error")("multi_query", EscapeC(PrintStatements(statements)))("reason", GetStringReport());
                ASSERT_WITH_LOG(IsExpectFail() || !NeedAssertOnTransactionFail) << GetStringReport() << Endl;
                return MakeAtomicShared<TQueryResult>();
            }
        }

        IFutureQueryResult::TPtr ITransaction::AsyncMultiExecRequests(const TSRRequests& requests) noexcept {
            auto gLogging = TFLRecords::StartContext().Method("AsyncMultiExecRequests")("db_stage", "async_multi_exec")("transaction_id", TransactionId);
            TVector<NCS::NStorage::TStatement::TPtr> statements;
            if (!requests.BuildStatementsPtr(*this, statements)) {
                ProblemLog("cannot construct statements")("result", "error")("async_multi_query", requests.DebugString());
                return MakeAtomicShared<TSimpleFutureQueryResult>(new TQueryResult(false, 0));
            }
            return AsyncMultiExec(statements);
        }

        IFutureQueryResult::TPtr ITransaction::AsyncMultiExec(const TVector<TStatement::TPtr>& statements) noexcept {
            auto gLogging = TFLRecords::StartContext().Method("AsyncMultiExec")("db_stage", "async_multi_exec")("transaction_id", TransactionId);
            for (auto&& statement : statements) {
                TFLEventLog::Lowest(statement->GetQuery());
                QueriesCollector.emplace_back(statement->GetQuery());
            }
            try {
                auto result = DoAsyncMultiExec(statements);
                if (!!result) {
                    result->SetTransactionId(TransactionId);
                    TFLEventLog::Debug()("result", "started");
                    return result;
                } else {
                    ProblemLog()("result", "error")("async_multi_query", PrintStatements(statements));
                    return MakeAtomicShared<TSimpleFutureQueryResult>(new TQueryResult(false, 0));
                }
            } catch (...) {
                Status = ETransactionStatus::Failed;
                ProblemLog(CurrentExceptionMessage())("result", "error")("async_multi_query", PrintStatements(statements));
                return MakeAtomicShared<TSimpleFutureQueryResult>(new TQueryResult(false, 0));
            }
        }

        IQueryResult::TPtr NStorage::ITransaction::DoMultiExec(TConstArrayRef<TStatement> statements) {
            ui32 affected = 0;
            bool succeeded = true;
            for (auto&& statement : statements) {
                auto subresult = DoExec(statement);
                if (!subresult) {
                    succeeded = false;
                    break;
                }
                affected += subresult->GetAffectedRows();
                succeeded &= subresult->IsSucceed();
                if (!succeeded) {
                    break;
                }
            }
            return MakeAtomicShared<TQueryResult>(succeeded, affected);
        }

        IFutureQueryResult::TPtr NStorage::ITransaction::DoAsyncMultiExec(const TVector<TStatement::TPtr>& statements) {
            TVector<TStatement> stLocal;
            for (auto&& i : statements) {
                stLocal.emplace_back(*i);
            }
            TConstArrayRef<TStatement> arrRef(stLocal.begin(), stLocal.end());
            return MakeAtomicShared<TSimpleFutureQueryResult>(DoMultiExec(arrRef));
        }

        TTransactionQueryResult ITransaction::Exec(const TString& query, IBaseRecordsSet* records) noexcept {
            TStatement statement(query, records);
            return Exec(statement);
        }

        TTransactionQueryResult ITransaction::Exec(const TString& query) noexcept {
            TStatement statement(query);
            return Exec(statement);
        }

        TTransactionQueryResult ITransaction::ExecRequest(const NRequest::INode& request) noexcept {
            TStringStream ss;
            if (!request.ToString(*this, ss)) {
                return MakeAtomicShared<TQueryResult>(false, 0u);
            }
            IBaseRecordsSet* records = nullptr;
            auto* response = dynamic_cast<const NRequest::IResponseContainer*>(&request);
            if (response && response->MutableRecordsSet()) {
                records = response->MutableRecordsSet();
            }
            return Exec(ss.Str(), records);
        }

        bool ITransaction::Commit() {
            auto gLogging = TFLRecords::StartContext().Method("ITransaction::Commit")("db_stage", "commit")("transaction_id", TransactionId)("status", Status);
            if (Status != ETransactionStatus::InProgress) {
                ProblemLog("incorrect status");
                return false;
            }
            try {
                if (!DoCommit()) {
                    ProblemLog("DoCommit failed")("result", "error");
                    FinishInstant = Now();
                    Status = ETransactionStatus::Failed;
                    return false;
                }
                FinishInstant = Now();
                Status = ETransactionStatus::Commited;
                //            TFLEventLog::Log()("db_stage", "commit")("transaction_id", TransactionId)("result", "success");
            } catch (...) {
                Status = ETransactionStatus::Failed;
                ProblemLog(CurrentExceptionMessage())("result", "error");
                return false;
            }
            return true;
        }

        bool ITransaction::Rollback() {
            auto gLogging = TFLRecords::StartContext().Method("ITransaction::Rollback")("transaction_id", TransactionId)("db_stage", "rollback");
            if (Status != ETransactionStatus::InProgress) {
                ProblemLog("incorrect status");
                return false;
            }
            try {
                if (!DoRollback()) {
                    ProblemLog("DoRollback failed")("result", "error");
                    FinishInstant = Now();
                    Status = ETransactionStatus::Failed;
                    return false;
                }
                FinishInstant = Now();
                Status = ETransactionStatus::Cancelled;
                //            TFLEventLog::Log()("db_stage", "rollback")("transaction_id", TransactionId)("result", "success");
            } catch (...) {
                Status = ETransactionStatus::Failed;
                ProblemLog(CurrentExceptionMessage())("result", "error");
                return false;
            }
            return true;
        }

        TFailedTransaction::TFailedTransaction(const IDatabase& db)
            : Database(db)
        {
            ProblemLog("failed transaction construction");
        }

    }
}
