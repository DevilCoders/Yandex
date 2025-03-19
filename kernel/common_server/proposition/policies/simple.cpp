#include "simple.h"
#include <kernel/common_server/proposition/object.h>
#include <kernel/common_server/proposition/verdict.h>
#include <kernel/common_server/proposition/manager.h>

namespace NCS {

    namespace NPropositions {

        TSimplePropositionPolicy::TFactory::TRegistrator<TSimplePropositionPolicy> TSimplePropositionPolicy::Registrator(TSimplePropositionPolicy::GetTypeName());

        NCS::NPropositions::EVerdict TSimplePropositionPolicy::BuildFinalVerdict(const TDBProposition& proposition, const TVector<TDBVerdict>& verdicts) const {
            TSet<TString> userIds;
            for (auto&& i : verdicts) {
                if (i.GetVerdict() == EVerdict::Decline) {
                    return EVerdict::Decline;
                }
                if (i.GetPropositionRevision() && i.GetPropositionRevision() != proposition.GetRevision()) {
                    continue;
                }
                if (i.GetVerdict() == EVerdict::Confirm) {
                    userIds.emplace(i.GetSystemUserId());
                }
                if (userIds.size() >= GetNeedApprovesCount()) {
                    return EVerdict::Confirm;
                }
            }
            return EVerdict::Comment;
        }

        NCS::NScheme::TScheme TSimplePropositionPolicy::GetScheme(const IBaseServer& server) const {
            ui32 limit = 0;
            if (auto manager = server.GetPropositionsManager()) {
                limit = manager->GetConfig().GetNeedApprovesCount();
            }
            NCS::NScheme::TScheme result;
            result.Add<TFSNumeric>("app_count", "Требуемое количество подтверждений").SetMin(limit);
            return result;
        }

        bool TSimplePropositionPolicy::IsValid(const IBaseServer& server) const {
            auto limit = server.GetPropositionsManager()->GetConfig().GetNeedApprovesCount();
            if (NeedApprovesCount < limit) {
                TFLEventLog::Info("proposition")("required amount of approves count is lesser than limit = ", limit);
                return false;
            }
            return true;
        }
    }
}
