#pragma once

#include "reqbundle.h"
#include "request_tr_compatibility_info.h"
#include "richnode_helpers.h"
#include "hashed_sequence.h"
#include "hashed_reqbundle.h"
#include "serializer.h"

#include <ysite/yandex/pure/pure_container.h>

namespace NReqBundle {
    class TRequestSplitter {
    public:
        using TUnpackOptions = NDetail::TRichNodeUnpackHelper::TOptions;
        using TSerializeOptions = TReqBundleSerializer::TOptions;

        struct TOptions {
            bool HashRequests = false;
            bool SerializeBlocks = false;
            bool SaveWordTokens = false;

            TUnpackOptions UnpackOptions;
            TSerializeOptions SerializerOptions;

            TOptions() {}
            TOptions(const TUnpackOptions& unpackOptions)
                : UnpackOptions(unpackOptions)
            {}

            static TOptions GetDefaultQtreeDeserializationOptions()
            {
                TOptions splitOpts;
                splitOpts.SerializeBlocks = true;
                splitOpts.SerializerOptions.BlocksFormat = TCompressorFactory::LZ_LZ4;
                splitOpts.SaveWordTokens = true;
                splitOpts.UnpackOptions.FilterOffBadAttributes = true;
                splitOpts.UnpackOptions.MaxSynonymsPerRequest = 200;
                splitOpts.UnpackOptions.UnpackAndBlocks = true;
                splitOpts.UnpackOptions.UnpackAttributes = true;
                splitOpts.UnpackOptions.UnpackConstraints = true;
                splitOpts.UnpackOptions.UnpackSynonyms = true; // Markup MT_SYNONYM
                splitOpts.UnpackOptions.UnpackWares = true; // Markup MT_ONTO
                splitOpts.UnpackOptions.WordOptions.StripLemmaForms = false;
                return splitOpts;
            }

            static TOptions GetQtreeDeserializationOptionsForSynonyms()
            {
                TOptions splitOpts;
                splitOpts.SerializeBlocks = true;
                splitOpts.SerializerOptions.BlocksFormat = TCompressorFactory::LZ_LZ4;
                splitOpts.SaveWordTokens = true;
                splitOpts.UnpackOptions.FilterOffBadAttributes = true;
                splitOpts.UnpackOptions.MaxSynonymsPerRequest = 200;
                splitOpts.UnpackOptions.UnpackAndBlocks = true;
                splitOpts.UnpackOptions.UnpackAttributes = false;
                splitOpts.UnpackOptions.UnpackConstraints = false;
                splitOpts.UnpackOptions.UnpackSynonyms = true; // Markup MT_SYNONYM
                splitOpts.UnpackOptions.UnpackWares = false; // Markup MT_ONTO
                splitOpts.UnpackOptions.WordOptions.StripLemmaForms = false;
                return splitOpts;
            }
        };

    private:
        TOptions Options;

        NDetail::TRichNodeUnpackHelper UnpackHelper;
        NDetail::THashedSequenceAdapter Seq;
        THolder<NDetail::THashedReqBundleAdapter> Bundle;
        THolder<TReqBundleSerializer> Ser;
        const TPureContainer* Pure = nullptr;

    private:
        TRequestSplitter(TSequenceAcc seq,
            TReqBundleAcc bundle,
            const TOptions& opts);

     public:
        TRequestSplitter(TSequenceAcc seq,
            const TOptions& opts = TOptions())
            : TRequestSplitter(seq, TReqBundleAcc(), opts)
        {}

        TRequestSplitter(TReqBundleAcc bundle,
            const TOptions& opts = TOptions())
            : TRequestSplitter(bundle.Sequence(), bundle, opts)
        {}

        void SetBlocksSerializer(const TSerializeOptions& options);
        void SetPure(const TPureContainer& pure);

        TRequestPtr SplitRequest(const TStringBuf& text,
            const TLangMask& mask = TLangMask(),
            const TVector<std::pair<TFacetId, float>>& facets = {});

        TRequestPtr SplitRequest(const TUtf16String& text,
            const TLangMask& mask = TLangMask(),
            const TVector<std::pair<TFacetId, float>>& facets = {});

        TRequestPtr SplitRequest(const TRichRequestNode& node,
            const TLangMask& mask = TLangMask(),
            const TVector<std::pair<TFacetId, float>>& facets = {});

        TRequestPtr SplitRequestSynonyms(const TRichRequestNode& node,
            const TLangMask& mask = TLangMask(),
            const TVector<std::pair<TFacetId, float>>& facets = {});

        TMatchAcc AddSynonym(TRequestAcc request,
            const TStringBuf& text,
            size_t fromIndex,
            size_t toIndex,
            const TLangMask& mask = TLangMask());

        TMatchAcc AddSynonym(TRequestAcc request,
            const TRichRequestNode& node,
            size_t fromIndex,
            size_t toIndex,
            const TLangMask& mask = TLangMask());

        // Split with TR-iterator compatibility features
        // See SEARCH-2510
        TRequestPtr SplitRequestForTrIterator(
            const TRichRequestNode& node,
            const TLangMask& mask = TLangMask(),
            const TVector<std::pair<TFacetId, float>>& facets = {},
            bool generateTopAndArgs = true);

        static TRequestPtr CreateTelFullRequest(TReqBundle bundle);

        static TMatchAcc AddMatch(TRequestAcc request, EMatchType type,
            size_t blockIndex, size_t fromIndex, size_t toIndex);

    private:
        TRequestPtr SplitRequestImpl(
            const TRichRequestNode& node,
            const TLangMask& mask = TLangMask(),
            const TVector<std::pair<TFacetId, float>>& facets = {},
            bool setTrCompatibilityInfo = false,
            bool generateTopAndArgs = true);

        size_t CheckedAddBlock(const TBlockPtr& block);

        void InitRequestProxes(
            const NDetail::TRichNodeUnpackHelper::TParts& mainParts,
            const TVector<size_t>& mainPartWordsOffset,
            TRequestAcc request);
    };
} // NReqBundle
