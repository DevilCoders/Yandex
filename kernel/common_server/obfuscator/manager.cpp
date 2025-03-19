#include "manager.h"

#include <kernel/common_server/obfuscator/obfuscators/with_policy.h>
#include <kernel/common_server/obfuscator/obfuscators/obfuscate_operator.h>
#include <kernel/common_server/obfuscator/obfuscators/fake.h>

#include <tuple>

namespace NCS {
    namespace NObfuscator {

        TObfuscatorsPool::TObfuscatorsPool(const TVector<TDBObfuscator>& objectsExt) {
            TVector<TDBObfuscator> objects = objectsExt;
            AddDefaultObfuscators(objects);
            const auto compare = [](const TDBObfuscator& left, const TDBObfuscator& right) -> bool {
                return std::make_tuple(left.GetPriority(), left->GetRulesCount(), left.GetObfuscatorId()) > std::make_tuple(right.GetPriority(), right->GetRulesCount(), right.GetObfuscatorId());
            };
            std::sort(objects.begin(), objects.end(), compare);
            for (auto&& i : objects) {
                Objects[i->GetContentType()].emplace_back(std::move(i));
            }
        }

        void TObfuscatorsPool::AddDefaultObfuscators(TVector<TDBObfuscator>& objects) const {
            {
                auto headersFnvHashObfuscator = MakeHolder<TObfuscatorWithPolicy>();
                headersFnvHashObfuscator->AddRules("header", {"Authorization", "X-Ya-Service-Ticket", "X-Ya-User-Ticket", "X-Bank-Token", "Cookie"});

                TInterfaceContainer<IObfuscateOperator> hashOperatorContainer;
                hashOperatorContainer.SetObject(IObfuscateOperator::TFactory::Construct(ToString(IObfuscateOperator::EPolicy::ObfuscateWithFnvHash)));
                headersFnvHashObfuscator->SetObfuscateOperator(hashOperatorContainer);

                headersFnvHashObfuscator->SetContentType(IObfuscator::EContentType::Header);
                TDBObfuscator object;
                object.SetObject(headersFnvHashObfuscator.Release());
                object.SetPriority(1);
                objects.push_back(object);
            }
            {
                auto dataTransformationObfuscator = MakeHolder<TFakeObfuscator>();
                dataTransformationObfuscator->AddRules("obfuscated_entity", {"request_body"});
                dataTransformationObfuscator->SetContentType(IObfuscator::EContentType::Json);
                TDBObfuscator object;
                object.SetObject(dataTransformationObfuscator.Release());
                object.SetPriority(1);
                objects.push_back(object);
            }
        }

        TVector<TDBObfuscator> TDBManager::GetObjectsByContentType(const EContentType key) const {
            const auto snapshot = GetSnapshots().GetMainSnapshotPtr();
            const auto mapping = snapshot->GetObjects();
            auto it = mapping.find(key);
            if (it != mapping.end()) {
                return it->second;
            }
            return TVector<TDBObfuscator>();
        }

        IObfuscator::TPtr TDBManager::GetObfuscatorFor(const TObfuscatorKey& key) const {
            auto obfuscators = GetObjectsByContentType(key.GetType());
            if (key.GetType() != EContentType::Header && (obfuscators.size() || !!DefaultObfuscator) && DisabledBySettings()) {
                if (!ICSOperator::GetServer().GetSettings().GetValueDef("obfuscator.disabled.silent", false)) {
                    TFLEventLog::Alert("obfuscators disabled by settings");
                } else {
                    TFLEventLog::Warning("obfuscators disabled by settings");
                }
                return nullptr;
            }
            for (const auto& i : obfuscators) {
                if (i->IsMatch(key))
                    return i.GetPtrAs<IObfuscator>();
            }
            return GetDefaultObfuscator(key);
        }

        IObfuscator::TPtr TDBManager::GetDefaultObfuscator(const TObfuscatorKey& obfuscatorKey) const {
            if (obfuscatorKey.GetType() == EContentType::Header) {
                return nullptr;
            }
            return DefaultObfuscator;
        }

    }
}
