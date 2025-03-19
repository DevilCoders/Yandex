#pragma once

#include "reqbundle_fwd.h"

#include <kernel/lingboost/error_handler.h>

namespace NReqBundle {
    class TReqBundleSearchParser {
    public:
        struct TOptions {
            bool FilterOffBadAttributes = true;
            bool Validate = true;
        };

        ~TReqBundleSearchParser();
        explicit TReqBundleSearchParser(const TOptions& options);
        TReqBundleSearchParser(const TOptions& options, const NDetail::TValidConstraints& validConstraints);

        bool IsInErrorState() const {
            return Handler.IsInErrorState();
        }
        void ClearErrorState() {
            Handler.ClearErrorState();
        }
        TString GetFullErrorMessage() const {
            return Handler.GetFullErrorMessage(TStringBuf("<reqbundle parser message> "));
        }

        void AddBase64(TStringBuf qbundle, bool isRequestsConstraints = false);

        TReqBundlePtr GetMerged();
        TReqBundlePtr GetPreparedForSearch();
        void PrepareForSearch(TReqBundlePtr& bundle);

    private:
        NLingBoost::TErrorHandler Handler;
        TOptions Options;
        NDetail::TValidConstraints ValidConstraints;
        size_t BundleIndex = 0;
        size_t NumParsed = 0;
        THolder<TReqBundleMerger> Merger;
        THolder<TReqBundleDeserializer> Deser;
        TReqBundlePtr MergedResult;
    };
} // NReqBundle
