#include "serialization.h"

#include <library/cpp/streams/lz/lz.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/stream/str.h>

namespace NBlender::NProtobufFactors {
    void CompressString(TString& out, const TString& s, bool base64Encode) {
        TString compressedData;
        TStringInput input(s);
        TStringOutput output(compressedData);
        TLz4Compress compressor(&output);
        TransferData(&input, &compressor);
        compressor.Finish();
        out = base64Encode ? Base64Encode(compressedData) : compressedData;
    }

    void DecompressString(TString& out, const TString& s, bool base64Decode) {
        TString inputStr = base64Decode ? Base64Decode(s) : s;
        TStringInput inputDec(inputStr);
        TStringOutput outputDec(out);
        TLz4Decompress decompressor(&inputDec);
        TransferData(&decompressor, &outputDec);
    }

    void CompressProto(TString& out, const TProtoStorages& protoStorages, bool base64Encode) {
        TString serializedStr = protoStorages.SerializeAsString();
        CompressString(out, serializedStr, base64Encode);
    }

    bool DecompressProto(TProtoStorages& protoStorages, const TString& compressed, TString* error, bool base64Decode) {
        TString out;
        try {
            DecompressString(out, compressed, base64Decode);
        }
        catch (const yexception& e) {
            if (error) {
                *error = e.what();
            }
            return false;
        }
        if (!protoStorages.ParseFromString(out)) {
            if (error) {
                *error = "Can't load from serialized proto message";
            }
            return false;
        }
        return true;
    }

    void Compress(TString& out, const TVector<float>* staticFactors, const THashMap<TString, TVector<float>>* dynamicFactors, bool base64Encode) {
        TProtoStorages protoStorages;
        if (staticFactors) {
            *protoStorages.MutableStaticStorage()->MutableValue() = {staticFactors->begin(), staticFactors->end()};
        }
        if (dynamicFactors) {
            auto* dynamicStorage = protoStorages.MutableDynamicStorage();
            dynamicStorage->MutableDynamicFactorGroup()->Reserve(dynamicFactors->size());
            for (auto& [key, value]: *dynamicFactors) {
                 auto* dynamicGroup = dynamicStorage->AddDynamicFactorGroup();
                 dynamicGroup->SetKey(key.Data());
                 *dynamicGroup->MutableValue() = {value.begin(), value.end()};
            }
        }
        CompressProto(out, protoStorages, base64Encode);
    }

    bool Decompress(TVector<float>* staticFactors, THashMap<TString, TVector<float>>* dynamicFactors, const TString& compressed, TString* error, bool base64Decode) {
        TProtoStorages protoStorages;
        if (!DecompressProto(protoStorages, compressed, error, base64Decode)) {
            return false;
        }
        if (staticFactors) {
            const auto& staticFactorsValue = protoStorages.GetStaticStorage().GetValue();
            *staticFactors = {staticFactorsValue.begin(), staticFactorsValue.end()};
        }
        if (dynamicFactors) {
            for (const auto& group : protoStorages.GetDynamicStorage().GetDynamicFactorGroup()) {
                const auto& dynamicFactorsValue = group.GetValue();
                (*dynamicFactors)[group.GetKey()] = {dynamicFactorsValue.begin(), dynamicFactorsValue.end()};
            }
        }
        return true;
    }
}
