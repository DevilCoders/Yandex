#include "common.h"

#include <kernel/lemmer/alpha/abc.h>

namespace NDictBuild {
    void Normalize(IInputStream& in, IOutputStream& out, const TDictBuildParams& params) {
        const NLemmer::TAlphabetWordNormalizer* normalizer = NLemmer::GetAlphaRulesUnsafe(params.Language);
        Y_ENSURE(normalizer, "no normalizer for " << params.Language);

        const size_t bufferSize = 16 * 1024 * 1024;
        static wchar16 buffer[bufferSize];
        NLemmer::TConvertMode convertMode = NLemmer::TConvertMode(NLemmer::CnvDecompose, NLemmer::CnvCompose, NLemmer::CnvLowCase) | NLemmer::TConvertMode(NLemmer::CnvDerenyx, NLemmer::CnvConvert);

        TUtf16String line;
        while (in.ReadLine(line)) {
            size_t pos1 = line.find(' ');
            size_t pos2 = line.find('\t');
            size_t pos = pos1 < pos2 ? pos1 : pos2;
            size_t len = (pos != TUtf16String::npos ? pos : line.size());
            NLemmer::TConvertRet r = normalizer->Normalize(line.data(), len, buffer, bufferSize, convertMode);
            Y_UNUSED(r);
            out << buffer << line.substr(len) << Endl;
        }
    }
}
