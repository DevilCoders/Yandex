#include "common.h"

#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/util/blob_with_header.h>
#include <kernel/common_server/library/storage/abstract/statement.h>

namespace NCS {

    TEntitySession::~TEntitySession() {
        NStorage::ITransaction::RegisterToDestroy(std::move(Transaction));
    }

    bool TEntitySession::MultiExecRequests(const TSRRequests& queries) {
        if (!Transaction->MultiExecRequests(queries)->IsSucceed()) {
            Error("transaction failed")("message", Transaction->GetStringReport());
            return false;
        }
        return true;
    }

    bool TEntitySession::Commit() {
        auto gLogging = TFLRecords::StartContext().Method("Session::Commit");
        if (!Transaction) {
            Error("transaction not initialized", ESessionResult::TransactionProblem);
            return false;
        }
        gLogging("transaction_id", Transaction->GetTransactionId())("current_status", Transaction->GetStatus());
        if (AtomicIncrement(CommitFlag) > 1) {
            Error("secondary commit usage", ESessionResult::TransactionProblem);
            return false;
        }
        if (Transaction->GetStatus() != NStorage::ETransactionStatus::InProgress) {
            Error("transaction terminated already", ESessionResult::TransactionProblem);
            return false;
        }
        if (!GetNeedCommit()) {
            Error("transaction not need commit after previous process", ESessionResult::TransactionProblem);
            return false;
        }
        if (!DryRun && !Transaction->Commit()) {
            Error("failed", ESessionResult::TransactionProblem);
            return false;
        } else {
            if (DryRun) {
                TFLEventLog::Warning("dry_run");
            }
            if (!!Context) {
                Context->OnAfterCommit();
            }
            for (auto&& act : AfterCommitActions) {
                act->Execute();
            }
            return true;
        }
    }

    bool TEntitySession::Rollback() {
        auto gLogging = TFLRecords::StartContext().Method("Session::Rollback");
        if (!Transaction) {
            Error("transaction not initialized", ESessionResult::TransactionProblem);
            return false;
        }
        gLogging("transaction_id", Transaction->GetTransactionId())("current_status", Transaction->GetStatus());
        if (AtomicIncrement(CommitFlag) > 1) {
            Error("secondary commit usage", ESessionResult::TransactionProblem);
            return false;
        }

        if (!Transaction->Rollback()) {
            Error("failed", ESessionResult::TransactionProblem);
            return false;
        } else {
            return true;
        }
    }

    void TInfoEntitySession::DoExceptionOnFail(const THttpStatusManagerConfig& config, const ILocalization* localization) const {
        int code = HTTP_OK;
        TString localizedMessage;
        TString localizedTitle;
        ASSERT_WITH_LOG(Result != ESessionResult::Success) << "incorrect situation - no error" << Endl;
        if (Result == ESessionResult::Success) {
            throw TCodedException(HTTP_INTERNAL_SERVER_ERROR).AddErrorCode("unknown error");
        }
        if (!localizedMessage && localization && LocalizedMessageKey) {
            localizedMessage = localization->GetLocalString(ELocalization::Rus, LocalizedMessageKey);
        }
        if (!localizedTitle && localization && LocalizedTitleKey) {
            localizedTitle = localization->GetLocalString(ELocalization::Rus, LocalizedTitleKey);
        }
        if (!TDecoder::DecodeEnumCode(Result, config, code)) {
            ASSERT_WITH_LOG(false) << "cannot decode code from session result" << Endl;
            return;
        }
        TCodedException exception(code);
        exception.SetDetails(GetCommentJson());
        throw exception.AddErrorCode("session_info").SetDebugMessage(localizedMessage) << "session failure";
    }

    ESessionResult TInfoEntitySession::DetachResult() {
        ESessionResult result = Result;
        ClearErrors();
        return result;
    }

    void TInfoEntitySession::ClearErrors() {
        AtomicSet(ErrorsInfoCounter, 0);
        Result = ESessionResult::Success;
    }

    TInfoEntitySession::TSessionInfoWriter TInfoEntitySession::Error(const TString& problemMessage, const ESessionResult result /*= ESessionResult::InternalError*/) {
        return SetErrorInfo("error", problemMessage, result);
    }

    TInfoEntitySession::TSessionInfoWriter TInfoEntitySession::Error(const TString& problemMessage, const bool isFinal) {
        if (isFinal) {
            return SetErrorInfo("session_error", problemMessage, ESessionResult::InternalError);
        } else {
            return TSessionInfoWriter(std::move(TFLEventLog::Error(problemMessage)("session_result", ESessionResult::InternalError)("source", "session_error")));
        }
    }

    TInfoEntitySession::TSessionInfoWriter TInfoEntitySession::SetErrorInfo(const TString& source, const TString& info, const ESessionResult result) {
        const i64 counter = AtomicIncrement(ErrorsInfoCounter);
        Y_ASSERT(IsMultiErrors() || counter == 1);
        if (Result == ESessionResult::Success) {
            Result = result;
        }
        return TSessionInfoWriter(std::move(TFLEventLog::Error(info)("session_result", result)("source", source)), *this);
    }

    TInfoEntitySession::TSessionInfoWriter::TSessionInfoWriter(TFLEventLog::TLogWriterContext&& context, TInfoEntitySession& owner)
        : TBase(std::move(context))
        , Owner(&owner) {

    }

    TInfoEntitySession::TSessionInfoWriter::TSessionInfoWriter(TFLEventLog::TLogWriterContext&& context)
        : TBase(std::move(context)) {

    }

    TInfoEntitySession::TSessionInfoWriter::~TSessionInfoWriter() {
        if (Owner) {
            Owner->AddComment(TBase::SerializeToJson());
        }
    }

}
