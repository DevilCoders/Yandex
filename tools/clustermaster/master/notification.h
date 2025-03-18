#pragma once

#include "recipients.h"
#include "tryvar.h"

#include <tools/clustermaster/common/async_jobs.h>

#include <library/cpp/tvmauth/client/facade.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/thread/pool.h>

namespace NAsyncJobs {

struct TSendUserNotifyJob: TAsyncJobs::IJob {
    enum EType {
        FAILURE,
        DEPFAILURE
    };

    class TTaskWithWorker {
    public:
        TTaskWithWorker(int task, const TString& worker)
            : Task(task)
            , Worker(worker)
        {
        }

        int GetTask() const {
            return Task;
        }

        const TString& GetWorker() const {
            return Worker;
        }

    private:
        int Task;
        TString Worker;
    };

    class TTargetWithTasks {
    public:
        TTargetWithTasks(const TString& target, const TVector<TTaskWithWorker>& tasks)
            : Target(target)
            , Tasks(tasks)
        {
            Y_VERIFY(!tasks.empty());
        }

        const TString& GetTarget() const {
            return Target;
        }

        const TVector<TTaskWithWorker>& GetTasks() const {
            return Tasks;
        }

        const TTaskWithWorker& GetOnlyTask() const {
            Y_VERIFY(Tasks.size() == 1);
            return Tasks[0];
        }

        TVector<int> GetTasksIds() const {
            TVector<int> result;
            for (TVector<TTaskWithWorker>::const_iterator i = Tasks.begin(); i != Tasks.end(); ++i) {
                result.push_back(i->GetTask());
            }
            return result;
        }

    private:
        TString Target;
        TVector<TTaskWithWorker> Tasks;
    };

    const EType Type;
    const TRecipients Recipients;

    const TTargetWithTasks Target;
    const TMaybe<TTargetWithTasks> CauseTarget;

    TSendUserNotifyJob(EType type, const TRecipients& recipients, const TTargetWithTasks& target, const TMaybe<TTargetWithTasks>& causeTarget, TVariablesMap variables)
        : Type(type)
        , Recipients(recipients)
        , Target(target)
        , CauseTarget(causeTarget)
        , Variables(variables)
    {
        bool isDepfailure = (type == DEPFAILURE);
        Y_VERIFY(isDepfailure == causeTarget.Defined());
    }

    TString GetInstance();
    TString GetShortDescription();

    virtual TString GetTitle() = 0;
    virtual TString GetMessage() = 0;
    virtual void Process(void*) = 0;

protected:
    TVariablesMap Variables;
};

struct TSendMailJob: public TSendUserNotifyJob {
public:
    TSendMailJob(EType type, const TRecipients& recipients, const TTargetWithTasks& target, const TMaybe<TTargetWithTasks>& causeTarget, TVariablesMap variables)
        : TSendUserNotifyJob(type, recipients, target, causeTarget, variables)
    {
    }

    TString GetTitle() override;
    TString GetMessage() override;
    void Process(void*) override;
};

struct TSendSmsJob: public TSendUserNotifyJob {
public:
    TSendSmsJob(EType type, const TRecipients& recipients, const TTargetWithTasks& target, const TMaybe<TTargetWithTasks>& causeTarget, TVariablesMap variables, std::shared_ptr<NTvmAuth::TTvmClient> tvmClient)
        : TSendUserNotifyJob(type, recipients, target, causeTarget, variables)
        , TvmClient(std::move(tvmClient))
    {
    }

    TString GetPhoneByLogin(const TString& login);
    void SendSms(const TString& phoneNumber);

    TString GetTitle() override;
    TString GetMessage() override;
    void Process(void*) override;

private:
    std::shared_ptr<NTvmAuth::TTvmClient> TvmClient;
};


struct TSendTelegramJob: public TSendUserNotifyJob {
public:
    TSendTelegramJob(EType type, const TRecipients& recipients, const TTargetWithTasks& target, const TMaybe<TTargetWithTasks>& causeTarget, TVariablesMap variables, const TString& telegramSecret)
        : TSendUserNotifyJob(type, recipients, target, causeTarget, variables)
        , TelegramSecret(telegramSecret)
    {
    }

    TString GetTitle() override;
    TString GetMessage() override;
    void Process(void*) override;

private:
    TString TelegramSecret;
};


struct TSendJNSChannelJob: public TSendUserNotifyJob {
public:
    TSendJNSChannelJob(EType type, const TRecipients& recipients, const TTargetWithTasks& target,
                       const TMaybe<TTargetWithTasks>& causeTarget, TVariablesMap variables, const TString& jnsToken)
        : TSendUserNotifyJob(type, recipients, target, causeTarget, variables)
        , Token(jnsToken)
    {
    }

    TString GetTitle() override;
    TString GetMessage() override;
    void Process(void*) override;

private:
    TString Token;
};


struct TPushJugglerEventJob: public TSendUserNotifyJob {
public:
    TPushJugglerEventJob(EType type, const TRecipients& recipients, const TTargetWithTasks& target, const TMaybe<TTargetWithTasks>& causeTarget, TVariablesMap variables)
        : TSendUserNotifyJob(type, recipients, target, causeTarget, variables)
    {
    }

    TString GetTitle() override;
    TString GetMessage() override;
    void ProcessJugglerResponse(TStringStream* response);
    void Process(void*) override;
};

} // NAsyncJobs
