#include "parse_for_search.h"

#include "reqbundle.h"
#include "merge.h"
#include "serializer.h"
#include "validate.h"

#include <library/cpp/string_utils/base64/base64.h>


namespace {
    using TErrorGuard = NLingBoost::TErrorHandler::TGuard;
} // namespace

namespace NReqBundle {
    TReqBundleSearchParser::~TReqBundleSearchParser() = default;

    TReqBundleSearchParser::TReqBundleSearchParser(const TOptions& options)
        : TReqBundleSearchParser(options, NDetail::ConstraintsForSearch)
    {
    }

    TReqBundleSearchParser::TReqBundleSearchParser(const TOptions& options, const NDetail::TValidConstraints& validConstraints)
        : Handler(NLingBoost::TErrorHandler::EFailMode::SkipOnError)
        , Options(options)
        , ValidConstraints(validConstraints)
    {
        TReqBundleDeserializer::TOptions deserOpts;
        // if validation is disabled, assume all reqbundle data is needed (i.e. split bundles)
        deserOpts.FailMode = TReqBundleDeserializer::EFailMode::SkipOnError;
        deserOpts.NeedAllData = !options.Validate;
        Deser = MakeHolder<TReqBundleDeserializer>(deserOpts);
    }

    void TReqBundleSearchParser::AddBase64(TStringBuf qbundle, bool isRequestsConstraints) {
        TErrorGuard bundleGuard{Handler, "reqbundle", BundleIndex};
        BundleIndex += 1;

        TReqBundlePtr bundle = new TReqBundle;

        try {
            TString binary = Base64StrictDecode(qbundle);
            Deser->Deserialize(binary, *bundle);
        } catch (...) {
            Handler.Error(
                yexception{}
                    << "failed to deserialize"
                    << "\n" << CurrentExceptionMessage());

            bundle = nullptr;
        }

        if (!bundle) {
            return;
        }

        NumParsed += 1;

        if (Deser->IsInErrorState()) {
            Handler.Error(
                yexception{}
                    << "protobuf is partially invalid"
                    << "\n" << Deser->GetFullErrorMessage());

            Deser->ClearErrorState();
        }

        if (Merger) {
            isRequestsConstraints ? Merger->AddRequestsConstraints(*bundle) : Merger->AddBundle(*bundle);
        } else if (!MergedResult) {
            MergedResult = bundle;
        } else {
            TReqBundleMerger::TOptions mergerOptions;
            // if validation is disabled, assume all reqbundle data is needed (i.e. split bundles)
            mergerOptions.NeedAllData = !Options.Validate;
            Merger = MakeHolder<TReqBundleMerger>(mergerOptions);
            isRequestsConstraints ? Merger->AddRequestsConstraints(*MergedResult) : Merger->AddBundle(*MergedResult);
            isRequestsConstraints ? Merger->AddRequestsConstraints(*bundle) : Merger->AddBundle(*bundle);
            MergedResult = nullptr;
        }
    }

    TReqBundlePtr TReqBundleSearchParser::GetMerged() {
        if (0 == NumParsed) {
            Y_ASSERT(!MergedResult);
            Y_ASSERT(!Merger);
        }

        if (MergedResult) {
            return MergedResult;
        }

        if (Merger) {
            MergedResult = Merger->GetResult();
        }

        return MergedResult;
    }


    void TReqBundleSearchParser::PrepareForSearch(TReqBundlePtr& bundle) {
        if (Y_UNLIKELY(!bundle)) {
            return;
        };

        auto seq = bundle->Sequence();
        for (size_t i : xrange(seq.GetNumElems())) {
            auto elem = seq.Elem(i);

            THolder<TReqBundleDeserializer> deser;
            TReqBundleDeserializer::TOptions deserOpts;
            deserOpts.FailMode = TReqBundleDeserializer::EFailMode::SkipOnError;
            // if validation is disabled, assume all reqbundle data is needed (i.e. split bundles)
            deserOpts.NeedAllData = !Options.Validate;
            deser = MakeHolder<TReqBundleDeserializer>(deserOpts);

            if (!elem.HasBlock()) {
                Y_ASSERT(!!deser);
                elem.PrepareBlock(*deser);
                if (deser->IsInErrorState()) {
                    Handler.Error(yexception{}
                        << "errors in block deserialization\n"
                        << deser->GetFullErrorMessage());
                    deser->ClearErrorState();
                    bundle = nullptr;
                    return;
                }
            }
        }
    }

    TReqBundlePtr TReqBundleSearchParser::GetPreparedForSearch() {
        TErrorGuard guard{Handler, "prepare_for_search"};

        TReqBundlePtr bundle = GetMerged();

        if (Options.Validate) {
            NReqBundle::NDetail::TReqBundleValidater val(ValidConstraints);
            val.PrepareForSearch(bundle, Options.FilterOffBadAttributes);
            if (val.IsInErrorState()) {
                Handler.Error(
                    yexception{}
                        << "validation finished with errors"
                        << "\n" << val.GetFullErrorMessage());
            }
        } else {
            PrepareForSearch(bundle);
        }

        if (!bundle) {
            Handler.Error(
                yexception{}
                    << "Bundle is empty after merge");
        }
        
        Y_ASSERT(
            !bundle
            || NReqBundle::NDetail::IsValidReqBundle(
                *bundle,
                ValidConstraints));

        return bundle;
     }
} // NReqBundle
