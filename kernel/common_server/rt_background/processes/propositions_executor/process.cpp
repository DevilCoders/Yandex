#include "process.h"

#include <kernel/common_server/proposition/verdict.h>
#include <kernel/common_server/proposition/manager.cpp>
#include <kernel/common_server/server/server.h>

namespace NCS {
    namespace NPropositions {
        TPropositionsExecutor::TFactory::TRegistrator<TPropositionsExecutor> TPropositionsExecutor::Registrator(TPropositionsExecutor::GetTypeName());

        TAtomicSharedPtr<IRTBackgroundProcessState> TPropositionsExecutor::DoExecute(
                TAtomicSharedPtr<IRTBackgroundProcessState> /*state*/, const TExecutionContext& context) const {
            const auto& server = context.GetServer();
            if (!server.GetPropositionsManager()) {
                TFLEventLog::Error("propositions_manager_not_configured");
                return nullptr;
            }

            const auto* manager = server.GetPropositionsManager();
            TVector<TDBProposition> records;
            manager->GetAllObjects(records);
            for (auto&& proposition : records) {
                if (proposition.GetProposedObject().IsActual(server)) {
                    EVerdict finalVerdict;
                    TVector<TDBVerdict> verdicts;
                    {
                        auto session = manager->BuildNativeSession(true);
                        if (!manager->GetVerdictManager()->GetVerdicts(proposition, verdicts, session)) {
                            TFLEventLog::Error("falied when restore verdicts");
                            return nullptr;
                        }
                    }
                    finalVerdict = proposition.GetPropositionPolicy().BuildFinalVerdict(proposition, verdicts);
                    if (finalVerdict == EVerdict::Confirm) {
                        auto session = manager->BuildNativeSession(false);
                        if (!manager->ExecuteProposition(proposition, GetRTProcessName(), session) || !session.Commit()) {
                            TFLEventLog::Error("proposition was not execute");
                            return nullptr;
                        }
                    }
                }
            }
            return MakeAtomicShared<IRTBackgroundProcessState>();
        }
    }
}
