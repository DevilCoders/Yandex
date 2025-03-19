#pragma once

#include "kernel/lemmer/new_engine/iface/dict_iface.h"
#include "kernel/lemmer/new_engine/build_lib/protos/dict.pb.h"
#include "util/stream/output.h"
#include <kernel/lemmer/dictlib/grammar_index.h>

namespace NNewLemmer {
    class TProtoGrammarStringIterator : public IGrammarStringIterator {
    public:
        TProtoGrammarStringIterator(const NLemmerProto::TLemmerDict* data, size_t blockId)
            : Data(data)
        {
            Y_ASSERT(Data);
            Y_ASSERT(Data->GrammarsSize() > 0);
            Y_ASSERT(blockId < Data->BlocksSize());
            Block = &Data->GetBlocks(blockId);
            if (Block->HasGrammarRefIndex()) {
                GrammarRefIndex = Block->GetGrammarRefIndex();
            } else {
                Y_ENSURE(Data->HasDefaultGrammarRefIndex());
                GrammarRefIndex = Data->GetDefaultGrammarRefIndex();
            }
        }

        bool IsValid() const override {
            return GrammarRefIndex < Max<size_t>();
        }

        void Next() override {
            Y_ASSERT(IsValid());
            Y_ASSERT(GrammarRefIndex < Data->GrammarRefsSize());
            if (Data->GetGrammarRefs(GrammarRefIndex).GetHasNext()) {
                GrammarRefIndex++;
                Y_ENSURE(GrammarRefIndex < Data->GrammarRefsSize());
            } else {
                GrammarRefIndex = Max<size_t>();
            }
        }

        TBinaryGrammarString Get() const override {
            Y_ASSERT(IsValid());
            Y_ASSERT(GrammarRefIndex < Data->GrammarRefsSize());
            const ui32 grammarId = Data->GetGrammarRefs(GrammarRefIndex).GetId();
            Y_ENSURE(grammarId < Data->GrammarsSize());
            return TBinaryGrammarString(Data->GetGrammars(grammarId).data());
        }
    private:
        const NLemmerProto::TLemmerDict* Data = nullptr;
        const NLemmerProto::TBlock* Block = nullptr;
        size_t GrammarRefIndex = Max<size_t>();
    };

    class TProtoBlock : public IBlock {
    public:
        TProtoBlock(const NLemmerProto::TLemmerDict* data, size_t blockId)
            : Data(data)
            , BlockId(blockId)
        {
            Y_ASSERT(Data);
            Y_ENSURE(BlockId < Data->BlocksSize());
            Block = &Data->GetBlocks(BlockId);
        }

        ui32 GetFrequency() const override {
            return Block->GetFrequency();
        }

        const wchar16* GetFormFlex() const override;

        IGrammarStringIteratorPtr GetGrammarStringIterator() const override {
            return MakeHolder<TProtoGrammarStringIterator>(Data, BlockId);
        }

        bool HasNext() const override {
            if (Block->HasHasNext()) {
                return Block->GetHasNext();
            } else {
                return false;
            }
        }
    private:
        const NLemmerProto::TLemmerDict* Data = nullptr;
        const NLemmerProto::TBlock* Block = nullptr;
        size_t BlockId = 0;
    };

    class TProtoScheme : public IScheme {
    public:
        TProtoScheme(const NLemmerProto::TLemmerDict* data, size_t schemeId)
            : Data(data)
            , SchemeId(schemeId)
        {
            Y_ASSERT(Data);
            Y_ENSURE(SchemeId < Data->SchemesSize());
            Scheme = &Data->GetSchemes(SchemeId);
        }

        size_t GetId() const override {
            return SchemeId;
        }

        ui32 GetFrequency() const override {
            return Scheme->GetFrequency();
        }

        const wchar16* GetLemmaFlex() const override;

        TBinaryGrammarString GetGrammarString() const override {
            const ui32 grammarId = Scheme->GetGrammarId();
            Y_ENSURE(grammarId < Data->GrammarsSize());
            return TBinaryGrammarString(Data->GetGrammars(grammarId).data());
        }

        TFlexTrie GetFlexTrie() const override {
            Y_ENSURE(Data->FlexTriesSize() > SchemeId);
            return TFlexTrie(Data->GetFlexTries(SchemeId).GetBinary().data(), Data->GetFlexTries(SchemeId).GetBinary().size());
        }

        IBlockPtr GetBlock(size_t schemeBlockIndex) const override {
            Y_ENSURE(Scheme->HasBlockId());
            return MakeHolder<TProtoBlock>(Data, Scheme->GetBlockId() + schemeBlockIndex);
        }
    private:
        const NLemmerProto::TLemmerDict* Data = nullptr;
        const NLemmerProto::TScheme* Scheme = nullptr;
        size_t SchemeId = 0;
    };

    class TProtoPattern : public IPattern {
    public:
        TProtoPattern(const NLemmerProto::TLemmerDict* data, ui32 patternId)
            : Data(data)
            , PatternId(patternId)
        {
        }

        ISchemePtr GetScheme() const override {
            ui32 schemeId = SelectBits<0, 16>(Data->GetPatterns(PatternId).GetBitmask());
            return MakeHolder<TProtoScheme>(Data, schemeId);
        }

        ui8 GetStemLen() const override {
            return SelectBits<16, 6>(Data->GetPatterns(PatternId).GetBitmask());
        }

        ui8 GetOldFlexLen() const override {
            return SelectBits<31, 3>(Data->GetPatterns(PatternId).GetBitmask());
        }

        bool HasPrefix() const override {
            return SelectBits<22, 1>(Data->GetPatterns(PatternId).GetBitmask());
        }

        bool IsFinal() const override {
            return SelectBits<23, 1>(Data->GetPatterns(PatternId).GetBitmask());
        }

        ui8 GetRest() const override {
            return SelectBits<24, 3>(Data->GetPatterns(PatternId).GetBitmask());
        }

        bool IsUseAlways() const override {
            return SelectBits<28, 1>(Data->GetPatterns(PatternId).GetBitmask());
        }

        ui32 GetFrequency() const override {
            return Data->GetPatterns(PatternId).GetFrequency();
        }

        TDiaMask GetDiaMask() const override {
            return Data->GetPatterns(PatternId).GetDiaMask();
        }

        TFlexTrie GetFlexTrie() const override {
            return GetScheme()->GetFlexTrie();
        }

        bool HasFlexTrie() const override {
            return Data->FlexTriesSize() > 0;
        }
    private:
        const NLemmerProto::TLemmerDict* Data = nullptr;
        ui32 PatternId = 0;
    };

    class TProtoPatternIterator : public IPatternIterator {
    public:
        TProtoPatternIterator(const NLemmerProto::TLemmerDict* data, ui32 patternId)
            : Data(data)
            , PatternId(patternId)
        {
            Y_ASSERT(Data);
            Y_ENSURE(PatternId < Data->MatchedPatternsSize());
        }

        TProtoPatternIterator()
            : PatternId(Max<ui32>())
        {
        }

        bool IsValid() const override {
            return PatternId < Max<ui32>();
        }

        void Next() override {
            Y_ASSERT(IsValid());
            Y_ASSERT(PatternId < Data->MatchedPatternsSize());
            if (Data->GetMatchedPatterns(PatternId).GetHasNext()) {
                PatternId++;
                Y_ENSURE(PatternId < Data->MatchedPatternsSize());
            } else {
                PatternId = Max<ui32>();
            }
        }

        IPatternPtr Get() const override {
            Y_ASSERT(IsValid());
            Y_ASSERT(PatternId < Data->MatchedPatternsSize());
            ui32 resPatternId = Data->GetMatchedPatterns(PatternId).GetId();
            return MakeHolder<TProtoPattern>(Data, resPatternId);
        }
    private:
        const NLemmerProto::TLemmerDict* Data = nullptr;
        ui32 PatternId = 0;
    };

    class TProtoDictData : public IDictData {
    public:
        TProtoDictData(const TBlob& blob)
        {
            Y_PROTOBUF_SUPPRESS_NODISCARD Data.ParseFromArray(blob.AsCharPtr(), blob.Length());
            PatternsTrie = TPatternsTrie(Data.GetPatternsTrie().GetBinary().data(), Data.GetPatternsTrie().GetBinary().size());
            Y_ENSURE(Data.GrammarsSize() > 0);
        }

        TPatternsTrie GetPatternsTrie() const {
            return PatternsTrie;
        }

        TPatternMatchResult GetMatchedPatterns(TWtringBuf revWord) const override {
            ui32 patternId = 0;
            size_t endLen = 0;
            TPatternMatchResult matchResult;
            if (PatternsTrie.FindLongestPrefix(revWord, &endLen, &patternId)) {
                matchResult.PatternIterator = MakeHolder<TProtoPatternIterator>(&Data, patternId);
                matchResult.MatchedLength = endLen;
                return matchResult;
            } else {
                matchResult.PatternIterator = MakeHolder<TProtoPatternIterator>();
                return matchResult;
            }
        }

        ISchemePtr GetScheme(size_t schemeId) const override {
            return MakeHolder<TProtoScheme>(&Data, schemeId);
        }

        TString GetFingerprint() const override {
            return Data.GetFingerprint();
        }
    private:
        NLemmerProto::TLemmerDict Data;
        TPatternsTrie PatternsTrie;
    };
}


