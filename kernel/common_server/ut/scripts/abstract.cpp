#include "abstract.h"
#include "default_actions.h"
#include <kernel/common_server/server/server.h>


namespace NServerTest {

    class TCurrentInfoWatcher: public IObjectInQueue {
    private:
        const TAtomic* IsActual = nullptr;
        TString Message;
        TRWMutex RWMutex;
    public:
        void SetMessage(const TString& message) {
            TWriteGuard wg(RWMutex);
            Message = message;
        }

        TCurrentInfoWatcher(const TAtomic& isActual)
            : IsActual(&isActual)
        {

        }

        virtual void Process(void* /*threadSpecificResource*/) override {
            while (AtomicGet(*IsActual)) {
                {
                    TReadGuard wg(RWMutex);
                    INFO_LOG << LogColorBlue << "RECORD=CURRENT_ITEM;" << Message << "TIME_HR=" << ModelingNow().ToString() << ";TIMESTAMP=" << ModelingNow().Seconds() << LogColorNo << Endl;
                }
                Sleep(TDuration::Seconds(1));
            }
        }

    };

    TScript::TScript(const TInstant start /*= Now()*/)
        : Start(start)
    {
        CurrentTS = Start;
    }

    bool TScript::Execute(ITestContext& context) {
        TAtomic idx = 0;
        TAtomic activity = 1;
        TString titleLast;
        TAtomic idxLocal = 0;
        THolder<TCurrentInfoWatcher> watcher = MakeHolder<TCurrentInfoWatcher>(activity);
        {
            TThreadPool tp;
            tp.Start(1);
            tp.SafeAdd(watcher.Get());
            try {
                for (auto&& i : Actions) {
                    for (auto&& e : i.second) {
                        TStringBuilder sbTitle;
                        if (e->GetTitle()) {
                            sbTitle << "CURRENT_ITEM=" << e->GetTitle() << ";";
                            titleLast = e->GetTitle();
                            AtomicSet(idxLocal, 0);
                        } else if (!!titleLast) {
                            sbTitle << "CURRENT_SECTION=" << titleLast << ";";
                            sbTitle << "LOCAL_STEP=" << AtomicIncrement(idxLocal) << ";";
                        }
                        sbTitle << "GLOBAL_STEP=" << AtomicIncrement(idx) << ";";
                        watcher->SetMessage(sbTitle);
                        INFO_LOG << LogColorGreen << sbTitle << LogColorNo << Endl;
                        try {
                            e->Execute(context);
                            INFO_LOG << LogColorGreen << "Finished item: " << ": " << AtomicGet(idx) << " : " << LogColorNo << Endl;
                        } catch (NUnitTest::TAssertException& assertEx) {
                            ythrow assertEx;
                        } catch (...) {
                            ERROR_LOG << LogColorRed << "Failed item: " << ": " << AtomicGet(idx) << " : " << CurrentExceptionMessage() << LogColorNo << Endl;
                            return false;
                        }
                    }
                }
            } catch (...) {
                ERROR_LOG << CurrentExceptionMessage() << Endl;
            }
            AtomicSet(activity, 0);
        }
        return true;
    }

    void TScript::AdvanceTime(const TDuration& tsDiff) {
        INFO_LOG << LogColorGreen << "ADVANCE THE TIME FORWARD BY: " << tsDiff.ToString() << LogColorNo << Endl;
        CurrentTS += tsDiff;
    }

    ITestContext::ITestContext(IServerGuard& server, const TSimpleClient& client)
        : Client(client)
        , Server(server)
    {
        NExternalAPI::TSenderConfig config;
        config.SetPort(server.GetAs<TBaseServer>().GetConfig().GetHttpServerOptions().Port);
        config.SetHost("127.0.0.1");
        config.MutableRequestConfig().SetMaxAttempts(1).SetGlobalTimeout(TDuration::Minutes(1));
        ServerClient.Reset(new NExternalAPI::TSender(config, ""));
    }

}
