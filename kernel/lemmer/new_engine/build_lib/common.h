#pragma once

#include <util/generic/utility.h>
#include <util/generic/set.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/algorithm.h>
#include <util/ysaveload.h>
#include <util/system/yassert.h>
#include <util/charset/wide.h>
#include <library/cpp/langs/langs.h>
#include <kernel/lemmer/new_engine/binary_dict/binary_dict_header.h>

namespace NDictBuild {
    //using TUsingWideStringType = TVector<wchar32>;
    using TUsingWideStringType = TUtf16String; //happy debugging
    using TUsingWideStringTypeBuf = TWtringBuf;
    TUsingWideStringType StringToUtf32(const TString& word, bool check = true);
    struct TParseConstants {
        const static TUsingWideStringType Underscore;
        const static TUsingWideStringType Tilde;
        const static TUsingWideStringType LeftBracket;
        const static TUsingWideStringType RightBracket;
        const static TUsingWideStringType Empty;
        const static TUsingWideStringType DollarSign;
    };

    //TUsingWideStringType WordToUtf32(TString word, bool shift);
    //TString Utf32ToWord(const TUsingWideStringType& utf32Word);

    struct TDictBuildParams {
        ELanguage Language = LANG_UNK;

        bool SchemesAsList = false;
        bool SchemesLcs = false;

        bool FioMode = false;
        size_t BastardFreq = 1;

        bool UseGeneration = true;
        bool UseDiaMask = false;
        bool UseFreqs = false;
        bool DetachGrammar = true;
        TString Fingerprint;
        bool Verbose = false;
        bool MakeProtoDict = false;
    };

    using TNumberedGrammar = std::pair<TVector<TString>, size_t>;

    struct TNumberedScheme {
        TUsingWideStringType LexEnd;
        size_t LexGram;
        struct TEnd {
            TUsingWideStringType End;
            size_t Gram;
            TEnd(const TUsingWideStringType& end, size_t gram)
                : End(end)
                , Gram(gram)
            {
            }
            bool operator<(const TEnd& other) const {
                return End < other.End || (End == other.End && Gram < other.Gram);
            }
        };
        TVector<TEnd> Ends;
        bool operator<(const TNumberedScheme& other) const {
            return std::tie(LexEnd, LexGram, Ends) < std::tie(other.LexEnd, other.LexGram, other.Ends);
        }
    };

    TString EndsMapToString(const TMap<TUsingWideStringType, TVector<size_t>>& endsMap);

    struct TFullScheme {
        TUsingWideStringType LexEnd;
        size_t LexGram = 0;
        TMap<TUsingWideStringType, TVector<size_t>> EndsMap;
        TSet<TUsingWideStringType> Stems;
        TFullScheme() {
        }
        TFullScheme(const TNumberedScheme& scheme, const TSet<TUsingWideStringType>& stems)
            : LexEnd(scheme.LexEnd)
            , LexGram(scheme.LexGram)
            , Stems(stems)
        {
            for (const auto& end : scheme.Ends) {
                EndsMap[end.End].push_back(end.Gram);
            }
            for (auto& end : EndsMap) {
                Sort(end.second.begin(), end.second.end());
            }
        }
        explicit TFullScheme(const std::pair<const TNumberedScheme&, TSet<TUsingWideStringType>>& p)
            : TFullScheme(p.first, p.second)
        {
        }
        bool operator<(const TFullScheme& other) const {
            size_t size = Stems.size();
            size_t otherSize = other.Stems.size();
            const auto& s1 = EndsMapToString(EndsMap);
            const auto& s2 = EndsMapToString(other.EndsMap);
            return std::tie(size, LexEnd, LexGram, s1, Stems) > std::tie(otherSize, other.LexEnd, other.LexGram, s2, other.Stems);
        }
        Y_SAVELOAD_DEFINE(LexEnd, LexGram, EndsMap, Stems);
    };

    template <class T>
    TVector<T> SortMapByValue(const TMap<T, size_t>& m) {
        TVector<T> result;
        result.reserve(m.size());
        for (const auto& x : m) {
            result.push_back(x.first);
        }
        Sort(result.begin(), result.end(), [&m](const T& a, const T& b) { return m.at(a) < m.at(b); });
        return result;
    }

    struct TSinglePackedEnd {
        size_t Freq;
        size_t Mask;
        NNewLemmer::TEndInfoImpl EndInfo;
        size_t TotalFreq;
        TSinglePackedEnd(size_t freq, size_t mask, NNewLemmer::TEndInfoImpl endInfo, size_t totalFreq);
        bool operator<(const TSinglePackedEnd& other) const;
        bool IsSimilar(const TSinglePackedEnd& other) const;
    };

    struct TGramIds {
        using TData = TMap<TVector<size_t>, size_t>;
        TData Data;
        size_t Len = 0;
        size_t PrepareGramIds(TVector<size_t> gramIds) {
            for (size_t& x : gramIds) {
                x -= 1;
            }
            Sort(gramIds.begin(), gramIds.end());
            if (!Data.contains(gramIds)) {
                Data[gramIds] = Len;
                Len += gramIds.size();
            }
            return Data[gramIds];
        }
        using TGrammarAddr = std::pair<TVector<size_t>, size_t>;
        TVector<TGrammarAddr> GetAddrs() const {
            const auto& data = SortMapByValue(Data);
            TVector<TGrammarAddr> result;
            result.reserve(data.size());
            for (const auto& x : data) {
                Y_VERIFY(!x.empty());
                result.emplace_back(TVector<size_t>(x.begin(), x.end() - 1), x.back());
            }
            return result;
        }
    };

    struct TDataStatistics {
        TMap<TUsingWideStringType, size_t> Flexies;
        struct TScheme {
            size_t FlexId = 0;
            size_t GramId = 0;
            size_t BlockId = 0;
            size_t FreqId = 0;
            TScheme(size_t flexId, size_t gramId, size_t blockId, size_t freqId)
                : FlexId(flexId)
                , GramId(gramId)
                , BlockId(blockId)
                , FreqId(freqId)
            {
            }
        };
        TVector<TScheme> Schemes;
        struct TBlock {
            size_t FlexId = 0;
            size_t GramId = 0;
            size_t FreqId = 0;
            bool HasNext = false;
            TBlock(size_t flexId, size_t gramId, size_t freqId, bool hasNext)
                : FlexId(flexId)
                , GramId(gramId)
                , FreqId(freqId)
                , HasNext(hasNext)
            {
            }
        };
        TVector<TBlock> Blocks;
    };

    struct TDataFreq {
        THashMap<std::pair<size_t, TUsingWideStringType>, size_t> EndsFreq;
        THashMap<TUsingWideStringType, size_t> RulesFreq;
        THashMap<size_t, size_t> SchemesFreq;
        THashMap<std::pair<size_t, TUsingWideStringType>, size_t> StemsFreq;
        THashSet<TUsingWideStringType> Blacklist;
        TSet<size_t> AllFreqValues{0};
    };

    void Normalize(IInputStream& in, IOutputStream& out, const TDictBuildParams& params);
    void MakeSchemes(IInputStream& in, IOutputStream& schemes, IOutputStream& grammar, const TDictBuildParams& params, TSet<TUsingWideStringType>* allForms = nullptr);
    void MakeDict(IInputStream& schemes, IInputStream& grammar, IOutputStream& out, const TDictBuildParams& params);

    void MakeAll(IInputStream& in, IOutputStream& out, const TDictBuildParams& params, TSet<TUsingWideStringType>* allForms = nullptr);
}
