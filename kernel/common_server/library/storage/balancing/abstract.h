#pragma once

#include "abstract.h"
#include <kernel/common_server/util/datacenter.h>
#include <kernel/common_server/settings/abstract/abstract.h>

namespace NCS {
    namespace NStorage {
        namespace NBalancing {

            class TBalancingObject {
            private:
                CSA_DEFAULT(TBalancingObject, TString, HostName);
                CS_ACCESS(TBalancingObject, TDuration, ReplicationLag, TDuration::Zero());
                CS_ACCESS(TBalancingObject, bool, Enabled, true);
                CS_ACCESS(TBalancingObject, TDuration, PingDuration, TDuration::Zero());
                CS_ACCESS(TBalancingObject, ui64, SettingsWeight, 1);
                CS_ACCESS(TBalancingObject, ui64, BalancingPriority, 0);
                CS_ACCESS(TBalancingObject, ui64, BalancingWeight, 1);
            };

            class IBalancingPolicy {
            public:
                static const TString DefaultBalancingClass;
            private:
                CSA_DEFAULT(IBalancingPolicy, TString, DBName);
                CSA_DEFAULT(IBalancingPolicy, TAtomicSharedPtr<NFrontend::IReadSettings>, ExternalSettings);
            protected:
                TString DCLocalCode;
                virtual const TBalancingObject* DoChooseObject(const TVector<const TBalancingObject*>& objects) const = 0;
                virtual bool DoCalculateObjectFeatures(const TVector<TBalancingObject*>& objects) const = 0;
                virtual bool DoDeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) {
                    return true;
                }
            public:
                IBalancingPolicy()
                    : DCLocalCode(NUtil::DetectDatacenterCode(HostName()))
                {
                }

                using TPtr = TAtomicSharedPtr<IBalancingPolicy>;
                using TFactory = NObjectFactory::TObjectFactory<IBalancingPolicy, TString>;
                virtual ~IBalancingPolicy() = default;

                bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                    return DoDeserializeFromJson(jsonInfo);
                }
                bool CalculateObjectFeaturesNative(const TVector<TBalancingObject*>& objects) const noexcept;

                template <class T>
                bool CalculateObjectFeatures(const TVector<T*>& objects) const noexcept {
                    TVector<TBalancingObject*> correctObjects;
                    for (auto&& i : objects) {
                        correctObjects.emplace_back(i);
                    }
                    return CalculateObjectFeaturesNative(correctObjects);
                }

                template <class T>
                const T* ChooseObject(const TVector<const T*>& objects) const noexcept {
                    auto gLogging = TFLRecords::StartContext().SignalId("pg_balancer")("&db_name", DBName);
                    TVector<const TBalancingObject*> objectsLocal;
                    for (auto&& i : objects) {
                        if (!i->GetEnabled()) {
                            continue;
                        }
                        objectsLocal.emplace_back(i);
                    }
                    if (objectsLocal.empty()) {
                        TFLEventLog::JustSignal("pg_balancer")("&code", "no_balance_hosts");
                        return nullptr;
                    }
                    try {
                        auto result = static_cast<const T*>(DoChooseObject(objectsLocal));
                        if (result) {
                            TFLEventLog::JustSignal("pg_balancer")("&code", "success")("&replica", result->GetHostName());
                        } else {
                            TFLEventLog::JustSignal("pg_balancer")("&code", "cannot_balance");
                        }
                        return result;
                    } catch (...) {
                        TFLEventLog::Error("cannot choose object on balancing")("message", CurrentExceptionMessage());
                        return nullptr;
                    }
                }

                virtual TString GetClassName() const = 0;
            };

            class TBalancingPolicy: public TBaseInterfaceContainer<IBalancingPolicy> {
            private:
                using TBase = TBaseInterfaceContainer<IBalancingPolicy>;
            public:
                using TBase::TBase;
                TBalancingPolicy();
            };

            class TBalancingPolicyOperator {
            private:
                CSA_DEFAULT(TBalancingPolicyOperator, TString, DBName);
                CSA_DEFAULT(TBalancingPolicyOperator, TAtomicSharedPtr<NFrontend::IReadSettings>, Settings);
                mutable TRWMutex Mutex;
                TBalancingPolicy CachedPolicy;
            public:
                using TPtr = TAtomicSharedPtr<TBalancingPolicyOperator>;
                TBalancingPolicyOperator() = default;

                TBalancingPolicyOperator(const TString& dbName, TAtomicSharedPtr<NFrontend::IReadSettings> settings)
                    : DBName(dbName)
                    , Settings(settings)
                {
                    CHECK_WITH_LOG(Settings);
                }

                TBalancingPolicy GetBalancingPolicy() const;
                TBalancingPolicy ConstructBalancingPolicy(const TString& defaultPolicyClass);
            };

            class TBalancingDatabaseConstructionContext: public IDatabaseConstructionContext {
            private:
                CSA_DEFAULT(TBalancingDatabaseConstructionContext, TBalancingPolicyOperator::TPtr, BalancingPolicy);
            public:

            };

        }
        using TBalancingDatabaseConstructionContext = NBalancing::TBalancingDatabaseConstructionContext;
        using TBalancingPolicyOperator = NBalancing::TBalancingPolicyOperator;
        using TBalancingObject = NBalancing::TBalancingObject;
    }
}
