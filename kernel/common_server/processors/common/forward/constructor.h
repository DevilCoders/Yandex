#pragma once
#include <library/cpp/yconf/conf.h>
#include <library/cpp/object_factory/object_factory.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/report/json.h>

namespace NCS {
    namespace NForwardProxy {
        class IReportConstructor;
        class IReportConstructorConfig {
        protected:
            using TFactory = NObjectFactory::TObjectFactory<IReportConstructorConfig, TString>;
            virtual bool DoInit(const TYandexConfig::Section* section) = 0;
            virtual void DoToString(IOutputStream& os) const = 0;
        public:
            using TPtr = TAtomicSharedPtr<IReportConstructorConfig>;
            virtual ~IReportConstructorConfig() = default;
            virtual THolder<IReportConstructor> BuildConstructor(const IBaseServer& server) const = 0;
            bool Init(const TYandexConfig::Section* section) {
                return DoInit(section);
            }
            void ToString(IOutputStream& os) const {
                return DoToString(os);
            }
            virtual TString GetClassName() const = 0;
        };

        class IReportConstructor {
        protected:
            virtual bool DoBuildReport(const NNeh::THttpRequest& request, const NUtil::THttpReply& originalResponse, TJsonReport::TGuard& g) const = 0;
            virtual bool DoBuildReportOnException(const NNeh::THttpRequest& request, const TString& exceptionMessage, TJsonReport::TGuard& g) const = 0;
            virtual bool DoBuildReportNoReply(const NNeh::THttpRequest& request, TJsonReport::TGuard& g) const = 0;
        public:
            using TPtr = TAtomicSharedPtr<IReportConstructor>;
            virtual ~IReportConstructor() = default;
            bool BuildReportOnException(const NNeh::THttpRequest& request, const TString& exceptionMessage, TJsonReport::TGuard& g) const;
            bool BuildReportNoReply(const NNeh::THttpRequest& request, TJsonReport::TGuard& g) const;
            bool BuildReport(const NNeh::THttpRequest& request, const NUtil::THttpReply& originalResponse, TJsonReport::TGuard& g) const;
        };

        class TReportConstructorContainer: public TBaseInterfaceContainer<IReportConstructor> {
        private:
            using TBase = TBaseInterfaceContainer<IReportConstructor>;
        public:
            using TBase::TBase;
            bool BuildReport(const NNeh::THttpRequest& request, const NUtil::THttpReply& originalResponse, TJsonReport::TGuard& g) const;
            bool BuildReportNoReply(const NNeh::THttpRequest& request, TJsonReport::TGuard& g) const;
            bool BuildReportOnException(const NNeh::THttpRequest& request, const TString& exceptionMessage, TJsonReport::TGuard& g) const;
        };
    }
}
