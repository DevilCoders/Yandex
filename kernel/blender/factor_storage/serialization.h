#pragma once

#include <kernel/blender/factor_storage/protos/storage.pb.h>

#include <util/generic/vector.h>
#include <util/generic/hash.h>

namespace NBlender::NProtobufFactors {
    using TProtoStorages = NBlender::NProto::TFactorStorages;

    void CompressString(TString& out, const TString& s, bool base64Encode=true);
    void DecompressString(TString& out, const TString& s, bool base64Decode=true);
    // nothing to check during proto compression
    void CompressProto(TString& out, const TProtoStorages& protoStorages, bool base64Encode=true);
    bool DecompressProto(TProtoStorages& protoStorages, const TString& compressed, TString* error = nullptr, bool base64Decode=true);

    void Compress(TString& out, const TVector<float>* staticFactors, const THashMap<TString, TVector<float>>* dynamicFactors, bool base64Encode=true);
    bool Decompress(TVector<float>* staticFactors, THashMap<TString, TVector<float>>* dynamicFactors, const TString& compressed, TString* error=nullptr, bool base64Decode=true);
}
