#pragma once

#include "environment.h"
#include "geo_checker.h"
#include "reloadable_data.h"
#include "request_params.h"
#include "service_param_holder.h"
#include "time_stats.h"

#include <library/cpp/http/io/stream.h>

class THttpHeaders;

namespace NLCookie {
    struct IKeychain;
}

namespace NAntiRobot {
    /**
     * Represents a general HTTP request.
     *
     * Parses location, CGI-params, headers.
     */
    class THttpInfo : public TRequest {
    public:
        THttpInfo(THttpInput& input, const TString& requesterAddr, TTimeStats* readStats,
                  bool failOnReadRequestTimeout, bool isWrappedRequest);

        void PrintData(IOutputStream& os, bool forceMaskCookies) const override;
        void PrintHeaders(IOutputStream& os, bool forceMaskCookies) const override;
        void SerializeTo(NCacheSyncProto::TRequest& serializedRequest) const override;

        class TBadRequest : public yexception {
        };

        class TTimeoutException : public yexception {
        };

    private:
        void DebugOutput();
        void WriteMaskedFirstLine(IOutputStream& out) const;

        char Buf_[20];
        TString FirstLine;
        THttpHeaders HttpHeaders;
    };

    using TSpravkaIgnorePredicate = std::function<bool(const TUid&, EHostType)>;

    /**
     * Evaluates Antirobot-specific information from the request: ReqType, HostType, Uid,
     * etc.
     */
    class TFullReqInfo: public THttpInfo {
    public:
        TFullReqInfo(
            THttpInput& input,
            const TString& wrappedRequest,
            const TString& requesterAddr,
            const TReloadableData& reloadable,
            const TPanicFlags& panicFlags,
            TSpravkaIgnorePredicate spravkaIgnorePredicate,
            const TRequestGroupClassifier* groupClassifier = nullptr,
            const TEnv* env = nullptr,
            TMaybe<TString> uniqueKey = Nothing(),
            TVector<TExpInfo> experimentsHeader = {},
            TMaybe<ui64> randomParam = {}
        );

        class TBadExpect : public yexception {
        };

        class TInvalidPartnerRequest : public yexception {
        };

        class TUidCreationFailure : public yexception {
        };

    private:
        void DebugOutput();
        void ApplyHackForXmlPartners();
        void CalcBlockCategory();
        void HackTrailingContentForVerochka(const TString& wrappedRequest);
        void FillJws(const TEnv* env);
    };

    THolder<TFullReqInfo> ParseFullreq(const NCacheSyncProto::TRequest& serializedRequest, const TEnv& env);

    THolder<TRequest> CreateDummyParsedRequest(
        const TString& request,
        const TRequestClassifier& classifier = {},
        const TRequestGroupClassifier& groupClassifier = {},
        const TString& geodataBinPath = ""
    );

    void SplitUri(TStringBuf& req, TStringBuf& doc, TStringBuf& cgi);

    void WriteMaskedUrl(EHostType service, TStringBuf url, IOutputStream& out);
    void WriteMaskedCgiParam(EHostType service, TStringBuf url, IOutputStream& out, size_t& i);
    TSpravkaIgnorePredicate GetSpravkaIgnorePredicate(const TEnv& env);
    TSpravkaIgnorePredicate GetEmptySpravkaIgnorePredicate();

} // namespace NAntiRobot
