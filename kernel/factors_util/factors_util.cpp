#include "factors_util.h"

#include <library/cpp/codecs/float_huffman.h>

#include <util/stream/str.h>
#include <util/stream/zlib.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/system/byteorder.h>


namespace NFactors {

    void ExtractCompressedFactors(const TStringBuf& encodedFactors, TVector<float>& rankingFactors) {
        TString compressedFactors;
        Base64Decode(encodedFactors, compressedFactors);
        TStringInput cs(compressedFactors);
        TZLibDecompress decompress(&cs);
        TString serializedFactors = decompress.ReadAll();
        if (serializedFactors.size() % sizeof(float) != 0)
            ythrow yexception() << "factors string is broken";
        size_t numOfFactors = serializedFactors.size() / sizeof(float);
        rankingFactors.reserve(numOfFactors);
        const float* factors = reinterpret_cast<const float*>(serializedFactors.c_str());
        for (size_t i = 0; i < numOfFactors; ++i) {
            rankingFactors.push_back(LittleToHost(factors[i]));
        }
    }

    /// Does NOT clear destination vector
    void ExtractHuffCompressedFactors(const TStringBuf& encodedFactors, TVector<float>& rankingFactors) {
        TString compressedFactors;
        Base64Decode(encodedFactors, compressedFactors);
        TVector<float> newFactors = NCodecs::NFloatHuff::Decode(compressedFactors);
        rankingFactors.insert(rankingFactors.end(), newFactors.begin(), newFactors.end());
    }

} // namespace NFactors

