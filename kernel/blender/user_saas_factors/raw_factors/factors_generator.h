#pragma once

#include <kernel/blender/user_saas_factors/protos/user_saas_factors.pb.h>
#include <kernel/blender/user_saas_factors/raw_factors/common.h>
#include <kernel/dssm_applier/nn_applier/lib/states.h>
#include <kernel/querydata/client/querydata.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/yson/node/node.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NBlender::NUserFactors {
    struct TRawIntentFactors {
        TRawIntentFactors(const TString& intent, const int version, const TVector<float>& factors = { })
            : Intent(intent)
            , Version(version)
            , Factors(factors)
        {
        }

        TString GetFactorsId() const;

        TString Intent;
        int Version;
        TVector<float> Factors;
    };

    template<typename TLogger>
    NBlenderUserFactors::NProto::TUserFactors ParseUserFactorsFromString(
        const TStringBuf dataStr,
        TLogger& logger
    )
    {
        NBlenderUserFactors::NProto::TUserFactors userFactors;
        try {
            const auto base64Data = NSc::TValue::FromJsonThrow(dataStr).Get("0");
            if (!base64Data.StringEmpty()) {
                TString decodedData;
                try {
                    Base64Decode(base64Data, decodedData);
                } catch (const yexception& ex) {
                    ythrow yexception() << "pers_saas: cannot decode data from base64. base64(data) = " << base64Data;
                }
                if (!userFactors.ParseFromString(decodedData)) {
                    ythrow yexception() << "pers_saas: cannot deserialize protobuf TUserFactors from base64(data) = " << base64Data;
                }
            }
        } catch (...) {
            logger << CurrentExceptionMessage();
        }
        return userFactors;
    }

    template<typename TLogger>
    NBlenderUserFactors::NProto::TUserFactors ParseUserFactorsFromQueryData(
        const NQueryData::TQueryDataWrapper& qd,
        TLogger& logger
    )
    {
        NQueryData::TKeys keys;
        qd.GetSourceKeys(SAAS_NAMESPACE, keys);
        if (keys.empty()) {
            return NBlenderUserFactors::NProto::TUserFactors();
        }
        if (keys.size() > 1) {
            logger << "pers_saas: received too many keys from saas, ignoring all but the first one";
        }
        TStringBuf dataStr;
        qd.GetJson(SAAS_NAMESPACE, *keys[0], dataStr);
        return ParseUserFactorsFromString<TLogger>(dataStr, logger);
    }

    using TRawFactors = TVector<TRawIntentFactors>;
    using TRawEmbeds = TVector<NBlenderUserFactors::NProto::TEmbeds>;
    using TEmbedsValuesMap = THashMap<TString, NNeuralNetApplier::IStatePtr>;


    TRawFactors GenerateRawFactors(const NBlenderUserFactors::NProto::TUserFactors& userFactors);
    TRawEmbeds GenerateRawEmbeds(const NBlenderUserFactors::NProto::TUserFactors& userFactors);

    TEmbedsValuesMap RawEmbedsToEmbedsValuesMap(const TRawEmbeds& rawEmbeds);
}
