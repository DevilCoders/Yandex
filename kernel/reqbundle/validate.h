#pragma once

#include "reqbundle.h"

#include <kernel/lingboost/error_handler.h>

namespace NReqBundle {
namespace NDetail {
    extern const TValidConstraints ConstraintsForSearch;

    class TReqBundleValidater {
    public:
        struct TOptions {
            bool RemoveRequestsWithoutMatches = true;

            TOptions() {
            }
        };

        TReqBundleValidater(const TValidConstraints& constr, const TOptions& options = {});
        TReqBundleValidater()
            : TReqBundleValidater(ConstraintsForSearch)
        {}

        bool IsInErrorState() const {
            return Handler.IsInErrorState();
        }
        void ClearErrorState() {
            Handler.ClearErrorState();
        }
        TString GetFullErrorMessage() const {
            return Handler.GetFullErrorMessage(TStringBuf("<validater message> "));
        }

        // Attempts to transform input bundle
        // into its sub-bundle that satisfies
        // all validation constraints.
        // Input bundle can be modified in the process.
        //
        void Validate(TReqBundlePtr& bundle);

        void PrepareForSearch(TReqBundlePtr& bundle, bool filterOffBadAttributes = false);

    private:
        TValidConstraints Constr;
        TOptions Options;
        NLingBoost::TErrorHandler Handler;
    };
} // NDetail
} // NReqBundle
