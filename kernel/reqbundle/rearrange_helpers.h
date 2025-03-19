#pragma once

#include "validate.h"

namespace NReqBundle {
    namespace NDetail {
        TValidConstraints GetParamsForThesaurusReqBundle();

        TValidConstraints GetParamsForWizardReqBundle();

        TReqBundlePtr PrepareThesaurusReqBundle(
            const TString& binary,
            const NReqBundle::NDetail::TValidConstraints& constraints,
            TString& errValString,
            bool removeEmptyRequests);
    } // namespace NDetail
} // namespace NReqBundle
