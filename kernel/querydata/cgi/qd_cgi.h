#pragma once

#include "qd_cgi_strings.h"
#include "qd_cgi_utils.h"
#include "qd_request.h"

#include <kernel/querydata/common/querydata_traits.h>

#include <library/cpp/scheme/scheme.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/vector.h>
#include <util/generic/noncopyable.h>

namespace NQueryData {

    class IRequestBuilder : public TThrRefBase, public TNonCopyable {
    public:
        using TRef = TIntrusivePtr<IRequestBuilder>;

        virtual void FormRequests(TVector<TString>&, const TRequestRec&) const = 0;

        virtual void ParseFormedRequest(TRequestRec&, const TCgiParameters&) const = 0;

        virtual void AdaptWebSearchRequest(TRequestRec& val, const TCgiParameters& cgi) const = 0;

        void FormRequestsJson(TVector<TString>&, const NSc::TValue&) const;

        void ParseFormedRequestJson(NSc::TValue& val, const TCgiParameters& cgi) const;

        void AdaptWebSearchRequestJson(NSc::TValue& val, const TCgiParameters& cgi) const {
            ParseFormedRequestJson(val, cgi);
        }
    };


    class TRequestBuilderOptions {
    public:
        bool FormRequestsOnlyIfHasItems = false;
        bool EnableUrls = false;
        bool AlwaysUseRelev = false;
        ECgiCompression UrlsCompression = CC_PLAIN;

    public:
        TRequestBuilderOptions() = default;

        TRequestBuilderOptions& SetFormRequestOnlyIfHasItems(bool val) {
            FormRequestsOnlyIfHasItems = val;
            return *this;
        }

        TRequestBuilderOptions& SetUrlsCompression(ECgiCompression val) {
            UrlsCompression = val;
            return *this;
        }

        TRequestBuilderOptions& SetEnableUrls(bool val) {
            EnableUrls = val;
            return *this;
        }

        static TRequestBuilderOptions BanFilterOpts(bool enableUrls = false, ECgiCompression cc = CC_PLAIN) {
            return TRequestBuilderOptions().SetFormRequestOnlyIfHasItems(true)
                    .SetEnableUrls(enableUrls).SetUrlsCompression(cc);
        }

        static TRequestBuilderOptions QuerySearchMidSSDOpts() {
            return BanFilterOpts(false, CC_PLAIN);
        }

        static TRequestBuilderOptions QuerySearchOpts() {
            return TRequestBuilderOptions();
        }
    };


    class TQuerySearchRequestBuilder : public IRequestBuilder {
    public:
        explicit TQuerySearchRequestBuilder(TRequestBuilderOptions opts = TRequestBuilderOptions(), TRequestSplitLimits lims = TRequestSplitLimits())
            : Limits(lims)
            , Options(opts)
        {}

        void FormRequests(TVector<TString>&, const TRequestRec&) const override;

        void ParseFormedRequest(TRequestRec& val, const TCgiParameters& cgi) const override;

        void AdaptWebSearchRequest(TRequestRec& val, const TCgiParameters& cgi) const override {
            ParseFormedRequest(val, cgi);
        }

    protected:
        TRequestSplitLimits Limits;
        TRequestBuilderOptions Options;
    };

}
