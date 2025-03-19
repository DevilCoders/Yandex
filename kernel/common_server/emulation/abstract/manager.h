#pragma once
#include <kernel/common_server/util/auto_actualization.h>
#include <kernel/common_server/util/network/http_request.h>
#include <library/cpp/yconf/conf.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/library/searchserver/simple/context/replier.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {

    class IEmulationsManager: public IAutoActualization {
    public:
        IEmulationsManager()
            : IAutoActualization("EmulationsManager") {
        }

        using TPtr = TAtomicSharedPtr<IEmulationsManager>;

        virtual bool Refresh() override {
            return true;
        }

        virtual ~IEmulationsManager() = default;
        virtual TMaybe<NUtil::THttpReply> GetHttpReply(const IReplyContext& context) const = 0;
    };

    class IEmulationsManagerConfig {
    protected:
        virtual void DoInit(const TYandexConfig::Section* section) = 0;
        virtual void DoToString(IOutputStream& os) const = 0;
    public:
        using TPtr = TAtomicSharedPtr<IEmulationsManagerConfig>;
        using TFactory = NObjectFactory::TObjectFactory<IEmulationsManagerConfig, TString>;
        virtual ~IEmulationsManagerConfig() = default;
        virtual THolder<IEmulationsManager> BuildManager(const IBaseServer& server) const = 0;
        virtual TString GetClassName() const = 0;

        void Init(const TYandexConfig::Section* section) {
            DoInit(section);
        }

        void ToString(IOutputStream& os) const {
            DoToString(os);
        }
    };

    class TEmulationsManagerConfig: public TBaseInterfaceContainer<IEmulationsManagerConfig> {
    public:
    };

}
