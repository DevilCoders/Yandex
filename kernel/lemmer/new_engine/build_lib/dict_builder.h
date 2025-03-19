#pragma once

#include "common.h"

#include <util/generic/buffer.h>
#include <util/generic/hash.h>
#include <kernel/lemmer/new_engine/build_lib/protos/dict.pb.h>
#include <kernel/lemmer/new_engine/binary_dict/binary_dict_header.h>

namespace NDictBuild {
    class TNewProtoDictBuilder {
    public:
        TNewProtoDictBuilder(
            const TVector<TString>& grammar,
            const TGramIds& grammarAddrs,
            const TDataStatistics& statistics, //Flexies, Schemes, Blocks
            const TVector<TString>& flexTries,
            const TString& endsTrie,
            const TVector<TSinglePackedEnd>& uniqueEnds,
            const TVector<TVector<size_t>>& anasLists,
            const TDataFreq& freqs,
            const TDictBuildParams& params);
        void Save(IOutputStream& out) const;

    private:
        THashMap<size_t, ui32> GrammarAddrs;
        THashMap<size_t, ui32> Flexies;
        THashMap<size_t, TUtf16String> FlexiesText;
        TMap<ui16, TString> Stems;
        TVector<ui32> FlexTriesAddrs;
        NLemmerProto::TLemmerDict Dict;
    };

    class TNewBinaryDictBuilder {
    public:
        TNewBinaryDictBuilder(
            const TVector<TString>& grammar,
            const TGramIds& grammarAddrs,
            const TDataStatistics& statistics, //Flexies, Schemes, Blocks
            const TVector<TString>& flexTries,
            const TString& endsTrie,
            const TVector<TSinglePackedEnd>& uniqueEnds,
            const TVector<TVector<size_t>>& anasLists,
            const TDataFreq& freqs,
            const TDictBuildParams& params);
        void Save(IOutputStream& out) const;

    private:
        TBuffer Buffer;
        THashMap<size_t, ui32> GrammarAddrs;
        THashMap<size_t, ui32> Flexies;
        THashMap<size_t, TUtf16String> FlexiesText;
        TMap<ui16, TString> Stems;
        TVector<ui32> FlexTriesAddrs;
        NNewLemmer::THeader Header;
    };
}
