#include "batch.h"

#include "solver.h"

typedef TLogOutput Log;

void TBatchUpdater::operator()(TAutoPtr<NCore::TPackedMessage> what) const {
PROFILE_ACQUIRE(PROFILE_BATCH_UPDATE)

    switch (what->GetType()) {
    case NCore::THeartbeatMessage::Type: {
        break;
    } case NCore::TDefineMessage::Type: {
        NCore::TDefineMessage message(*what);

        try {
            if (message.HasOrigin()) {
                TBatch::const_iterator origin = Batch->find(message.GetOrigin());

                if (origin == Batch->end()) {
                    throw TRequest::TRejectException() << "origin "sv << message.GetOrigin() << " is not defined"sv;
                }

                TRequest newRequest(Batch, message, *origin);

                TRequest* request = &Batch->find_or_insert(TRequest(Batch, message.GetKey()));
                DoSwap<TClaims>(*request, newRequest);

                Log() << "batch "sv << Batch->Id << ": "sv << request->Key << " \""sv << request->Label << "\" defined as clone of "sv << origin->Key << " \""sv << origin->Label << '"';
            } else {
                TRequest newRequest(Batch, message);

                TRequest* request = &Batch->find_or_insert(TRequest(Batch, message.GetKey()));
                DoSwap<TClaims>(*request, newRequest);

                Log() << "batch "sv << Batch->Id << ": "sv << request->Key << " \""sv << request->Label << "\" defined ("sv << static_cast<TClaims>(*request) << ')';
            }

            const TBatch::const_iterator request = Batch->find(message.GetKey());

            if (request != Batch->end() && request->State != TRequest::DEFINED) {
                AtomicSet(NGlobal::NeedSolve, true);
            }
        } catch (const TRequest::TRejectException& e) {
            Batch->Enqueue(NCore::TRejectMessage(message.GetActionId(), message.GetKey(), e.what()).Pack());
            Log() << "batch "sv << Batch->Id << ": "sv << message.GetKey() << " rejected: "sv << e.what();
        }

        break;
    } case NCore::TUndefMessage::Type: {
        NCore::TUndefMessage message(*what);

        const TBatch::const_iterator request = Batch->find(message.GetKey());

        if (request == Batch->end()) {
            break;
        }

        if (request->State != TRequest::DEFINED) {
            AtomicSet(NGlobal::NeedSolve, true);
        }

        TString label = request->Label;
        Batch->erase(request);
        Log() << "batch "sv << Batch->Id << ": "sv << message.GetKey() << " \""sv << label << "\" definition erased"sv;

        break;
    } case NCore::TDetailsMessage::Type: {
        NCore::TDetailsMessage message(*what);

        const TBatch::iterator request = Batch->find(message.GetKey());

        if (request == Batch->end()) {
            break;
        }

        Log() << "batch "sv << Batch->Id << ": "sv << request->Key << " \""sv << request->Label << "\" details received ("sv << message.GetDetails().ShortDebugString() << ')';

        if (message.GetDetails().HasPriority()) {
            const double& priority = message.GetDetails().GetPriority();

            if (priority <= 0.0) {
                request->Priority = 0;
            } else if (priority >= 1.0) {
                request->Priority = Max<unsigned>();
            } else {
                request->Priority = priority * Max<unsigned>();
            }
        }

        if (message.GetDetails().HasDuration()) {
            request->Duration = TDuration::Seconds(message.GetDetails().GetDuration());
        }

        if (message.GetDetails().HasLabel()) {
            request->Label = message.GetDetails().GetLabel();
        }

        break;
    } case NCore::TRequestMessage::Type: {
        NCore::TRequestMessage message(*what);

        const TBatch::iterator request = Batch->find(message.GetKey());

        if (request == Batch->end()) {
            break;
        }

        request->GrantActionId = message.GetActionId();

        request->IndexNumber = AtomicIncrement(NGlobal::LastRequestIndexNumber);

        AtomicSet(NGlobal::NeedSolve, true);

        if (request->State != TRequest::REQUESTED) {
            request->State = TRequest::REQUESTED;

            TGuard<TSpinLock> guard(*Requests->Lock);
            request->LinkBefore(&*Requests->Requested->End());
            guard.Release();
        }

        Log() << "batch "sv << Batch->Id << ": "sv << request->Key << " \""sv << request->Label << "\" requested"sv;

        break;
    } case NCore::TClaimMessage::Type: {
        NCore::TClaimMessage message(*what);

        const TBatch::iterator request = Batch->find(message.GetKey());

        if (request == Batch->end()) {
            break;
        }

        if (request->State != TRequest::GRANTED) {
            if (request->State == TRequest::REQUESTED) {
                AtomicSet(NGlobal::NeedSolve, true);
            }

            request->State = TRequest::GRANTED;

            TGuard<TSpinLock> guard(*Requests->Lock);
            request->LinkBefore(&*Requests->Granted->End());
            guard.Release();

            request->DisclaimForecast = request->Duration != TDuration::Max() ? request->Duration.ToDeadLine() : TInstant::Zero();

            Log() << "batch "sv << Batch->Id << ": "sv << request->Key << " \""sv << request->Label << "\" claimed"sv;
        }

        Batch->Enqueue(NCore::TGrantMessage(message.GetActionId(), message.GetKey()).Pack());

        break;
    } case NCore::TDisclaimMessage::Type: {
        NCore::TDisclaimMessage message(*what);

        const TBatch::iterator request = Batch->find(message.GetKey());

        if (request == Batch->end() || request->State == TRequest::DEFINED) {
            break;
        }

        AtomicSet(NGlobal::NeedSolve, true);

        request->State = TRequest::DEFINED;

        TGuard<TSpinLock> guard(*Requests->Lock);
        request->Unlink();
        guard.Release();

        Log() << "batch "sv << Batch->Id << ": "sv << request->Key << " \""sv << request->Label << "\" disclaimed"sv;

        break;
    } case NCore::TGroupMessage::Type: {
        NCore::TGroupMessage message(*what);

        Batch->GroupIncoming(message.GetCount());

        break;
    } default:
        Log() << "batch "sv << Batch->Id << ": Bad message type "sv << what->GetType() << ", ignoring"sv;
    }
}

void TBatchProcessor::Process(void*) {
    THolder<TBatchProcessor, TDestructor> This(this);

    try {
        if (!Recv && !Send) {
            ythrow NCore::TDisconnection() << "connection error"sv;
        }

        if (Recv && Batch->Recv()) {
            Batch->LastHeartbeat = Now;
        }
        if (Send) {
            Batch->Send();
        }

        Batch->UpdatePoller(*Poller);
    } catch (const NCore::TBadVersion& e) {
        Log() << "batch "sv << Batch->Id << ": Disconnecting: Bad protocol version: "sv << e.what();

        Poller->Remove(*Batch->GetSocket());

        Batch->ResetSocket(nullptr);
    } catch (const NCore::TDisconnection& e) {
        Log() << "batch "sv << Batch->Id << ": Disconnecting: "sv << e.what();

        Poller->Remove(*Batch->GetSocket());

        Batch->ResetSocket(nullptr);

        AtomicSet(NGlobal::NeedSolve, true);
    } catch (...) {
        Log() << "batch "sv << Batch->Id << ": Exception caught: "sv << CurrentExceptionMessage();
    }
}
