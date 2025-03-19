#include "abstract.h"
#include "no_balancing.h"

namespace NCS {
    namespace NStorage {
        namespace NBalancing {

            const TString IBalancingPolicy::DefaultBalancingClass = TNoBalancingPolicy::GetTypeName();

            bool IBalancingPolicy::CalculateObjectFeaturesNative(const TVector<TBalancingObject*>& objectsExt) const noexcept {
                auto gLogging = TFLRecords::StartContext().SignalId("pg_balancer.rebuild")("&db_name", DBName);
                try {
                    TVector<TBalancingObject*> objects;
                    if (!!ExternalSettings) {
                        const TDuration d = ExternalSettings->GetValueDef<TDuration>("pg_balancing." + DBName + ".replication_lag_limit", TDuration::Seconds(10));
                        if (d) {
                            for (auto&& i : objectsExt) {
                                if (i->GetReplicationLag() > d) {
                                    i->SetBalancingPriority(0).SetBalancingWeight(1).SetEnabled(false);
                                } else {
                                    objects.emplace_back(i);
                                }
                            }
                        } else {
                            objects = objectsExt;
                        }
                    } else {
                        objects = objectsExt;
                    }
                    if (!DoCalculateObjectFeatures(objects)) {
                        return false;
                    }
                    ui64 pMax = 0;
                    for (auto&& i : objects) {
                        if (i->GetBalancingPriority() > pMax) {
                            pMax = i->GetBalancingPriority();
                        }
                    }
                    for (auto&& i : objects) {
                        if (i->GetBalancingPriority() < pMax) {
                            i->SetBalancingWeight(0);
                        }
                    }
                    for (auto&& wHost : objects) {
                        auto gLoggingHost = TFLRecords::StartContext()("&replica", wHost->GetHostName());
                        TFLEventLog::JustLTSignal("", wHost->GetBalancingPriority())("&code", "b_priority");
                        TFLEventLog::JustLTSignal("", wHost->GetBalancingWeight())("&code", "b_weight");
                        TFLEventLog::JustLTSignal("", wHost->GetSettingsWeight())("&code", "s_weight");
                        TFLEventLog::JustLTSignal("", wHost->GetEnabled() ? 1 : 0)("&code", "enabled");
                        TFLEventLog::JustLTSignal("", (double)wHost->GetReplicationLag().MicroSeconds() / 1000000.0)("&code", "lag");
                        TFLEventLog::JustLTSignal("", wHost->GetPingDuration().MilliSeconds())("&code", "ping_duration");
                    }
                    TFLEventLog::JustSignal()("&code", "success");
                    return true;
                } catch (...) {
                    TFLEventLog::Error("cannot calculate object features for balancing")("message", CurrentExceptionMessage()).Signal()("&code", "failed");
                    return false;
                }
            }

            NBalancing::TBalancingPolicy::TBalancingPolicy() {
                Object = new TNoBalancingPolicy;
            }

            NCS::NStorage::NBalancing::TBalancingPolicy TBalancingPolicyOperator::GetBalancingPolicy() const {
                TReadGuard rg(Mutex);
                return CachedPolicy;
            }

            NCS::NStorage::NBalancing::TBalancingPolicy TBalancingPolicyOperator::ConstructBalancingPolicy(const TString& defaultPolicyClass) {
                TBalancingPolicy result;
                const TString balancerPolicyConfig = Settings ? Settings->GetValueDef<TString>("db_balancer." + DBName + ".policy", "") : "";
                NJson::TJsonValue jsonInfo;
                if (!balancerPolicyConfig) {
                    THolder<IBalancingPolicy> policy = IBalancingPolicy::TFactory::MakeHolder(defaultPolicyClass);
                    if (!policy) {
                        TFLEventLog::JustSignal("db_balancer")("&db_name", DBName)("&code", "incorrect_default_policy");
                    } else {
                        result = policy.Release();
                    }
                } else if (!NJson::ReadJsonFastTree(balancerPolicyConfig, &jsonInfo)) {
                    TFLEventLog::JustSignal("db_balancer")("&db_name", DBName)("&code", "cannot_parse_balancer_config_as_json");
                } else if (!result.DeserializeFromJson(jsonInfo)) {
                    TFLEventLog::JustSignal("db_balancer")("&db_name", DBName)("&code", "cannot_parse_balancer_config");
                }
                result->SetExternalSettings(Settings);
                result->SetDBName(DBName);
                TWriteGuard wg(Mutex);
                std::swap(CachedPolicy, result);
                return CachedPolicy;
            }

        }
    }
}
