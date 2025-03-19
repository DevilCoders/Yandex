#pragma once
#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/processors/forward_proxy/handler.h>

namespace NCS {
    namespace NFallbackProxy {
        class TStatusProcessorConfig: public NForwardProxy::THandlerConfig {
        private:
            using TBase = NForwardProxy::THandlerConfig;
            CSA_READONLY_DEF(TString, InputPathForMessageId);
            CSA_READONLY_DEF(TString, ReplyTemplate);
            CSA_READONLY_DEF(TString, PersistentQueueId);
            CSA_READONLY(ui32, ReplyCode, HTTP_ACCEPTED);
            bool FallbackForServerErrors = true;
            bool FallbackForUserErrors = true;

        public:
            bool NeedFallback(const ui32 httpCode) const {
                if (IsServerError(httpCode) && FallbackForServerErrors) {
                    return true;
                }
                if (IsUserError(httpCode) && FallbackForUserErrors) {
                    return true;
                }
                return false;
            }

            bool InitFeatures(const TYandexConfig::Section* section) {
                TBase::InitFeatures(section);
                InputPathForMessageId = section->GetDirectives().Value("InputPathForMessageId", InputPathForMessageId);
                ReplyTemplate = section->GetDirectives().Value("ReplyTemplate", ReplyTemplate);
                ReplyCode = section->GetDirectives().Value("ReplyCode", ReplyCode);
                PersistentQueueId = section->GetDirectives().Value("PersistentQueueId", PersistentQueueId);
                FallbackForServerErrors = section->GetDirectives().Value("FallbackForServerErrors", FallbackForServerErrors);
                FallbackForUserErrors = section->GetDirectives().Value("FallbackForUserErrors", FallbackForUserErrors);
                return true;
            }
            void ToStringFeatures(IOutputStream& os) const {
                TBase::ToStringFeatures(os);
                os << "InputPathForMessageId: " << InputPathForMessageId << Endl;
                os << "ReplyTemplate: " << ReplyTemplate << Endl;
                os << "ReplyCode: " << ReplyCode << Endl;
                os << "PersistentQueueId: " << PersistentQueueId << Endl;
                os << "FallbackForServerErrors: " << FallbackForServerErrors << Endl;
                os << "FallbackForUserErrors: " << FallbackForUserErrors << Endl;
            }
        };

        class TStatusProcessor: public NCS::NForwardProxy::TCommonHandler<TStatusProcessor, TStatusProcessorConfig> {
        private:
            using TBase = NCS::NForwardProxy::TCommonHandler<TStatusProcessor, TStatusProcessorConfig>;

        protected:
            virtual NCS::NForwardProxy::TReportConstructorContainer BuildReportConstructor() const override;

        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "fallback-proxy-status-processor";
            }
        };
    }
}
