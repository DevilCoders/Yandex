#pragma once

#include "server.h"
#include "config.h"

#include <kernel/common_server/server/server.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <library/cpp/mediator/messenger.h>
#include <kernel/common_server/migrations/manager.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/tags/manager.h>
#include <kernel/common_server/abstract/common.h>


namespace NServerTest {

    class TTestWithDatabase: public NUnitTest::TBaseFixture, public IMessageProcessor {
        CS_ACCESS(TTestWithDatabase, TAtomic, IsActive, 0);
        TThreadPool RegularTestThreadPool;
    public:
        TTestWithDatabase() {
            RegisterGlobalMessageProcessor(this);
        }

        ~TTestWithDatabase() {
            AtomicSet(IsActive, 0);
            RegularTestThreadPool.Stop();
            UnregisterGlobalMessageProcessor(this);
        }

        TString Name() const override {
            return "TTestWithDatabase";
        }

        virtual bool Process(IMessage* message) override {
            const TMessageOnAfterDatabaseConstructed* messEvent = dynamic_cast<const TMessageOnAfterDatabaseConstructed*>(message);
            if (messEvent) {
                AtomicSet(IsActive, 1);
                RegularTestThreadPool.Start(1);
                RegularTestThreadPool.SafeAddAndOwn(MakeHolder<TRegularTestChecker>(this));
                BuildDatabase(messEvent->GetDatabase("main-db"));
                if (!!messEvent->GetDatabase("additional-db")) {
                    BuildDatabase(messEvent->GetDatabase("additional-db"));
                }
                return true;
            }
            return false;
        }

    private:
        class TRegularTestChecker: public IObjectInQueue {
        private:
            const TTestWithDatabase* Owner = nullptr;
        public:
            TRegularTestChecker(const TTestWithDatabase* owner)
                    : Owner(owner) {

            }
            virtual void Process(void* /*threadSpecificResource*/) override {
                while (AtomicGet(Owner->GetIsActive())) {
                    Owner->RegularTestCheck();
                    Sleep(TDuration::Seconds(1));
                }
            }
        };

        virtual void DoRegularTestCheck() const {}

        void BuildDatabase(TDatabasePtr db);

        virtual void RegularTestCheck() const final {
            INFO_LOG << LogColorBlue << "CURRENT_SCHEME: " << Singleton<TDatabaseNamespace>()->ToString() << LogColorNo << Endl;
            DoRegularTestCheck();
        }
    };

    template<class TServerGuard>
    class TAbstractTestCase: public TTestWithDatabase {
    private:
        THolder<typename TServerGuard::TConfig> Config;
        THolder<TServerGuard> Server;
        THolder<TSimpleClient> Client;

        bool Initialized = false;

        NRTProc::TAbstractLock::TPtr TestLock;

    public:
        TAbstractTestCase() {
            THistoryConfig::DefaultPingPeriod = TDuration::Seconds(3);
        }

        ~TAbstractTestCase() {
        }

        virtual bool FillSpecialDBFeatures(TConfigGenerator& /*configGenerator*/) const {
            return false;
        }

        void Initialize(TConfigGenerator& configGenerator) {
            NStorage::ITransaction::NeedAssertOnTransactionFail = true;
            Singleton<TConfigurableCheckers>()->SetHistoryParsingAlertsActive(true);
            if (GetEnv("POSTGRES_RECIPE_HOST")) {
                configGenerator.SetDBHost(GetEnv("POSTGRES_RECIPE_HOST"));
                configGenerator.SetDBPort(FromString<ui64>(GetEnv("POSTGRES_RECIPE_PORT")));
                configGenerator.SetDBName(GetEnv("POSTGRES_RECIPE_DBNAME"));
                configGenerator.SetDBUser(GetEnv("POSTGRES_RECIPE_USER"));
            } else {
                CHECK_WITH_LOG(FillSpecialDBFeatures(configGenerator));
            }
            configGenerator.SetDBMainNamespace(TString("t") + ::ToString(Now().Seconds()) + "_" + ToLowerUTF8(Name_).substr(0, 16) + "_" + ::ToString(RandomNumber<ui16>()));
            TFLEventLog::Notice("CURRENT_SCHEMA")("schema", configGenerator.GetDBMainNamespace());

            CHECK_WITH_LOG(!Initialized);
            THistoryConfig::DefaultNeedLock = true;
            Initialized = true;
            ui16 serverPort = 16000;
#ifdef _win_
            configGenerator.SetDaemonPort(8000);
#else
            configGenerator.SetDaemonPort(Singleton<TPortManager>()->GetPort());
            serverPort = Singleton<TPortManager>()->GetPort();
#endif
            configGenerator.SetHomeDir(GetEnv("HOME"));
            if (GetEnv("WorkDir")) {
                configGenerator.SetWorkDir(GetEnv("WorkDir"));
            }
            configGenerator.SetBasePort(serverPort);

            Config = configGenerator.BuildConfig<typename TServerGuard::TConfig>(configGenerator.GetFullConfig());
            try {
                TUnstrictConfig::ToJson(Config->ToString());
            } catch (...) {
                {
                    TFileOutput fo("incorrect_config.txt");
                    fo << Config->ToString();
                }
                S_FAIL_LOG << CurrentExceptionMessage() << Endl;
            }
            Server = MakeHolder<TServerGuard>(*Config);
            Client = MakeHolder<TSimpleClient>(serverPort);

            TVector<NFrontend::TSetting> settings;
            PrepareSettings(settings);
            CHECK_WITH_LOG((*Server)->GetSettings().SetValues(settings, "tester"));
            TVector<TRTBackgroundProcessContainer> rtbgContainers;
            PrepareBackgrounds(rtbgContainers);
            for (auto&& i : rtbgContainers) {
                CHECK_WITH_LOG((*Server)->GetRTBackgroundManager()->GetStorage().ForceUpsertBackgroundSettings(i, "env_initialization"));
            }
            IUserPermissions::TPtr permissions = MakeAtomicShared<TTransparentPermissions>("testing");
            (*Server)->BuildInterfaceConstructor(permissions);

            DoInitialize();
        }

    private:
        virtual void DoInitialize() {}

    protected:
        void AddTagDescription(const TDBTagDescription& descr) {
            auto& server = *GetServerGuard();
            const TTagDescriptionsManager& tagDescriptionsManager = server.GetTagDescriptionsManager();
            NCS::TEntitySession session = tagDescriptionsManager.BuildNativeSession(false);
            tagDescriptionsManager.UpsertTagDescriptions({ descr }, "test", session);
            UNIT_ASSERT(session.Commit());
            tagDescriptionsManager.RefreshCache(Now());
        }

        virtual void PrepareSettings(TVector<NFrontend::TSetting>& settings) {
            Y_UNUSED(settings);
        }

        virtual void PrepareBackgrounds(TVector<TRTBackgroundProcessContainer>& rtbgContainers) {
            Y_UNUSED(rtbgContainers);
        }

        TServerGuard& GetServerGuard() {
            return *Server;
        }

        TSimpleClient& GetServerClient() {
            return *Client;
        }

        TString BuildHandlerConfig(const TString& handlerType, const TString& additionalCgi = "") const {
            TStringBuilder sb;
            sb << "AuthModuleName: fake" << Endl;
            sb << "ProcessorType: " << handlerType << Endl;
            if (additionalCgi) {
                sb << "OverrideCgiPart: " << additionalCgi << Endl;
            }
            return sb;
        }
    };
}
