#include "manager.h"

#include <kernel/common_server/proposition/actions/send_request.h>

namespace NCS {
    namespace NPropositions {

        bool TDBManager::ApplyFinalVerdict(const TDBProposition& proposition, const EVerdict verdict, const TString& userId, NCS::TEntitySession& session, IReplyContext::TPtr requestContext) const {
            if (verdict == EVerdict::Comment) {
                return true;
            }
            if (verdict == EVerdict::Decline) {
                session.SetComment("decline");
                return RemoveObject(proposition.GetPropositionId(), proposition.GetRevision(), userId, session);
            }
            if (verdict == EVerdict::Confirm) {
                if (!proposition.GetProposedObject().TuneAction(session, requestContext)) {
                    return false;
                }
                if (!ExecuteProposition(proposition, userId, session)) {
                    TFLEventLog::Log("proposition was not executed");
                    return false;
                }
                return true;
            }
            session.Error("undefined verdict");
            return false;
        }

        bool TDBManager::DoRebuildCacheUnsafe() const {
            if (!TBase::DoRebuildCacheUnsafe()) {
                return false;
            }
            IndexByPropositionObjectId.Initialize(Objects);
            IndexByPropositionCategoryId.Initialize(Objects);
            return true;
        }

        void TDBManager::DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TDBProposition>& ev) const {
            TBase::DoAcceptHistoryEventBeforeRemoveUnsafe(ev);
            IndexByPropositionObjectId.Remove(ev);
            IndexByPropositionCategoryId.Remove(ev);
        }

        void TDBManager::DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TDBProposition>& ev, TDBProposition& object) const {
            TBase::DoAcceptHistoryEventAfterChangeUnsafe(ev, object);
            IndexByPropositionObjectId.Upsert(ev);
            IndexByPropositionCategoryId.Upsert(ev);
        }

        bool TDBManager::Verdict(const TDBVerdict& verdict, const TString& userId, NCS::TEntitySession& session, IReplyContext::TPtr requestContext) const {
            auto gLogging = TFLRecords::StartContext()("proposition_id", verdict.GetPropositionId())("expected_revision", verdict.GetPropositionRevision());
            TMaybe<TDBProposition> proposition;
            if (!RestoreObject(verdict.GetPropositionId(), proposition, session)) {
                return false;
            }
            if (!proposition) {
                session.Error("incorrect proposition_id restored");
                return false;
            }
            if (proposition->GetRevision() != verdict.GetPropositionRevision()) {
                session.Error("inconsistency proposition_id")("actual_revision", proposition->GetRevision());
                return false;
            }
            EVerdict finalVerdict;
            if (!Verdicts->Verdict(*proposition, verdict, userId, session, finalVerdict)) {
                return false;
            }
            gLogging("verdict", finalVerdict);

            return ApplyFinalVerdict(*proposition, finalVerdict, userId, session, requestContext);
        }

        TVector<TDBProposition> TDBManager::GetObjectsByPropositionObjectId(const TString& key) const {
            TReadGuard rg(TBase::MutexCachedObjects);
            return IndexByPropositionObjectId.GetObjectsByKey(key);
        }

        TVector<TDBProposition> TDBManager::GetObjectsByPropositionCategoryId(const TString& key) const {
            TReadGuard rg(TBase::MutexCachedObjects);
            return IndexByPropositionCategoryId.GetObjectsByKey(key);
        }

        bool TDBVerdictsManager::GetVerdicts(const TDBProposition& proposition, TVector<TDBVerdict>& verdicts, NCS::TEntitySession& session) const {
            return RestoreObjectsBySRCondition(verdicts, TSRCondition().Init<TSRBinary>("proposition_id", proposition.GetPropositionId()), &session);
        }

        bool TDBVerdictsManager::Verdict(const TDBProposition& proposition, const TMaybe<TDBVerdict>& verdict, const TString& userId, NCS::TEntitySession& session, EVerdict& finalVerdict) const {
            if (!!verdict && !AddObjects({ *verdict }, userId, session)) {
                return false;
            }
            TVector<TDBVerdict> verdicts;
            if (!GetVerdicts(proposition, verdicts, session)) {
                return false;
            }
            finalVerdict = proposition.GetPropositionPolicy().BuildFinalVerdict(proposition, verdicts);
            return true;
        }

        bool TDBManager::ExecuteProposition(const TDBProposition& proposition, const TString& userId, NCS::TEntitySession& session) const {
            TStringBuilder sb;
            sb << "confirm";
            if (proposition.GetProposedObject().IsActual(Server)) {
                if (!proposition.GetProposedObject().Execute(userId, Server)) {
                    session.Error("cannot execute proposition");
                    return false;
                }
                sb << " result: " << proposition.GetProposedObject()->GetResult();
                session.SetComment(sb);
                if (!RemoveObject(proposition.GetPropositionId(), proposition.GetRevision(), userId, session)){
                    session.Error("cannot remove proposition");
                    return false;
                }
                return true;
            } else {
                sb << " result: execution is not actual" << Endl;
                session.SetComment(sb);
                return true;
            }
        }
    }
}
