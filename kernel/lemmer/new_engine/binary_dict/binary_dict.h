#pragma once

#include <kernel/lemmer/new_engine/iface/dict_iface.h>
#include "binary_dict_header.h"

namespace NNewLemmer {
    class TBinaryGrammarStringIterator: public IGrammarStringIterator {
    public:
        TBinaryGrammarStringIterator(const TData* data, const ui32* addr = nullptr)
            : Data(data)
            , Addr(addr)
        {
        }

        bool IsValid() const override {
            return Addr != nullptr;
        }

        void Next() override {
            Y_ASSERT(IsValid());
            if (*Addr & HAS_NEXT_GRAMMAR_BIT) {
                ++Addr;
            } else {
                Addr = nullptr;
            }
        }

        TBinaryGrammarString Get() const override {
            Y_ASSERT(IsValid());
            if (Y_LIKELY(Data->Has(F_GRAMMAR))) {
                return TBinaryGrammarString((*Data)[F_GRAMMAR] + (*Addr & ~HAS_NEXT_GRAMMAR_BIT));
            } else {
                Y_ASSERT(false);
                return TBinaryGrammarString();
            }
        }

    private:
        const TData* Data = nullptr;
        const ui32* Addr = nullptr;
    };

    class TBinaryBlock: public IBlock {
    public:
        TBinaryBlock(const TData* data, size_t blockId)
            : Data(data)
            , BlockId(blockId)
        {
        }

        ui32 GetFrequency() const override {
            return Data->Has(F_BLOCKS_FREQ_ID) ? Data->GetFreq(Data->Unpack<ui16>(F_BLOCKS_FREQ_ID, BlockId)) : 0;
        }

        const wchar16* GetFormFlex() const override {
            if (Y_LIKELY(Data->Has(F_BLOCKS_FORM_FLEX))) {
                ui32 flexId = Data->Unpack<ui32>(F_BLOCKS_FORM_FLEX, BlockId) & ~HAS_NEXT_BLOCK_BIT;
                ui32 flexAddr = Data->Unpack<ui32>(F_FLEXIES_ADDRS, flexId);
                return Data->UnpackPtr<wchar16>(F_FLEXIES, flexAddr);
            } else {
                return GetEmptyFlex();
            }
        }

        IGrammarStringIteratorPtr GetGrammarStringIterator() const override {
            ui32 grammarId = 0;
            if (Y_LIKELY(Data->Has(F_BLOCKS_FLEX_GRAM))) {
                grammarId = Data->Unpack<ui16>(F_BLOCKS_FLEX_GRAM, BlockId);
            } else {
                Y_ASSERT(Data->Has(F_DEFAULT_FLEX_GRAM));
                grammarId = Data->Unpack<ui16>(F_DEFAULT_FLEX_GRAM);
            }
            const ui32* grammarAddr = Data->UnpackPtr<ui32>(F_GRAMMAR_ADDRS, grammarId);
            Y_ASSERT(grammarAddr);
            return MakeHolder<TBinaryGrammarStringIterator>(Data, grammarAddr);
        }

        bool HasNext() const override {
            if (Data->Has(F_BLOCKS_FORM_FLEX)) {
                return Data->Unpack<ui32>(F_BLOCKS_FORM_FLEX, BlockId) & HAS_NEXT_BLOCK_BIT;
            } else {
                return false;
            }
        }

    private:
        const TData* Data = nullptr;
        size_t BlockId = 0;
    };

    class TBinaryScheme: public IScheme {
    public:
        TBinaryScheme(const TData* data, size_t schemeId)
            : Data(data)
            , SchemeId(schemeId)
        {
        }

        size_t GetId() const override {
            return SchemeId;
        }

        ui32 GetFrequency() const override {
            return Data->Has(F_SCHEMES_FREQ_ID) ? Data->GetFreq(Data->Unpack<ui16>(F_SCHEMES_FREQ_ID, SchemeId)) : 0;
        }

        const wchar16* GetLemmaFlex() const override {
            if (Y_LIKELY(Data->Has(F_SCHEMES_LEMMA_FLEX))) {
                ui32 lemmaFlexId = Data->Unpack<ui32>(F_SCHEMES_LEMMA_FLEX, SchemeId);
                ui32 lemmaFlexAddr = Data->Unpack<ui32>(F_FLEXIES_ADDRS, lemmaFlexId);
                return Data->UnpackPtr<wchar16>(F_FLEXIES, lemmaFlexAddr);
            } else {
                return GetEmptyFlex();
            }
        }

        TBinaryGrammarString GetGrammarString() const override {
            if (Y_LIKELY(Data->Has(F_SCHEMES_STEM_GRAM))) {
                ui16 grammarId = Data->Unpack<ui16>(F_SCHEMES_STEM_GRAM, SchemeId);
                const ui32 grammarAddr = Data->Unpack<ui32>(F_GRAMMAR_ADDRS, grammarId) & ~HAS_NEXT_GRAMMAR_BIT;
                return TBinaryGrammarString((*Data)[F_GRAMMAR] + grammarAddr);
            } else {
                return TBinaryGrammarString();
            }
        }

        TFlexTrie GetFlexTrie() const override {
            Y_ASSERT(Data->Has(F_FLEX_TRIES));
            const ui32 flexTrieAddr = Data->Unpack<ui32>(F_FLEX_TRIES_ADDRS, SchemeId);
            const ui32 flexTrieEnd = size_t((SchemeId + 1) * BYTES[F_FLEX_TRIES_ADDRS]) < Data->Len(F_FLEX_TRIES_ADDRS)
                                         ? Data->Unpack<ui32>(F_FLEX_TRIES_ADDRS, SchemeId + 1)
                                         : Data->Len(F_FLEX_TRIES);
            return TFlexTrie((*Data)[F_FLEX_TRIES] + flexTrieAddr, flexTrieEnd - flexTrieAddr);
        }

        IBlockPtr GetBlock(size_t blockId) const override {
            Y_ASSERT(Data->Has(F_SCHEMES_BLOCK_ID));
            ui32 firstBlockId = Data->Unpack<ui32>(F_SCHEMES_BLOCK_ID, SchemeId);
            return MakeHolder<TBinaryBlock>(Data, firstBlockId + blockId);
        }

    private:
        const TData* Data = nullptr;
        size_t SchemeId = 0;
    };

    class TBinaryPattern: public IPattern {
    public:
        TBinaryPattern(const TData* data, ui32 patternId)
            : Data(data)
            , PatternId(patternId)
            , Impl(Data->Unpack<TEndInfoImpl>(F_END_INFO, patternId))
        {
        }

        ISchemePtr GetScheme() const override {
            return MakeHolder<TBinaryScheme>(Data, Impl.SchemeId);
        }

        ui8 GetStemLen() const override {
            return Impl.StemLen;
        }

        ui8 GetOldFlexLen() const override {
            return Impl.OldFlexLen;
        }

        bool HasPrefix() const override {
            return Impl.HasPrefix;
        }

        bool IsFinal() const override {
            return Impl.IsFinal;
        }

        ui8 GetRest() const override {
            return Impl.Rest;
        }

        bool IsUseAlways() const override {
            return Impl.UseAlways;
        }

        ui32 GetFrequency() const override {
            return Data->Has(F_END_FREQ_ID) ? Data->GetFreq(Data->Unpack<ui16>(F_END_FREQ_ID, PatternId)) : 0;
        }

        TDiaMask GetDiaMask() const override {
            return Data->Has(F_END_DIA_MASK) ? Data->Unpack<ui16>(F_END_DIA_MASK, PatternId) : 0;
        }

        TFlexTrie GetFlexTrie() const override {
            return GetScheme()->GetFlexTrie();
        }

        bool HasFlexTrie() const override {
            return Data->Has(F_FLEX_TRIES);
        }

    private:
        const TData* Data = nullptr;
        ui32 PatternId = 0;
        TEndInfoImpl Impl;
    };

    class TBinaryPatternIterator: public IPatternIterator {
    public:
        TBinaryPatternIterator(const TData* data, const ui32* addr = nullptr)
            : Data(data)
            , Addr(addr)
        {
        }

        bool IsValid() const override {
            return Addr != nullptr;
        }

        void Next() override {
            Y_ASSERT(IsValid());
            if (*Addr & HAS_NEXT_ANA_BIT) {
                ++Addr;
            } else {
                Addr = nullptr;
            }
        }

        IPatternPtr Get() const override {
            Y_ASSERT(IsValid());
            ui32 patternId = *Addr & ~HAS_NEXT_ANA_BIT;
            return MakeHolder<TBinaryPattern>(Data, patternId);
        }

    private:
        const TData* Data = nullptr;
        const ui32* Addr = nullptr;
    };

    class TBinaryDictData: public IDictData {
    public:
        TBinaryDictData(const char* data, size_t size)
            : Data(data, size)
        {
            PatternsTrie = TPatternsTrie(Data[F_ENDS_TRIE], Data.Len(F_ENDS_TRIE));
        }

        const TData& GetTData() const {
            return Data;
        }

        TPatternsTrie GetPatternsTrie() const {
            return PatternsTrie;
        }

        TPatternMatchResult GetMatchedPatterns(TWtringBuf revWord) const override {
            ui32 patternId;
            size_t endLen = 0;
            TPatternMatchResult matchResult;
            if (PatternsTrie.FindLongestPrefix(revWord, &endLen, &patternId)) {
                matchResult.PatternIterator = MakeHolder<TBinaryPatternIterator>(&Data, Data.UnpackPtr<ui32>(F_ANAS_LISTS, patternId));
                matchResult.MatchedLength = endLen;
                return matchResult;
            } else {
                matchResult.PatternIterator = MakeHolder<TBinaryPatternIterator>(&Data);
                return matchResult;
            }
        }

        ISchemePtr GetScheme(size_t schemeId) const override {
            return MakeHolder<TBinaryScheme>(&Data, schemeId);
        }

        TString GetFingerprint() const override {
            return Data.Fingerprint();
        }

    private:
        TData Data;
        TPatternsTrie PatternsTrie;
    };
}
