#pragma once

#include "block.h"

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/proxim.h>
#include <kernel/qtree/richrequest/range.h>
#include <kernel/qtree/richrequest/markup/synonym.h>

namespace NReqBundle {
namespace NDetail {
    class TRichNodeUnpackHelper {
    public:
        struct TWordOptions {
            bool PreserveJoins = false;
            bool StripLemmaForms = false;

            TWordOptions()
            {}
        };

        struct TOptions {
            ui64 AllowedSynonymTypes = ~ui64(0); // TypesOf(synonym) \subset AllowedTypes
            ui32 MaxSynonymsPerRequest = ~ui32(0);
            bool UnpackSynonyms = true;
            bool UnpackWares = false;
            bool UnpackConstraints = false;
            bool UnpackAttributes = false;
            bool UnpackAndBlocks = false;
            bool FilterOffBadAttributes = false;
            bool UnpackQuotedConstraint = false;
            TWordOptions WordOptions;

            TOptions()
            {}
        };

        enum EPartType {
            PtWord,
            PtMarkup
        };

        enum ENodeParsingType {
            TopLevelNode,
            CollectSubtreeWordsForAndBlock,
            OtherParsingType,
        };

        struct TPart {
            // Common fields
            EPartType Type = PtWord;
            TBlockPtr Block;
            long RevFreq = NLingBoost::InvalidRevFreq;

            // Word fields
            bool IsMultitokenFragment = false; // True for all parts of m/t except last
            bool IsWaresObjFragment = false; // True for parts of wares objects
            TProximity ProxAfter; // Proximity constraint after this part
            EStickySide Stickiness = STICK_NONE; // Stickiness attribute for stop words

            // Markup fields
            NSearchQuery::EMarkupType MarkupType = NSearchQuery::MT_SYNONYM; // Original markup type
            size_t MarkupMask = 0x0; // Subtype mask, used for synonyms (see kernel/qtree/richrequest/thesaurus_exttype.proto)
            double MarkupWeight = 0.0;

            // Multitoken and markup fields
            size_t PartBegin = 0; // First main part referenced by markup or multitoken
            size_t PartEnd = 0; // Corresponding last, inclusive

            // Constraint fields
            EConstraintType ConstraintType = EConstraintType::ConstraintTypeMax;

            TPart() = default;

            TPart(const TPart&) = default;

            // word
            TPart(TBlockPtr block)
                : Type(PtWord)
                , Block(block)
            {}

            // markup phrase
            TPart(TBlockPtr block, NSearchQuery::EMarkupType markupType, size_t partBegin, size_t partEnd)
                : Type(PtMarkup)
                , Block(block)
                , MarkupType(markupType)
                , PartBegin(partBegin)
                , PartEnd(partEnd)
            {}
        };

        struct TQuotedConstraintPartRange {
            size_t Begin = 0;
            size_t End = 0;

            TQuotedConstraintPartRange() = default;

            TQuotedConstraintPartRange(size_t begin, size_t end)
                : Begin(begin)
                , End(end)
            {}
        };

        using TParts = TVector<TPart>;
        using TQuotedConstraintInfo = TVector<TQuotedConstraintPartRange>;

    private:
        using TIndexes = TVector<size_t>;
        using TBeginEnd = std::pair<size_t, size_t>;

    private:
        TOptions Opts;
        TParts MainParts;
        TParts MarkupParts;
        TParts ConstraintParts;
        TIndexes Indexes;
        TVector<ui64> TrIteratorMainPartsWordMasks;
        TVector<EFormClass> TrIteratorMarkupPartsBestFormClasses;
        ui32 TrIteratorWordCount = 0;
        TLangMask LangMask;
        TVector<TQuotedConstraintInfo> QuotedConstraintInfos;

    public:
        TRichNodeUnpackHelper() = default;

        TRichNodeUnpackHelper(const TOptions& opts)
            : Opts(opts)
        {
        }

        void Unpack(const TRichRequestNode& node, const TLangMask& mask = TLangMask());

        TOptions& GetOptions() {
            return Opts;
        }

        const TOptions& GetOptions() const {
            return Opts;
        }

        const TParts& GetMainParts() const {
            return MainParts;
        }

        const TParts& GetMarkupParts() const {
            return MarkupParts;
        }

        const TParts& GetConstraintParts() const {
            return ConstraintParts;
        }

        const TVector<ui64>& GetTrIteratorMainPartsWordMasks() const {
            return TrIteratorMainPartsWordMasks; // TrIteratorMainPartsWordMasks[i] == mask of words covered by MainParts[i]
        }

        const TVector<EFormClass>& GetTrIteratorMarkupPartsBestFormClasses() const {
            return TrIteratorMarkupPartsBestFormClasses; // BestFormClass from synonym's qtree node
        }

        ui32 GetTrIteratorWordCount() const {
            return TrIteratorWordCount;
        }

        const TVector<TQuotedConstraintInfo>& GetQuotedConstraintInfos() const {
            return QuotedConstraintInfos;
        }

    private:
        void ResetIndexes(size_t begin, size_t end, size_t wordIndex);
        // Retuns min i, begin <= i < end: WordIndex(i) >= wordIndex
        size_t GetPartIndexLowerBound(size_t begin, size_t end, size_t wordIndex) const;

        TBeginEnd UnpackWordOrPhrase(const TRichRequestNode& node, size_t wordIndex, ENodeParsingType parsingType);
        TBeginEnd UnpackPhrase(const TRichRequestNode& node, size_t wordIndex, ENodeParsingType parsingType);
        void UnpackQuotedWordOrPhrase(TQuotedConstraintInfo& currentQuotedConstraint, const TRichRequestNode& node);
        void UnpackSynonyms(const TRichRequestNode& node, size_t partBegin, size_t partEnd, bool isQuotedMultitoken);
        void UnpackWares(const TRichRequestNode& node, size_t partBegin, size_t partEnd);
        void UnpackConstraints(const TRichRequestNode& node);

        TPart MakeSimpleWordsOrAttributePart(const TRichRequestNode& node) const;
        // Automatically simplifies phrase to word for size = 1
        TPart MakeSimplePhrasePart(const TRichRequestNode& node, NSearchQuery::EMarkupType markupType, size_t partBegin, size_t partEnd,
            size_t markupMask = 0x0, double markupWeight = 0.0) const;

        static bool IsMultitokenContinuation(const TProximity& prox) {
            return prox.Beg == 1 && prox.End == 1 && prox.Level == 3; // && prox.DistanceType == DT_MULTITOKEN
        }
    };
} // NDetail
} // NReqBundle
