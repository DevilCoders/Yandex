#pragma once

#include "reqbundle_accessors.h"

#include <util/stream/output.h>

namespace NReqBundle {
    enum EPrintFlags {
        PF_SYNONYMS    = 0x001,
        PF_WORD_FREQ   = 0x002,
        PF_MATCH_FREQ  = 0x004,
        PF_BLOCK_FREQ  = 0x008,
        PF_WORD_IDX    = 0x010,
        PF_BLOCK_IDX   = 0x020,
        PF_REQUEST_IDX = 0x040,
        PF_FACET_NAME  = 0x080,
        PF_FACET_VALUE = 0x100,
        PF_CUSTOM_SEP  = 0x200
    };

    struct TPrintableSeparators {
        const char* RequestSeparator = "\n";
        const char* RequestIndexSeparator = ") ";
        const char* MatchSeparator = " ";
        const char* WordIndexSeparator = "-";
        const char* WordIndexLeftMargin = "[";
        const char* WordIndexRightMargin = "]";
        const char* WordIndexBlockSeparator = ":";
        const char* BlockIndexSeparator = "=";
        const char* WordSeparator = " ";
        const char* TextLeftMargin = "\"";
        const char* TextRightMargin = "\"";
        const char* MatchFreqSeparator = "::";
        const char* FreqSeparator = "/";
        const char* SynonymsSeparator = " ^ ";
        const char* RequestFacetSeparator = " ";
        const char* FacetSeparator = " ";
        const char* FacetLeftMargin = "{";
        const char* FacetRightMargin = "}";
        const char* FacetValueSeparator = "=";
    };

    void SetCustomSeparators(const TPrintableSeparators& separators);

    namespace NDetail {
        struct TBlockOutputWrapper {
            TConstBlockAcc Block;
            ui32 Flags = 0x0;

            TBlockOutputWrapper(TConstBlockAcc block,
                ui32 flags)
                : Block(block)
                , Flags(flags)
            {}
        };

        struct TRequestOutputWrapper {
            TConstRequestAcc Request;
            TConstSequenceAcc Sequence;
            TFacetId Id;
            ui32 Flags = 0x0;

            TRequestOutputWrapper(TConstRequestAcc request,
                TConstSequenceAcc sequence,
                TFacetId id,
                ui32 flags)
                : Request(request)
                , Sequence(sequence)
                , Id(id)
                , Flags(flags)
            {}
        };

        struct TReqBundleOutputWrapper {
            TConstReqBundleAcc Bundle;
            TFacetId Id;
            ui32 Flags = 0x0;

            TReqBundleOutputWrapper(TConstReqBundleAcc bundle,
                TFacetId id,
                ui32 flags)
                : Bundle(bundle)
                , Id(id)
                , Flags(flags)
            {}
        };
    } // NDetail

    NDetail::TBlockOutputWrapper PrintableBlock(TConstBlockAcc block,
        ui32 flags = 0x0);

    NDetail::TRequestOutputWrapper PrintableRequest(TConstRequestAcc request,
        const TFacetId& id, ui32 flags = 0x0);
    NDetail::TRequestOutputWrapper PrintableRequest(TConstRequestAcc request, TConstSequenceAcc sequence,
        const TFacetId& id, ui32 flags = 0x0);
    NDetail::TRequestOutputWrapper PrintableRequest(TConstReqBundleAcc bundle, size_t requestIndex,
        const TFacetId& id, ui32 flags = 0x0);
    NDetail::TReqBundleOutputWrapper PrintableReqBundle(TConstReqBundleAcc bundle,
        const TFacetId& id, ui32 flags = 0x0);

    NDetail::TRequestOutputWrapper PrintableRequest(TConstRequestAcc request,
        ui32 flags = 0x0);
    NDetail::TRequestOutputWrapper PrintableRequest(TConstRequestAcc request, TConstSequenceAcc sequence,
        ui32 flags = 0x0);
    NDetail::TRequestOutputWrapper PrintableRequest(TConstReqBundleAcc bundle, size_t requestIndex,
        ui32 flags = 0x0);
    NDetail::TReqBundleOutputWrapper PrintableReqBundle(TConstReqBundleAcc bundle,
        ui32 flags = 0x0);

    class TRequestPrinter {
    public:
        TRequestPrinter(TConstRequestAcc request,
            TConstSequenceAcc seq = TConstSequenceAcc())
            : Request(request)
            , Seq(seq)
        {
            Init();
        }

        void PrintWords(IOutputStream& out,
            EMatchType matchType = TMatch::OriginalWord,
            ui32 flags = 0x0) const;

        TString GetBlockText(size_t blockIndex, ui32 flags = 0x0) const;

        bool HasMatches(EMatchType matchType) const;
        void SaveMatches(TMultiMap<size_t, TConstMatchAcc>& words) const;

    private:
        void Init();

    private:
        TConstRequestAcc Request;
        TConstSequenceAcc Seq;
        TVector<TBlockPtr> AuxBlocks;

        TSet<EMatchType> Types;
        TMap<size_t, TConstBlockAcc> Blocks;
        TMultiMap<size_t, TConstMatchAcc> Matches;
    };

    inline NDetail::TBlockOutputWrapper PrintableBlock(TConstBlockAcc block, ui32 flags) {
        return NDetail::TBlockOutputWrapper(block, flags);
    }

    inline NDetail::TRequestOutputWrapper PrintableRequest(TConstRequestAcc request, const TFacetId& id, ui32 flags) {
        return NDetail::TRequestOutputWrapper(request, TConstSequenceAcc(), id, flags);
    }

    inline NDetail::TRequestOutputWrapper PrintableRequest(TConstRequestAcc request, ui32 flags) {
        return NDetail::TRequestOutputWrapper(request, TConstSequenceAcc(), TFacetId(), flags);
    }

    inline NDetail::TRequestOutputWrapper PrintableRequest(TConstRequestAcc request, TConstSequenceAcc sequence,
        const TFacetId& id, ui32 flags)
    {
        return NDetail::TRequestOutputWrapper(request, sequence, id, flags);
    }

    inline NDetail::TRequestOutputWrapper PrintableRequest(TConstRequestAcc request, TConstSequenceAcc sequence, ui32 flags) {
        return NDetail::TRequestOutputWrapper(request, sequence, TFacetId(), flags);
    }

    inline NDetail::TRequestOutputWrapper PrintableRequest(TConstReqBundleAcc bundle, size_t requestIndex,
        const TFacetId& id, ui32 flags)
    {
        return NDetail::TRequestOutputWrapper(bundle.GetRequest(requestIndex), bundle.GetSequence(), id, flags);
    }

    inline NDetail::TRequestOutputWrapper PrintableRequest(TConstReqBundleAcc bundle, size_t requestIndex, ui32 flags) {
        return NDetail::TRequestOutputWrapper(bundle.GetRequest(requestIndex), bundle.GetSequence(), TFacetId(), flags);
    }

    inline NDetail::TReqBundleOutputWrapper PrintableReqBundle(TConstReqBundleAcc bundle, const TFacetId& id, ui32 flags) {
        return NDetail::TReqBundleOutputWrapper(bundle, id, flags);
    }

    inline NDetail::TReqBundleOutputWrapper PrintableReqBundle(TConstReqBundleAcc bundle, ui32 flags) {
        return NDetail::TReqBundleOutputWrapper(bundle, TFacetId(), flags);
    }
} // NReqBundle
