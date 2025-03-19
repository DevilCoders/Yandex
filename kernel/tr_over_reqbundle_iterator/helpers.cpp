#include "helpers.h"

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/reqbundle/request_pure.h>
#include <kernel/reqbundle/request_splitter.h>


namespace NTrOverReqBundleIterator {

    NReqBundle::TReqBundlePtr ConvertRichTreeToReqBundle(
        const TRichTreeConstPtr& richTree,
        bool filterOffBadAttribute,
        bool useConstraintChecker,
        bool generateTopAndArgs)
    {
        NReqBundle::TReqBundlePtr bundle = new TReqBundle();

        NReqBundle::TRequestSplitter::TUnpackOptions options;
        options.UnpackConstraints = true;
        options.UnpackAttributes = true;
        options.UnpackAndBlocks = true;
        options.UnpackQuotedConstraint = useConstraintChecker && richTree->Root;
        options.FilterOffBadAttributes = filterOffBadAttribute;

        NReqBundle::TRequestSplitter splitter(*bundle, options);
        NReqBundle::TRequestPtr requestPtr = splitter.SplitRequestForTrIterator(*richTree->Root, TLangMask(),
            { { NReqBundle::MakeFacetId(NLingBoost::TExpansion::OriginalRequest), 1.0f } }, generateTopAndArgs);

        if (requestPtr) {
            NReqBundle::FillRevFreqs(*bundle);
        }

        return bundle;
    }

}
