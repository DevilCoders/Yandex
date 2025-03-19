#pragma once
#include "ct_parser.h"
#include <kernel/common_server/library/request_session/request_session.h>
#include <kernel/common_server/util/network/http_request.h>
#include <kernel/common_server/util/network/neh_request.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/obfuscator/obfuscators/abstract.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <library/cpp/http/misc/httpcodes.h>

namespace NExternalAPI {

    enum class EReplyClass {
        Success,
        ServiceProblem,
        IncorrectRequest
    };

    class IServiceApiHttpRequest {
    private:
        using TLinks = TMap<TString, TString>;
        CS_ACCESS(IServiceApiHttpRequest, TString, RequestId, CreateGuidAsString());
        CSA_DEFAULT(IServiceApiHttpRequest, TLinks, Links);
        CSA_DEFAULT(IServiceApiHttpRequest, TSet<TString>, ReplyLinks);
        CSA_MUTABLE_DEF(IServiceApiHttpRequest, TString, SignalId);
        CSA_MAYBE(IServiceApiHttpRequest, TDuration, Timeout);
        CS_ACCESS(IServiceApiHttpRequest, bool, NeedTuning, true);
    public:
        IServiceApiHttpRequest() = default;
        IServiceApiHttpRequest(const TDuration& timeout)
            : Timeout(timeout) {

        }

        virtual TVector<NCS::NLogging::TBaseLogRecord> TuneLogEvent(const NCS::NLogging::TBaseLogRecord& event) const;
        virtual TSignalTagsSet BuildSignalTags() const;
        virtual TString GetBodyForLog(const NNeh::THttpRequest& request) const;
        virtual NCS::NObfuscator::TObfuscatorKeyMap GetObfuscatorKey() const;

        using EReplyClass = NExternalAPI::EReplyClass;

        class IBaseResponse {
        private:
            CSA_DEFAULT(IBaseResponse, TString, RequestId);
            CSA_DEFAULT(IBaseResponse, TRequestSessionContainer, RequestSession)
            CS_ACCESS(IBaseResponse, i32, Code, 0);
            CSA_READONLY(bool, IsReportCorrect, false);
        protected:
            virtual bool IsReplyCodeSuccess(const i32 code) const {
                return code == 200;
            }
            bool IsReplyCodeSuccess() const {
                return IsReplyCodeSuccess(Code);
            }
            virtual bool DoParseHttpResponse(const NUtil::THttpReply& reply) = 0;

        public:
            NCS::NLogging::TBaseLogRecord GetResponseLog() const {
                NCS::NLogging::TBaseLogRecord result;
                result.Add("req_id", RequestId);
                return result;
            }

            EReplyClass GetReplyClass() const {
                if (IsSuccess()) {
                    return EReplyClass::Success;
                }
                if (Code == 0) {
                    return EReplyClass::ServiceProblem;
                }
                if (Code == HTTP_OK && !IsReportCorrect) {
                    return EReplyClass::ServiceProblem;
                }
                if (GetCode() / 100 == 5) {
                    return EReplyClass::ServiceProblem;
                } else if (GetCode() / 100 == 4) {
                    return EReplyClass::IncorrectRequest;
                } else {
                    return EReplyClass::ServiceProblem;
                }
            }

            void ParseReply(const NUtil::THttpReply& reply) noexcept {
                try {
                    Code = reply.Code();
                    IsReportCorrect = DoParseHttpResponse(reply);
                } catch (...) {
                    TFLEventLog::Log(CurrentExceptionMessage())("code", reply.Code())("content", reply.Content());
                    IsReportCorrect = false;
                }
            }

            bool IsSuccess() const {
                return IsReplyCodeSuccess(Code) && IsReportCorrect;
            }
            bool IsSuccessCode() const {
                return IsReplyCodeSuccess(Code);
            }

            virtual TSignalTagsSet BuildSignalTags() const {
                return TSignalTagsSet();
            }

            virtual void ConfigureFromRequest(const IServiceApiHttpRequest* /*request*/){};

            virtual ~IBaseResponse() = default;
        };

        class IResponse: public IBaseResponse {
        protected:
            virtual bool DoParseReply(const NUtil::THttpReply& reply) = 0;
            virtual bool DoParseHeaders(const THttpHeaders& /*headers*/) {
                return true;
            }
            virtual bool DoParseError(const NUtil::THttpReply& /*reply*/) {
                return true;
            }
            virtual bool DoParseHttpResponse(const NUtil::THttpReply& reply) override final {
                if (!IsReplyCodeSuccess(GetCode())) {
                    return DoParseError(reply);
                } else {
                    return DoParseReply(reply) && DoParseHeaders(reply.GetHeaders());
                }
            }

        public:
        };

        class IResponseByContentType: public IBaseResponse, public TBaseParserCallbackContext {
        protected:
            virtual bool DoParseHttpResponse(const NUtil::THttpReply& reply) override {
                TStringBuf content(reply.Content().data(), reply.Content().size());
                TParser parser(content, reply.GetHeaders(), *this);
                return parser.Parse();
            }

        public:
        };

        virtual bool BuildHttpRequest(NNeh::THttpRequest& request) const = 0;
        
        virtual ~IServiceApiHttpRequest() = default;
    };
}
