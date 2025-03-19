#pragma once
#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/processors/forward_proxy/handler.h>

namespace NCS {
    namespace NFallbackProxy {
        class TWriterProcessorConfig: public NForwardProxy::THandlerConfig {
        private:
            using TBase = NForwardProxy::THandlerConfig;
            CSA_READONLY_DEF(TString, PersistentQueueId);
            CSA_READONLY_DEF(TString, InputPathForMessageId);
            CSA_READONLY_DEF(TString, OutputPathForMessageId);
            CSA_READONLY_FLAG(ServerErrorsForWriting, true);

        public:
            bool InitFeatures(const TYandexConfig::Section* section) {
                TBase::InitFeatures(section);
                InputPathForMessageId = section->GetDirectives().Value("InputPathForMessageId", InputPathForMessageId);
                OutputPathForMessageId = section->GetDirectives().Value("OutputPathForMessageId", OutputPathForMessageId);
                PersistentQueueId = section->GetDirectives().Value("PersistentQueueId", PersistentQueueId);
                ServerErrorsForWritingFlag = section->GetDirectives().Value("ServerErrorsForWriting", ServerErrorsForWritingFlag);
                return true;
            }
            void ToStringFeatures(IOutputStream& os) const {
                TBase::ToStringFeatures(os);
                os << "InputPathForMessageId: " << InputPathForMessageId << Endl;
                os << "OutputPathForMessageId: " << OutputPathForMessageId << Endl;
                os << "PersistentQueueId: " << PersistentQueueId << Endl;
                os << "ServerErrorsForWriting: " << IsServerErrorsForWriting() << Endl;
            }
        };

        class TWriterProcessor: public NCS::NForwardProxy::TCommonHandler<TWriterProcessor, TWriterProcessorConfig> {
        private:
            using TBase = NCS::NForwardProxy::TCommonHandler<TWriterProcessor, TWriterProcessorConfig>;

        protected:
            virtual NCS::NForwardProxy::TReportConstructorContainer BuildReportConstructor() const override;

        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "fallback-proxy-writer-processor";
            }
        };
    }
}
