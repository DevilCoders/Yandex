#pragma once

#include <kernel/common_server/rt_background/processes/common/database.h>
#include <kernel/common_server/rt_background/state.h>
#include <kernel/common_server/proposition/object.h>

namespace NCS {
    namespace NPropositions {
        class TPropositionsExecutor: public IRTRegularBackgroundProcess {
        private:
            using TBase = IRTRegularBackgroundProcess;
            static TFactory::TRegistrator<TPropositionsExecutor> Registrator;

            virtual TAtomicSharedPtr<IRTBackgroundProcessState> DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> state, const TExecutionContext& context) const override;

        public:
            static TString GetTypeName() {
                return "propositions_executor";
            }

            virtual TString GetType() const override {
                return GetTypeName();
            }
        };
    }
}
