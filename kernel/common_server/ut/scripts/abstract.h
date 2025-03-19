#pragma once

#include <library/cpp/testing/unittest/registar.h>
#include <kernel/common_server/ut/server.h>
#include <kernel/common_server/ut/const.h>
#include <kernel/common_server/abstract/common.h>
#include <kernel/common_server/util/instant_model.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

namespace NServerTest {

    class ITestContext: TNonCopyable {
        CS_ACCESS(ITestContext, TString, UserId, DefaultUserId);

    private:
        const TSimpleClient& Client;
        THolder<NExternalAPI::TSender> ServerClient;
        THolder<TInstantGuard> InstantGuard;
        IServerGuard& Server;

    public:
        ITestContext(IServerGuard& server, const TSimpleClient& client);

        const NExternalAPI::TSender& GetServerClient() const {
            return *ServerClient;
        }

        virtual ~ITestContext() {
        }

        NNeh::THttpRequest CreateRequest(const TString& uri, const NJson::TJsonValue& post = NJson::JSON_NULL, const TString& cgi = "") const {
            NNeh::THttpRequest request;
            request.SetUri(uri).SetPostData(TBlob::FromString(post.GetStringRobust()));
            if (cgi) {
                request.SetCgiData(cgi);
            }
            if (!post.IsNull()) {
                DEBUG_LOG << post.GetStringRobust() << Endl;
                request.SetRequestType("POST");
                request.SetPostData(post.GetStringRobust());
            }
            request.AddHeader("Authorization", GetUserId());
            return request;
        }

        NJson::TJsonValue AskServer(const NNeh::THttpRequest& request, const TInstant deadline, bool parseBadReplies = false) const {
            return Client.AskServer(request, deadline, parseBadReplies);
        }

        ITestContext& SetInstantGuard(const TInstant value) {
            if (!InstantGuard) {
                InstantGuard.Reset(new TInstantGuard(value));
            }
            InstantGuard->Set(value);
            return *this;
        }

        template <class T>
        const T& GetServer() const {
            return Server.GetAs<T>();
        }

        template <class T>
        T& GetAs() {
            return *VerifyDynamicCast<T*>(this);
        }
    };

    class ITestAction {
        CSA_DEFAULT(ITestAction, TInstant, StartInstant);
        CS_ACCESS(ITestAction, bool, ExpectOK, true);
        CS_ACCESS(ITestAction, TDuration, Timeout, TDuration::Minutes(1));
        CSA_DEFAULT(ITestAction, TString, Title);
        CSA_MAYBE(ITestAction, bool, TransactionFailable);
    public:
        ITestAction& SetTimeoutS(const ui32 seconds) {
            Timeout = TDuration::Seconds(seconds);
            return *this;
        }

        using TPtr = TAtomicSharedPtr<ITestAction>;

        ITestAction(TInstant startInstant = TInstant::Zero())
            : StartInstant(startInstant)
        {
        }

        virtual ~ITestAction() = default;

        virtual void Execute(ITestContext& context) {
            const bool baseTransactionFailable = NStorage::ITransaction::NeedAssertOnTransactionFail;
            if (!!TransactionFailable) {
                NStorage::ITransaction::NeedAssertOnTransactionFail = !*TransactionFailable;
            }
            try {
                if (StartInstant) {
                    INFO_LOG << LogColorBlue << "TIME INITIATION: " << StartInstant.ToString() << " / " << StartInstant.Seconds() << LogColorNo << Endl;
                    context.SetInstantGuard(StartInstant);
                }
                DoExecute(context);
            } catch (...) {
                TFLEventLog::Error("exception in script")("message", CurrentExceptionMessage());
            }
            if (!!TransactionFailable) {
                NStorage::ITransaction::NeedAssertOnTransactionFail = baseTransactionFailable;
            }
        }

    protected:
        virtual void DoExecute(ITestContext& context) = 0;
    };

    class TScript {
    private:
        TInstant Start;
        TInstant CurrentTS;

        TMap<TInstant, TVector<ITestAction::TPtr>> Actions;

    public:
        TScript(const TInstant start = Now());

        template <class T, typename... Args>
        T& Add(Args... args) {
            THolder<T> result(new T(CurrentTS, args...));
            Actions[CurrentTS].emplace_back(result.Get());
            return *result.Release();
        }

        template <class T, typename... Args>
        T& AddMove(const TDuration tsDiff, Args... args) {
            CurrentTS = CurrentTS + tsDiff;
            return Add<T>(args...);
        }

        template <class T, typename... Args>
        T& AddTitle(const TString& title, Args... args) {
            auto& result = Add<T>(args...);
            result.SetTitle(title);
            return result;
        }

        bool Execute(ITestContext& context);

        void AdvanceTime(const TDuration& tsDiff);
    };
}
