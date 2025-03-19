#include "versions.h"

#include <kernel/fast_user_factors/protos/user_sessions_stat.pb.h>
#include <kernel/fast_user_factors/factors_utils/factors.h>

#include <util/generic/hash.h>

namespace NFastUserFactors {

    void TVersionDictionary::Init(const TVersionDictionaryProto& proto) {
        ui32 maxFoundVersionId = 0UL;
        for (const auto& version : proto.GetVersions()) {
            maxFoundVersionId = Max(maxFoundVersionId, version.GetId());
        }
        Y_ENSURE(maxFoundVersionId <= MaxVersionId);
        Versions = TVector<TVector<TString>>(maxFoundVersionId + 1UL);

        for (const auto& version : proto.GetVersions()) {
            TVector<TString>& names = Versions[version.GetId()];
            names.reserve(version.NamesSize());
            for (const auto& name : version.GetNames()) {
                names.push_back(name);
            }
        }
    }

    TCountersProto TVersionDictionary::FillNames(const TCountersProto& countersProto) const {
        if (!countersProto.HasVersionId()) {
            return countersProto;
        }

        TCountersProto result;
        if (countersProto.HasTimestamp()) {
            result.SetTimestamp(countersProto.GetTimestamp());
        }

        const TVector<TString>& names = GetCounterNamesByVersionId(countersProto.GetVersionId());
        Y_ENSURE(names.size() == countersProto.CountersSize(), "Number of counters and number of names differ");
        TCountersProto::TCounter* counter;
        for (size_t i = 0; i < names.size(); ++i) {
            counter = result.AddCounters();
            counter->SetName(names[i]);
            counter->SetValue(countersProto.GetCounters(i).GetValue());
        }
        result.SetVersionId(countersProto.GetVersionId());

        return result;
    }

    TQueryAggregation TVersionDictionary::FillNames(const TQueryAggregation& queryAggregation) const {
        TQueryAggregation result;

        if (queryAggregation.HasQueryCounters()) {
            result.MutableQueryCounters()->CopyFrom(FillNames(queryAggregation.GetQueryCounters()));
        }

        TQueryAggregation::TQuerySmthStat* resultQuerySmthStat;
        TKeyCounters* resultKeyCounters;
        for (const auto& querySmthStats : queryAggregation.GetQuerySmthStats()) {
            resultQuerySmthStat = result.AddQuerySmthStats();
            resultQuerySmthStat->SetType(querySmthStats.GetType());

            for (const auto& keyCounters : querySmthStats.GetKeyCounters()) {
                resultKeyCounters = resultQuerySmthStat->AddKeyCounters();
                if (keyCounters.HasKey()) {
                    resultKeyCounters->SetKey(keyCounters.GetKey());
                }
                if (keyCounters.HasHashKey()) {
                    resultKeyCounters->SetHashKey(keyCounters.GetHashKey());
                }
                if (keyCounters.HasCounters()) {
                    resultKeyCounters->MutableCounters()->CopyFrom(FillNames(keyCounters.GetCounters()));
                }
            }
        }

        return result;
    }

    TCountersProto TVersionDictionary::FillVersion(const TCountersProto& countersProto, const ui32 versionId) const {
        Y_ENSURE(versionId < Versions.size());
        THashMap<TString, float> values;
        THashMap<TString, ui8> compressedValues;

        if (countersProto.HasVersionId()) {
            const TVector<TString>& oldNames = GetCounterNamesByVersionId(countersProto.GetVersionId());
            TVector<ui8> compressedCounters;
            for (size_t i = 0; i < oldNames.size(); ++i) {
                if (countersProto.HasCompressedValues()) {
                    compressedValues[oldNames[i]] = countersProto.GetCompressedValues().at(i);
                } else {
                    values[oldNames[i]] = countersProto.GetCounters(i).GetValue();
                }
            }
        } else {
            for (size_t i = 0; i < countersProto.CountersSize(); ++i) {
                Y_ENSURE(countersProto.GetCounters(i).HasName(), "Counters have neither VersionId nor Names");
                values[countersProto.GetCounters(i).GetName()] = countersProto.GetCounters(i).GetValue();
            }
        }

        TCountersProto result;
        if (countersProto.HasTimestamp()) {
            result.SetTimestamp(countersProto.GetTimestamp());
        }
        // ui64 hash hack
        if (countersProto.GetHash64() != 0) {
            result.SetHash64(countersProto.GetHash64()); 
        }

        result.SetVersionId(versionId);

        const TVector<TString>& names = GetCounterNamesByVersionId(versionId);
        bool filled = false;
        TCountersProto::TCounter* counter;
        for (const TString& name : names) {
            if (!compressedValues.empty()) {
                const auto it = compressedValues.find(name);
                if (it != compressedValues.end()) {
                    result.MutableCompressedValues()->append(it->second);
                    filled = true;
                } else {
                    result.MutableCompressedValues()->append(FloatToChar(0.0f));
                }
            } else {
                counter = result.AddCounters();
                const auto it = values.find(name);
                if (it != values.end()) {
                    counter->SetValue(it->second);
                    filled = true;
                } else {
                    counter->SetValue(0.0f);
                }
            }
        }
        return filled ? result : TCountersProto{};
    }

}
