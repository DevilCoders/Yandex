#pragma once

#include <kernel/qtree/richrequest/richnode_fwd.h>
#include <kernel/reqbundle/reqbundle_fwd.h>

namespace NTrOverReqBundleIterator {

    NReqBundle::TReqBundlePtr ConvertRichTreeToReqBundle(
        const TRichTreeConstPtr& richTree,
        bool filterOffBadAttribute,
        bool useConstraintChecker,
        bool generateTopAndArgs);

}
