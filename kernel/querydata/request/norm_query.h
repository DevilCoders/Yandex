#pragma once

#include "norm_util.h"
#include "qd_rawkeys.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <util/string/vector.h>

namespace NQueryData {

// TODO: make it language-aware

    TString SimpleNormalization(TStringBuf userquery);
    TString DoppelNormalization(TStringBuf userquery);
    TString DoppelNormalizationW(TStringBuf userquery);
    TString LowerCaseNormalization(TStringBuf userquery);

    inline void Tokenize(TStringBufs& res, TStringBuf query) {
        GetTokens(res, query, ' ');
    }

    inline void GeneratePairs(TStringBufs& res, TDeque<TString>& pool, TStringBuf query) {
        GetPairs(res, pool, query, ' ');
    }

    typedef TString (*DoNormalizeQuery)(TStringBuf);

    TString NormalizeRequestUTF8Wrapper(TStringBuf uq);

    TStringBufs& GetOrMakeNormalization(TSubkeysCache& cache, EKeyType kt, TStringBuf reqfield, DoNormalizeQuery,
                                        TStringBuf uquery);

}
