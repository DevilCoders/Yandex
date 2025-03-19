#pragma once
#include <kernel/common_server/emulation/abstract/manager.h>
#include <kernel/common_server/emulation/abstract/case.h>

namespace NCS {

    class TConfiguredEmulationsManagerConfig: public IEmulationsManagerConfig {
    private:
        static TFactory::TRegistrator<TConfiguredEmulationsManagerConfig> Registrator;
        CSA_READONLY_DEF(TVector<TEmulationCaseContainer>, EmulationCases);

        virtual void DoToString(IOutputStream& os) const override;
        virtual void DoInit(const TYandexConfig::Section* section) override;
    public:
        static TString GetTypeName() {
            return "configured";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }

        virtual THolder<IEmulationsManager> BuildManager(const IBaseServer& server) const override;
    };

    class TConfiguredEmulationsManager: public IEmulationsManager {
    protected:
        const TConfiguredEmulationsManagerConfig& Config;
    public:
        static TString GetTypeName() {
            return "configured";
        }

        TConfiguredEmulationsManager(const TConfiguredEmulationsManagerConfig& config)
            : Config(config) {
        }

        virtual TMaybe<NUtil::THttpReply> GetHttpReply(const IReplyContext& request) const override;
    };

}
