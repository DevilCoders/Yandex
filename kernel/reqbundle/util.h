#pragma once

#include "reqbundle_fwd.h"
#include "parse_for_search.h"

#include <kernel/reqbundle/request_helpers.h>
#include <kernel/qtree/richrequest/richnode_fwd.h>


namespace NReqBundle {
    struct TReqBundleFromRichTreeOpts {
        bool TrHitsCompatibilityMode = false;
        bool FilterOffBadAttribute = false;
    };

    TReqBundlePtr ReqBundleFromRichTree(const TRichTreeConstPtr& richTree,
                                        const TReqBundleFromRichTreeOpts& opts = TReqBundleFromRichTreeOpts());
    TString Base64QBundleFromRichTree(const TRichTreeConstPtr& richTree);
    bool AddLboostExpansion(TReqBundle& qbundle, EExpansionType expansionType,
                            const TLangMask& langs, const TString& expansionText,
                            float expansionWeight = 1.0f); // 1.0 is default weight

    TVector<float> ExtractQueryWordWeights(const TReqBundle& qbundle);
    TMaybe<ui32> GetTrCompatibleWordCount(const TReqBundle& qbundle);
    ui32 GetOriginalQueryWordCount(const TReqBundle& qbundle);

    /* We assume PrepareAllBlocks call before invocation */
    ui64 GetStopWordsMask(const TReqBundle& qbundle);

    /* We assume PrepareAllBlocks call before invocation */
    ui32 GetNonStopWordsCount(const TReqBundle& qbundle);

    /* We assume PrepareAllBlocks call before invocation */
    ui64 GetNonStopWordsMask(const TReqBundle& qbundle);

    TReqBundlePtr Base64UnpackBundle(TStringBuf packedBundle);

    TVector<TReqBundlePtr> FetchBundlesFromCgi(TStringBuf cgi);

    TVector<TString> FetchPackedBundlesFromCgi(TStringBuf cgi);

    TReqBundlePtr MergeBase64Bundles(const TVector<TString>& packedBundles);

    TString PackAsBinary(NReqBundle::TConstReqBundleAcc bundle);
    TString PackAsText(NReqBundle::TConstReqBundleAcc bundle);

    struct TOriginalReqBundleFetchingOptions {
        bool EnableAttributesInReqBundle = false;
        bool UseThesBundle = true;
        TMergeSynonymsOptions MergeOptions;

        void Parse(TStringBuf jsonOpts);
    };

    TReqBundlePtr FetchAndMergeOriginalReqBundleFromCgi(TStringBuf cgi, TOriginalReqBundleFetchingOptions options);

    template <typename TContainer>
    NReqBundle::TReqBundlePtr DeserializeReqBundle(const TContainer& bundles,
                                                   bool filterOffBadAttributes,
                                                   bool prepareForSearch = true)
    {
        Y_ENSURE(!bundles.empty());
        NReqBundle::TReqBundleSearchParser::TOptions parserOptions{
            .FilterOffBadAttributes = filterOffBadAttributes
        };
        NReqBundle::TReqBundleSearchParser parser(parserOptions);

        for (const auto& bundle : bundles) {
            parser.AddBase64(bundle);
        }

        if (prepareForSearch) {
            return parser.GetPreparedForSearch();
        }
        /* else do not validate and simply return merged */
        return parser.GetMerged();
    }
} // NReqBundle
