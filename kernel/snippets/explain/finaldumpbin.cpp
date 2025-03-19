#include "finaldumpbin.h"

#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/factors/factor_storage.h>

#include <util/string/cast.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/stream/zlib.h>

namespace NSnippets {
    void TFinalFactorsDumpBinary::OnBestFinal(const TSnip& snip, bool isByLink) {
        if (isByLink && Result.size()) {
            return;
        }
        if (!snip.Factors.Size())
            return;
        TString dumped = DumpFactors(snip, FactorsToDump);
        Result = "binaryFactors=" + dumped + ";";
        if (Result.size() > 4096) {
            Result.clear();
        }
    }

    TString TFinalFactorsDumpBinary::DumpFactors(const TSnip& snip, const TFactorsToDump& factorsToDump) {
        TVector<float> factors;
        for (auto&& range : factorsToDump.FactorRanges) {
            for (size_t num = range.first; num <= range.second; ++num) {
                if (num >= snip.Factors.Size()) {
                    break;
                }
                factors.push_back(snip.Factors[num]);
            }
        }
        TStringStream rfs;
        TZLibCompress compressor(&rfs, ZLib::ZLib);
        if (factors.empty()) {
            compressor.Write(reinterpret_cast<const char*>(snip.Factors.Ptr(0)), sizeof(float) * snip.Factors.Size());
        } else {
            compressor.Write(reinterpret_cast<const char*>(factors.data()), sizeof(float) * factors.size());
        }
        if (factorsToDump.DumpFormulaWeight) {
            float weight = snip.Weight;
            compressor.Write(&weight, sizeof(float));
        }
        compressor.Finish();
        return Base64Encode(rfs.Str());
    }
}
