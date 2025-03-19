#include "query_result.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {
    namespace NStorage {

        TTransactionQueryResult IFutureQueryResult::Fetch(const ui32* requestAddress) noexcept {
            auto gLogging = TFLRecords::StartContext().Method("IFutureQueryResult::Fetch")("transaction_id", TransactionId);
            try {
                return DoFetch(requestAddress);
            } catch (...) {
                TFLEventLog::Error("cannot fetch future data")("reason", CurrentExceptionMessage());
                return MakeAtomicShared<TQueryResult>(false, 0u);
            }
        }

        bool IFutureQueryResult::AddRequest(const TStatement& statement, ui32* requestAddress) noexcept {
            return AddRequest(new TStatement(statement), requestAddress);
        }

        bool IFutureQueryResult::AddRequest(TStatement::TPtr statement, ui32* requestAddress) noexcept {
            auto gLogging = TFLRecords::StartContext().Method("IFutureQueryResult::AddRequest")("transaction_id", TransactionId);
            if (!statement) {
                TFLEventLog::Error("nullptr statement for add request");
                return false;
            }
            try {
                TFLEventLog::Lowest(statement->GetQuery());
                return DoAddRequest(statement, requestAddress);
            } catch (...) {
                TFLEventLog::Error("cannot add statement")("statement", statement->GetQuery())("message", CurrentExceptionMessage());
                return false;
            }
        }

        bool IFutureQueryResult::AddQuery(const NRequest::IExternalMethods& extMethods, const TSRQuery& query, ui32* requestAddress) noexcept {
            auto gLogging = TFLRecords::StartContext().Method("IFutureQueryResult::AddQuery")("transaction_id", TransactionId);
            auto statement = query.BuildStatement(extMethods);
            if (!statement) {
                TFLEventLog::Error("cannot build statement for query");
                return false;
            }
            return AddRequest(*statement, requestAddress);
        }

    }
}
