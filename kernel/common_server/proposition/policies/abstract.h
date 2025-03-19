#pragma once
#include <kernel/common_server/library/interfaces/container.h>
#include <library/cpp/object_factory/object_factory.h>
#include <kernel/common_server/proposition/common.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {
    namespace NPropositions {
        class TDBProposition;
        class TDBVerdict;

        class IPropositionPolicy {
        public:
            using TPtr = TAtomicSharedPtr<IPropositionPolicy>;
            using TFactory = NObjectFactory::TObjectFactory<IPropositionPolicy, TString>;
            virtual ~IPropositionPolicy() = default;
            virtual EVerdict BuildFinalVerdict(const TDBProposition& proposition, const TVector<TDBVerdict>& verdicts) const = 0;
            virtual TString GetClassName() const = 0;
            virtual NJson::TJsonValue SerializeToJson() const = 0;
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const = 0;
            virtual bool IsValid(const IBaseServer& server) const = 0;
        };

        class TPropositionPolicyContainer: public TBaseInterfaceContainer<IPropositionPolicy> {
        private:
            using TBase = TBaseInterfaceContainer<IPropositionPolicy>;
        public:
            using TBase::TBase;
            EVerdict BuildFinalVerdict(const TDBProposition& proposition, const TVector<TDBVerdict>& verdicts) const {
                if (!Object) {
                    return EVerdict::Comment;
                } else {
                    return Object->BuildFinalVerdict(proposition, verdicts);
                }
            }

            bool IsValid(const IBaseServer& server) const {
                if (!Object) {
                    TFLEventLog::Error("incorrect internal object for validity check");
                    return false;
                }
                return Object->IsValid(server);
            }
        };
    }
}

