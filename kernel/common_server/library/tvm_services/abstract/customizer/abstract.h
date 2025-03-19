#pragma once
#include <library/cpp/object_factory/object_factory.h>
#include <kernel/common_server/library/metasearch/simple/config.h>
#include <kernel/common_server/util/network/neh_request.h>
#include <kernel/common_server/util/auto_actualization.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <library/cpp/yconf/conf.h>
#include <kernel/common_server/library/tvm_services/abstract/request/abstract.h>
#include <kernel/common_server/obfuscator/obfuscators/abstract.h>
#include <kernel/common_server/library/interfaces/tvm_manager.h>

namespace NExternalAPI {

    class TSender;

    class IRequestCustomizationContext {
    public:
        virtual ~IRequestCustomizationContext() = default;
        virtual TMaybe<NSimpleMeta::TConfig> GetRequestConfig(const TString& apiName, const NNeh::THttpRequest& request) const = 0;
        virtual NCS::NObfuscator::TObfuscatorManagerContainer GetObfuscatorManager() const = 0;
        virtual NCS::ITvmManager* GetTvmManager() const {
            return nullptr;
        }

        template <class T>
        const T* GetAs() const {
            return dynamic_cast<const T*>(this);
        }
    };

    class IRequestCustomizer: public IStartStopProcessImpl<const NExternalAPI::TSender&> {
    private:
        CSA_READONLY_PROTECTED(bool, IsNecessary, false);
    protected:
        virtual bool DoTuneRequest(const IServiceApiHttpRequest& /*baseRequest*/, NNeh::THttpRequest& /*request*/, const IRequestCustomizationContext*, const TSender* /*sender*/) const {
            return true;
        }
        virtual void DoInit(const TYandexConfig::Section* section) = 0;
        virtual void DoToString(IOutputStream& os) const = 0;
        virtual bool DoStart(const TSender& /*client*/) override {
            return true;
        }
        virtual bool DoStop() override {
            return true;
        }
    public:
        using TPtr = TAtomicSharedPtr<IRequestCustomizer>;
        using TFactory = NObjectFactory::TObjectFactory<IRequestCustomizer, TString>;
        virtual ~IRequestCustomizer() = default;
        virtual bool TuneRequest(const IServiceApiHttpRequest& baseRequest, NNeh::THttpRequest& request, const IRequestCustomizationContext* context, const TSender* sender) const noexcept final {
            try {
                if (!baseRequest.GetNeedTuning() && !IsNecessary) {
                    return true;
                }
                return DoTuneRequest(baseRequest, request, context, sender);
            } catch (...) {
                ERROR_LOG << "Exception on TuneRequest: " << CurrentExceptionMessage() << Endl;
                return false;
            }
        }
        virtual void Init(const TYandexConfig::Section* section) final {
            IsNecessary = section->GetDirectives().Value("IsNecessary", IsNecessary);
            DoInit(section);
        }
        virtual void ToString(IOutputStream& os) const final {
            os << "IsNecessary: " << IsNecessary << Endl;
            DoToString(os);
        }
        virtual TString GetClassName() const = 0;
    };

    class TRequestCustomizerContainer: public TBaseInterfaceContainer<IRequestCustomizer> {
    private:
        using TBase = TBaseInterfaceContainer<IRequestCustomizer>;
    public:
        using TBase::TBase;
    };

}
