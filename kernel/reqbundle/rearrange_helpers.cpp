#include "rearrange_helpers.h"
#include "serializer.h"

#include <library/cpp/string_utils/base64/base64.h>

namespace NReqBundle {
    namespace NDetail {

        TValidConstraints GetParamsForThesaurusReqBundle() {
            NReqBundle::NDetail::TValidConstraints constraints;
            constraints.IgnoreElems = true;
            constraints.NeedBlocks = false;
            constraints.NeedBinaries = false;
            constraints.NeedOriginal = false;
            constraints.NeedNonEmpty = false;
            return constraints;
        }

        TValidConstraints GetParamsForWizardReqBundle() {
            NReqBundle::NDetail::TValidConstraints constraints;
            constraints.IgnoreElems = true;
            constraints.NeedBlocks = false;
            constraints.NeedBinaries = false;
            constraints.NeedOriginal = true;
            constraints.NeedNonEmpty = true;
            return constraints;
        }

        TReqBundlePtr PrepareThesaurusReqBundle(
            const TString& binary,
            const NReqBundle::NDetail::TValidConstraints& constraints,
            TString& errValString,
            bool removeEmptyRequests)
        {
            Y_ASSERT(errValString.empty());
            TString decodedBinary = Base64StrictDecode(binary);
            NReqBundle::NSer::TDeserializer deserializer;
            TReqBundlePtr result = deserializer.DeserializeBundle(decodedBinary);
            NReqBundle::NDetail::TReqBundleValidater::TOptions options;
            options.RemoveRequestsWithoutMatches = removeEmptyRequests;

            NReqBundle::NDetail::TReqBundleValidater validater(constraints, options);
            validater.Validate(result);
            if (validater.IsInErrorState()) {
                errValString = validater.GetFullErrorMessage();
            }
            return result;
        }
    } // namespace NDetail
} //namespace NReqBundle

