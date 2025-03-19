#pragma once
#include <kernel/daemon/module/module.h>
#include <library/cpp/messagebus/scheduler/scheduler.h>
#include <library/cpp/logger/global/global.h>
#include <util/system/mem_info.h>
#include <util/system/getpid.h>

const TString PARENT_EXISTENCE_CHECKER_MODULE_NAME = "ParentExistenceChecker";

namespace NParentExistenceChecker {
    class TParentExistenceConfig : public TPluginConfig<IDaemonModuleConfig> {

    protected:
        virtual bool DoCheck() const override {
            return true;
        }

        virtual void DoInit(const TYandexConfig::Section& componentSection) override {
            const TYandexConfig::Directives& directives = componentSection.GetDirectives();
            CHECK_WITH_LOG(directives.find("Enabled") != directives.end());
            CHECK_WITH_LOG(directives.GetValue("Enabled", Enabled));

            CHECK_WITH_LOG(directives.find("ParentPid") != directives.end());
            CHECK_WITH_LOG(directives.GetValue("ParentPid", ParentPid));

            if (directives.find("CheckInterval") != directives.end()) {
                ui32 interval = 0;
                CHECK_WITH_LOG(directives.GetValue("CheckInterval", interval));
                CheckInterval = TDuration::Seconds(interval);
            }
        }

        virtual void DoToString(IOutputStream& so) const override {
            so << "Enabled:" << Enabled << Endl;
            so << "ParentPid:" << ParentPid << Endl;
            so << "CheckInterval:" << CheckInterval.Seconds() << Endl;
        }

    public:
        virtual ~TParentExistenceConfig() {}

        inline TDuration GetCheckInterval() const {
            return CheckInterval;
        }

        inline TProcessId GetParentPid() const {
            return ParentPid;
        }

        inline void SetParentPid(const TProcessId pid) {
            ParentPid = pid;
        }

        inline void SetCheckInterval(TDuration interval) {
            CheckInterval = interval;
        }

        inline bool IsEnabled() const {
            return Enabled;
        }

    private:
        bool Enabled = false;
        TProcessId ParentPid = 0;
        TDuration CheckInterval = TDuration::Seconds(10);
        static IDaemonModuleConfig::TFactory::TRegistrator<TParentExistenceConfig> Registrator;
    };


    class TParentExistenceChecker : public IDaemonModule {

    private:
        class TParentExistenceCheckerTask : public NBus::NPrivate::IScheduleItem {
        public:
            TParentExistenceCheckerTask(TParentExistenceChecker& owner)
                : NBus::NPrivate::IScheduleItem(TInstant::Now() + owner.GetConfig().GetCheckInterval())
                , Owner(owner)
            {
            }

        private:
            virtual void Do() override {
                Owner.EnqueueWithCheck();
            }

        private:
            TParentExistenceChecker& Owner;
        };

    public:
        TParentExistenceChecker(const IServerConfig& config)
            : ParentExistenceConfig(*config.GetModuleConfig<TParentExistenceConfig>(PARENT_EXISTENCE_CHECKER_MODULE_NAME))
        {

        }

        virtual ~TParentExistenceChecker() {}

        virtual bool Start() override {
            if (ParentExistenceConfig.IsEnabled()) {
                return EnqueueWithCheck();
            }
            return true;
        }

        virtual bool Stop(const TStopContext&) override {
            Scheduler.Stop();
            return true;
        }

        virtual TString Name() const override {
            return PARENT_EXISTENCE_CHECKER_MODULE_NAME;
        }

        const TParentExistenceConfig& GetConfig() const {
            return ParentExistenceConfig;
        }

        bool EnqueueWithCheck() {
            if (!CheckExistence(ParentExistenceConfig.GetParentPid())) {
                _exit(-1);
            }
            Scheduler.Schedule(new TParentExistenceCheckerTask(*this));
            return true;
        }

        static bool CheckExistence(TProcessId pid) {
            INFO_LOG << "Check pid " << pid << Endl;
            try {
                NMemInfo::TMemInfo info = NMemInfo::GetMemInfo(static_cast<pid_t>(pid));
                INFO_LOG << pid << " occupy memory:" << info.RSS << " " << info.VMS << Endl;
                return true;
            } catch (const yexception& ex) {
                INFO_LOG << ex.what() << Endl;
                return false;
            }
        }

    private:
        NBus::NPrivate::TScheduler Scheduler;
        const TParentExistenceConfig ParentExistenceConfig;
        static TDaemonModules::TFactory::TRegistrator<TParentExistenceChecker> Registrator;
    };

}
