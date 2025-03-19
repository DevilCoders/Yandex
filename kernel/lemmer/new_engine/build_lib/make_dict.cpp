#include "common.h"
#include "dict_builder.h"

#include <kernel/lemmer/alpha/abc.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <kernel/lemmer/new_engine/binary_dict/binary_dict_header.h>

#include <util/stream/str.h>
#include <util/string/builder.h>
#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/string/split.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/string/subst.h>

using namespace NDictBuild;
namespace {
    const static THashSet<TString> GlobalProductivePos = {"A", "ANUM", "S", "V", "ADV"};
    const static THashSet<TString> GlobalShortProductivePos = {"S"};
    const static THashSet<TString> GlobalBadBastardGrammar = {"irreg", "rare", "awkw", "abbr"};

    template <class T>
    bool IntersectsSet(const TVector<T>& items, const THashSet<T>& set) {
        for (const auto& s : items) {
            if (set.contains(s)) {
                return true;
            }
        }
        return false;
    }

    template <class T>
    size_t GetOrElse(const THashMap<T, size_t>& h, const T& key, size_t def = 0) {
        const auto& it = h.find(key);
        if (it != h.end()) {
            return it->second;
        }
        return def;
    }

    TUsingWideStringType ReverseString(const TUsingWideStringType& s) {
        auto vec = TVector<TUsingWideStringType::value_type>(s.cbegin(), s.cend());
        Reverse(vec.begin(), vec.end());
        TUsingWideStringType result(vec.begin(), vec.end());
        return result;
    }

    struct TBlockOpts {
        bool IsFio = false;
        ELanguage Lang = LANG_UNK;
    };

    struct TGrammarInfo {
        THashSet<size_t> ProductiveGramms, ShortProductiveGramms, BadBastardGrammar;
        TVector<TString> AllGrammars;
        TGrammarInfo(IInputStream& grammar) {
            size_t i = 0;
            TString line;
            while (grammar.ReadLine(line)) {
                ++i;
                TVector<TString> g;
                line = line.substr(line.find('\t') + 1, line.size());
                AllGrammars.push_back(line);
                StringSplitter(line).Split(',').SkipEmpty().Collect(&g);
                if (IntersectsSet(g, GlobalProductivePos)) {
                    ProductiveGramms.insert(i);
                }
                if (IntersectsSet(g, GlobalShortProductivePos)) {
                    ShortProductiveGramms.insert(i);
                }
                if (IntersectsSet(g, GlobalBadBastardGrammar)) {
                    BadBastardGrammar.insert(i);
                }
            }
        }
    };

    bool BastardAllowedForStem(const TUsingWideStringType& stem, size_t stemGram, const TGrammarInfo& grammarInfo) {
        return (stem.empty() || stem[0] != '~') && grammarInfo.ProductiveGramms.contains(stemGram) && !grammarInfo.BadBastardGrammar.contains(stemGram);
    }
    bool BastardAllowedForFlex(TVector<size_t> flexGrams, const TGrammarInfo& grammarInfo) {
        return !IntersectsSet(flexGrams, grammarInfo.BadBastardGrammar);
    }

    size_t MinBastardLen(size_t grammar, const TGrammarInfo& info) {
        if (info.ShortProductiveGramms.contains(grammar)) {
            return 4;
        }
        return 6;
    }

    struct TProcessedEnd {
        TUsingWideStringTypeBuf Key;
        uint32_t StemFreq = 0;
        uint32_t Mask = 0;
        uint32_t TotalFreq = 0;

        NNewLemmer::TEndInfoImpl EndInfo;
        TProcessedEnd(TUsingWideStringTypeBuf key, size_t sch, size_t stemFreq, size_t stemLen, size_t rest, size_t oldFlexLen, bool hasPrefix, bool isFinal, size_t mask, size_t totalFreq)
            : Key(key)
            , StemFreq(stemFreq)
            , Mask(mask)
            , TotalFreq(totalFreq)
            , EndInfo(sch, stemLen, hasPrefix, isFinal, rest, oldFlexLen)
        {
        }

        TProcessedEnd() = default;

        bool operator<(const TProcessedEnd& other) const {
            return std::tie(Key, EndInfo.SchemeId, StemFreq, EndInfo.StemLen, EndInfo.Rest, EndInfo.OldFlexLen, EndInfo.HasPrefix, EndInfo.IsFinal, Mask, TotalFreq) < std::tie(other.Key, other.EndInfo.SchemeId, other.StemFreq, other.EndInfo.StemLen, other.EndInfo.Rest, other.EndInfo.OldFlexLen, other.EndInfo.HasPrefix, other.EndInfo.IsFinal, other.Mask, other.TotalFreq);
        }

        bool EqualPacked(const TProcessedEnd& other) const {
            return std::tie(EndInfo.SchemeId, StemFreq, EndInfo.StemLen, EndInfo.Rest, EndInfo.OldFlexLen, EndInfo.HasPrefix, EndInfo.IsFinal, TotalFreq) == std::tie(other.EndInfo.SchemeId, other.StemFreq, other.EndInfo.StemLen, other.EndInfo.Rest, other.EndInfo.OldFlexLen, other.EndInfo.HasPrefix, other.EndInfo.IsFinal, other.TotalFreq);
        }

        bool operator==(const TProcessedEnd& other) const {
            return std::tie(Key, EndInfo.SchemeId, StemFreq, EndInfo.StemLen, EndInfo.Rest, EndInfo.OldFlexLen, EndInfo.HasPrefix, EndInfo.IsFinal, Mask, TotalFreq) == std::tie(other.Key, other.EndInfo.SchemeId, other.StemFreq, other.EndInfo.StemLen, other.EndInfo.Rest, other.EndInfo.OldFlexLen, other.EndInfo.HasPrefix, other.EndInfo.IsFinal, other.Mask, other.TotalFreq);
        }

        NNewLemmer::TEndInfoImpl MegaPack() const {
            return EndInfo;
        }
    };

    std::tuple<TUsingWideStringType, TUsingWideStringType, bool> SplitForPrefix(const TUsingWideStringType& s) {
        size_t tildePos = s.find('~');
        bool hasPrefix = tildePos != TUsingWideStringType::npos;
        TUsingWideStringType prefix;
        TUsingWideStringType end = s;
        if (hasPrefix) {
            prefix = s.substr(0, tildePos);
            end = s.substr(tildePos + 1, end.size());
        }
        return std::tie(prefix, end, hasPrefix);
    }

    TVector<TProcessedEnd> ProcessEndWithStem(TUsingWideStringType stem, TUsingWideStringType end, size_t sch, const TDataFreq& dataFreq, const TBlockOpts& blockOpts, bool bastardAllowed, size_t minLen, THashSet<TUsingWideStringType>& strings) {
        bool hasPrefix;
        TUsingWideStringType prefix;
        std::tie(prefix, end, hasPrefix) = SplitForPrefix(end);
        size_t endFreq = GetOrElse(dataFreq.EndsFreq, std::make_pair(sch, end));
        size_t schemeFreq = GetOrElse(dataFreq.SchemesFreq, sch) + 1;
        SubstGlobal(stem, TParseConstants::Tilde, TParseConstants::Empty); //WTF? - copypaste from python
        //jmax is the upper bound for start index of bastard sample
        size_t jmax = 0;
        bool hasStar = stem.find('*') != TUsingWideStringType::npos; // like _*xxx
        if (hasStar || blockOpts.IsFio) {
            jmax = stem.size();
        } else if (bastardAllowed && stem.size() > 2) {
            jmax = stem.size() - 2;
        }
        Y_ASSERT(jmax <= stem.size());
        TVector<TProcessedEnd> result;
        const TUsingWideStringType& wordReversed = *strings.insert(ReverseString(stem + end)).first;
        for (size_t j = 0; j <= jmax; ++j) {
            if (hasStar && j < 2) { // skip two starting symbols for samples like _*xxx
                continue;
            }
            TUsingWideStringType subStem = stem.substr(j, stem.size());
            TUsingWideStringType key = subStem + end;
            size_t stemLen = subStem.size();
            size_t rest = 0;
            if (minLen > key.size()) {
                rest = minLen - key.size();
            }
            if (rest + stem.size() - j < 4) {
                rest = 4 - stem.size() + j;
            }
            size_t stemFreq = GetOrElse(dataFreq.StemsFreq, std::make_pair(sch, subStem));
            int totalFreq = float(stemFreq) * endFreq / schemeFreq * 100;
            bool isFinal = j == 0;
            Y_ASSERT(!key.empty());
            bool keyChanged = false;
            if (isFinal && hasPrefix && key[0] == '_') {
                key = TParseConstants::Underscore + prefix + key.substr(1, key.size());
                keyChanged = true;
                stemLen += prefix.size();
            }
            size_t mask = 0; //todo: dia_mask
            TUsingWideStringType genKey = ReverseString(key);
            if (!isFinal && dataFreq.Blacklist.contains(genKey)) { // Really? Why do we check gen_key instead of key?
                continue;
            }
            size_t stemMask = 0; //todo: dia_mask
            if (mask != 0) {
                mask = (stemMask * 2) | 1;
            }
            if (mask != 0 && isFinal) { // in fact, there is no generalization for bastards
                throw yexception() << "diacritics was not tested, dangerous!";
            }
            TProcessedEnd curEnd;
            if (!keyChanged) {
                Y_ASSERT(wordReversed.StartsWith(genKey));
                curEnd.Key = TUsingWideStringTypeBuf(wordReversed.c_str(), genKey.size());
            } else {
                curEnd.Key = *strings.insert(genKey).first;
            }
            curEnd.StemFreq = stemFreq;
            curEnd.EndInfo = NNewLemmer::TEndInfoImpl(sch, stemLen, hasPrefix, isFinal, rest, 0);
            curEnd.Mask = 0;
            curEnd.TotalFreq = totalFreq;
            result.emplace_back(std::move(curEnd));
        }
        return result;
    }

    void GatherSchemeStats(const NDictBuild::TFullScheme& scheme, const TDataFreq& dataFreq, size_t sch, TGramIds& globalGramIds, TDataStatistics& stats) {
        TUsingWideStringType lexEnd = scheme.LexEnd;
        SubstGlobal(lexEnd, TParseConstants::Tilde, TParseConstants::DollarSign);
        if (!stats.Flexies.contains(lexEnd)) {
            stats.Flexies[lexEnd] = stats.Flexies.size();
        }
        size_t lexGram = globalGramIds.PrepareGramIds({scheme.LexGram});
        stats.Schemes.emplace_back(stats.Flexies[lexEnd], lexGram, stats.Blocks.size(), GetOrElse(dataFreq.SchemesFreq, sch, 0));
        size_t i = 0;
        for (const auto& end : scheme.EndsMap) {
            TUsingWideStringType flex;
            size_t tildePos = end.first.find(TParseConstants::Tilde);
            if (tildePos != TUsingWideStringType::npos) {
                flex = end.first.substr(tildePos + 1) + TParseConstants::DollarSign + end.first.substr(0, tildePos);
            } else {
                flex = end.first;
            }
            auto gramIds = globalGramIds.PrepareGramIds(end.second);
            if (!stats.Flexies.contains(flex)) {
                stats.Flexies[flex] = stats.Flexies.size();
            }
            stats.Blocks.emplace_back(stats.Flexies[flex], gramIds, GetOrElse(dataFreq.EndsFreq, std::make_pair(sch, flex), 0), i + 1 < scheme.EndsMap.size() ? 1 : 0);
            ++i;
        }
    }

    struct TPackedEndStruct {
        TUsingWideStringTypeBuf Key;
        //TMaybeOwnString Key;
        TVector<TSinglePackedEnd> Ends;
        TPackedEndStruct(const TUsingWideStringTypeBuf& key, const TVector<TSinglePackedEnd>& ends)
            : Key(key)
            , Ends(ends)
        {
        }
        bool IsSimilar(const TPackedEndStruct& other) const {
            //shouldn't happen at all
            if (Y_UNLIKELY(Key.size() + 1 != other.Key.size())) {
                return false;
            }
            if (other.Key.substr(0, Key.size()) != Key) {
                return false;
            }
            if (Ends.size() != other.Ends.size()) {
                return false;
            }
            for (size_t i = 0; i < Ends.size(); ++i) {
                if (!Ends[i].IsSimilar(other.Ends[i])) {
                    return false;
                }
            }
            return true;
        }
    };

    using TProcessedEndIterator = TSet<TProcessedEnd>::const_iterator;
    TVector<TSinglePackedEnd> PackSingleKeyEnds(TProcessedEndIterator itBegin, TProcessedEndIterator itEnd) {
        TVector<TSinglePackedEnd> result;
        bool prevIsFinal = false;
        const TProcessedEnd* oldFields = nullptr;
        for (; itBegin != itEnd; ++itBegin) {
            const auto& end = *itBegin;
            if (oldFields && end.EqualPacked(*oldFields)) {
                continue;
            }
            if (!end.EndInfo.IsFinal && prevIsFinal) {
                continue;
            }

            TSinglePackedEnd newVal(end.StemFreq, end.Mask, end.MegaPack(), end.TotalFreq);
            if (end.EndInfo.IsFinal && !prevIsFinal) {
                result.clear();
                prevIsFinal = true;
            }
            result.emplace_back(newVal);
            oldFields = &end;
        }
        Sort(result.begin(), result.end());
        return result;
    }

    void InsertPackedEndIfNecessary(TPackedEndStruct end, THashMap<size_t, TPackedEndStruct>& prev, TVector<TPackedEndStruct>& result) {
        {
            auto it = prev.emplace(end.Key.size(), end);
            if (!it.second) { //element already in hash
                it.first->second = end;
            }
        }
        if (prev.contains(end.Key.size() - 1)) {
            if (!end.Key.empty() && prev.contains(end.Key.size() - 1) && prev.at(end.Key.size() - 1).IsSimilar(end)) {
                return;
            }
        }
        //we will not need TotalFreq anymore, but we will need to compare ends ignoring it
        for (auto& singleEnd : end.Ends) {
            singleEnd.TotalFreq = 0;
        }
        result.emplace_back(end);
    }

    TVector<TPackedEndStruct> PackAllEnds(const TSet<TProcessedEnd>& ends) {
        TProcessedEndIterator b = ends.begin(), e = ends.begin();
        ++e;
        TVector<TPackedEndStruct> result;
        //from EliminateEnds
        THashMap<size_t, TPackedEndStruct> prev;
        //poor man groupby
        while (e != ends.end()) {
            if (e->Key != b->Key) {
                const auto& packed = PackSingleKeyEnds(b, e);
                TPackedEndStruct str(b->Key, packed);
                if (!b->Key.empty() && !packed.empty()) {
                    //result.emplace_back(b->Key, packed);
                    InsertPackedEndIfNecessary(TPackedEndStruct(b->Key, packed), prev, result);
                }
                b = e;
            }
            ++e;
        }
        const auto& packed = PackSingleKeyEnds(b, e);
        if (packed.size() && b->Key.size()) {
            if (!b->Key.empty() && !packed.empty()) {
                InsertPackedEndIfNecessary(TPackedEndStruct(b->Key, packed), prev, result);
            }
        }
        return result;
    }

    struct TSplittedEndsData {
        TVector<TSinglePackedEnd> UniqueEnds;
        TVector<std::pair<TUsingWideStringTypeBuf, size_t>> EndsKeysPopularity;
        TVector<TVector<size_t>> AnasLists;
        TSplittedEndsData(const TVector<TPackedEndStruct>& packedEnds) {
            //split.py
            TMap<TVector<TSinglePackedEnd>, size_t> packedEndsPopularityMap;
            for (const auto& end : packedEnds) {
                ++packedEndsPopularityMap[end.Ends];
            }

            class TPackedEndsVectorWithPopularity {
            private:
                //python compatibility
                TString EndsString;

            public:
                TVector<TSinglePackedEnd> Ends;
                size_t Popularity;
                TPackedEndsVectorWithPopularity(const std::pair<TVector<TSinglePackedEnd>, size_t>& p)
                    : Ends(p.first)
                    , Popularity(p.second)
                {
                    TStringBuilder builder;
                    for (size_t i = 0; i < Ends.size(); ++i) {
                        if (i) {
                            builder << ' ';
                        }
                        builder << Ends[i].EndInfo.SchemeId << '/' << Ends[i].Freq << '/' << Ends[i].Mask << '/' << (Ends[i].EndInfo.Pack() >> 16);
                    }
                    EndsString = builder;
                }
                bool operator<(const TPackedEndsVectorWithPopularity& other) const {
                    return std::tie(other.Popularity, EndsString) < std::tie(Popularity, other.EndsString);
                }
            };
            TVector<TPackedEndsVectorWithPopularity> endsPopularity(packedEndsPopularityMap.begin(), packedEndsPopularityMap.end());
            Sort(endsPopularity.begin(), endsPopularity.end());
            TMap<TVector<TSinglePackedEnd>, size_t> endVectorPopularityIndices;
            {
                size_t i = 0;
                for (const auto& ends : endsPopularity) {
                    endVectorPopularityIndices[ends.Ends] = i;
                    i += ends.Ends.size();
                }
            }

            TMap<TSinglePackedEnd, size_t> anasList;
            for (const auto& ends : endsPopularity) {
                AnasLists.emplace_back(0);
                for (const auto& end : ends.Ends) {
                    if (!anasList.contains(end)) {
                        UniqueEnds.push_back(end);
                        anasList[end] = anasList.size();
                    }
                    AnasLists.back().push_back(anasList[end]);
                }
            }

            EndsKeysPopularity.reserve(packedEnds.size());
            for (const auto& end : packedEnds) {
                EndsKeysPopularity.emplace_back(end.Key, endVectorPopularityIndices[end.Ends]);
            }
        }
    };

    using TSingleSchemeNumeratedEnds = TVector<std::pair<TUsingWideStringType, size_t>>;
    TSingleSchemeNumeratedEnds ExtractSchemeEnumeratedEnds(const NDictBuild::TFullScheme& scheme, ELanguage lang) {
        TSingleSchemeNumeratedEnds result;
        size_t i = 0;
        const NLemmer::TAlphabetWordNormalizer* normalizer = nullptr;
        if (lang == LANG_TUR) { //python scripts used generatlization only for tr
            normalizer = NLemmer::GetAlphaRules(lang);
        }
        for (const auto& end : scheme.EndsMap) {
            TUsingWideStringType key = end.first;
            size_t tildePos = key.find(TParseConstants::Tilde);
            if (tildePos != TUsingWideStringType::npos) {
                key = key.substr(tildePos + 1) + TParseConstants::DollarSign + key.substr(0, tildePos);
            }
            result.emplace_back(key, i);
            TUsingWideStringType generalized = key;
            //@todo: it it possible to have different ends generalizing to the same?
            //why we check generalized against ends instead of reversed ends?
            if (normalizer && normalizer->GetDiacriticsMap()->Generalize(key.begin(), key.size(), generalized.begin()) && !scheme.EndsMap.contains(generalized)) {
                result.emplace_back(generalized, i);
            }
            ++i;
        }
        return result;
    }

    TVector<TSingleSchemeNumeratedEnds> ExtractEnds(const TVector<NDictBuild::TFullScheme>& schemes, ELanguage lang) {
        TVector<TSingleSchemeNumeratedEnds> result;
        for (const auto& scheme : schemes) {
            result.push_back(ExtractSchemeEnumeratedEnds(scheme, lang));
        }
        return result;
    }

    TString SchemeEndsToTrie(const TSingleSchemeNumeratedEnds& ends) {
        TCompactTrieBuilder<TUsingWideStringType::char_type, ui32, TCompactTriePacker<ui32>> builder;
        for (const auto& end : ends) {
            builder.Add(end.first.data(), end.first.size(), end.second);
        }
        TBufferOutput raw;
        builder.SaveAndDestroy(raw);
        TBufferOutput result;
        CompactTrieMinimize(result, raw.Buffer().Data(), raw.Buffer().Size(), false, TCompactTriePacker<ui32>());
        TString s;
        result.Buffer().AsString(s);
        return s;
    }

    TString EndsPopulartyToTrie(TVector<std::pair<TUsingWideStringTypeBuf, size_t>>& ends) {
        TCompactTrieBuilder<TUsingWideStringType::char_type, ui32, TCompactTriePacker<ui32>> builder(CTBF_PREFIX_GROUPED);
        for (const auto& end : ends) {
            builder.Add(end.first.data(), end.first.size(), end.second);
        }
        TBufferOutput raw;
        builder.SaveAndDestroy(raw);
        TBufferOutput result;
        CompactTrieMinimize(result, raw.Buffer().Data(), raw.Buffer().Size(), false, TCompactTriePacker<ui32>());
        TString s;
        result.Buffer().AsString(s);
        return s;
    }

}

namespace NDictBuild {
    //@todo: fioMode
    //@todo: freqs
    void MakeDict(IInputStream& schemesFile, IInputStream& grammarFile, IOutputStream& out, const TDictBuildParams& params) {
        TDataFreq dataFreq;
        TBlockOpts blockOpts;
        blockOpts.IsFio = params.FioMode;
        blockOpts.Lang = params.Language;
        TGrammarInfo grammarInfo(grammarFile);
        TGramIds gramIds;
        TDataStatistics stats;
        size_t sch = 0;
        TSet<TProcessedEnd> processedEnds;
        TVector<TFullScheme> schemes;
        size_t totalKeys = 0;
        THashSet<TUsingWideStringType> reservedStrings;
        while (true) {
            TFullScheme scheme;
            try {
                scheme.Load(&schemesFile);
            } catch (TLoadEOF&) {
                break;
            }
            bool isFrequent = sch < params.BastardFreq;
            for (const auto& end : scheme.EndsMap) {
                bool baFlex = BastardAllowedForFlex(end.second, grammarInfo);
                for (const auto& stem : scheme.Stems) {
                    bool baStem = BastardAllowedForStem(stem, scheme.LexGram, grammarInfo);
                    //reversedStrings.emplace_back(ReverseString(stem + end.first));
                    const auto& p = ProcessEndWithStem(stem, end.first, sch, dataFreq, blockOpts, isFrequent && baFlex && baStem, MinBastardLen(scheme.LexGram, grammarInfo), reservedStrings);
                    for (const auto& x : p) {
                        totalKeys += x.Key.size();
                    }
                    processedEnds.insert(p.begin(), p.end());
                }
            }
            GatherSchemeStats(scheme, dataFreq, sch, gramIds, stats);
            ++sch;
            schemes.emplace_back(std::move(scheme));
        }
        if (params.Verbose) {
            Cerr << "schemes read" << Endl;
        }
        const TVector<TSingleSchemeNumeratedEnds>& extractedEnds = ExtractEnds(schemes, params.Language);
        TVector<TPackedEndStruct> packedEnds = PackAllEnds(processedEnds);
        TSplittedEndsData splittedEndsData(packedEnds);
        if (params.Verbose) {
            Cerr << "creating tries... ";
        }
        TString endsPopularityTrie = EndsPopulartyToTrie(splittedEndsData.EndsKeysPopularity);
        TVector<TString> schemeEndsTries;
        schemeEndsTries.reserve(splittedEndsData.EndsKeysPopularity.size());
        for (const auto& end : extractedEnds) {
            schemeEndsTries.push_back(SchemeEndsToTrie(end));
        }
        if (params.Verbose) {
            Cerr << "ok" << Endl;
        }

        if (params.MakeProtoDict) {
            TNewProtoDictBuilder builder(
                grammarInfo.AllGrammars,
                gramIds,
                stats,
                schemeEndsTries,
                endsPopularityTrie,
                splittedEndsData.UniqueEnds,
                splittedEndsData.AnasLists,
                dataFreq,
                params);
            builder.Save(out);
        } else {
            TNewBinaryDictBuilder builder(
                grammarInfo.AllGrammars,
                gramIds,
                stats,
                schemeEndsTries,
                endsPopularityTrie,
                splittedEndsData.UniqueEnds,
                splittedEndsData.AnasLists,
                dataFreq,
                params);
            builder.Save(out);
        }
    }
}
