#pragma once

#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/searchserver/simple/context/replier.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/coded_exception.h>

#include <util/datetime/base.h>
#include <util/generic/cast.h>
#include <util/system/spinlock.h>

class TRegExMatch;

class TIncorrectFinishRequestMessage: public NMessenger::IMessage {
public:
    TIncorrectFinishRequestMessage() = default;
};

class IFrontendReportBuilder: public NCS::NLogging::TBaseLogsAccumulator {
public:
    struct TCtx {
        IReplyContext::TPtr Context;
        const TRegExMatch* AccessControlAllowOrigin;
        TString Handler;

        explicit operator bool() const {
            return !!Context;
        }
    };
    using TPtr = TAtomicSharedPtr<IFrontendReportBuilder>;

private:
    CSA_READONLY_DEF(TString, HandlerName);
    CSA_MAYBE(IFrontendReportBuilder, ui32, FinishedCode);

private:
    TAdaptiveLock Lock;
    TAtomic FinishFlag = 0;

protected:
    IReplyContext::TPtr Context;
    const TRegExMatch* AccessControlAllowOrigin = nullptr;
    TSignalTagsSet Tags;

protected:
    virtual void DoAddEvent(TInstant timestamp, const NCS::NLogging::TBaseLogRecord& e) = 0;
    virtual void DoFinish(const TCodedException& e) = 0;
    virtual void DoSetError(const TString& errorInfo) = 0;

    auto MakeThreadSafeGuard() const {
        return Guard(Lock);
    }

    virtual void DoAddRecord(const NCS::NLogging::TBaseLogRecord& r) override final;
public:
    IFrontendReportBuilder(IReplyContext::TPtr context, const TString& processor);
    IFrontendReportBuilder(const TCtx& ctx);
    virtual ~IFrontendReportBuilder();

    IReplyContext::TPtr GetContextPtr() {
        return Context;
    }

    virtual TString GetClassName() const override {
        return "request_event_log";
    }

    IFrontendReportBuilder& SetError(const TString& errorInfo) {
        {
            auto g = MakeThreadSafeGuard();
            DoSetError(errorInfo);
        }
        return *this;
    }

    TSignalTagsSet& AddSignalTags(const TString& name, const TString& value) {
        return Tags.AddTag(name, value);
    }

    void Finish(const int code) {
        Finish(TCodedException(code));
    }

    void Finish(const TCodedException& e);

public:
    class TGuard: public TNonCopyable {
    private:
        IFrontendReportBuilder::TPtr Report;
        TAtomic Flushed = 0;
        TCodedException ErrorsInfo = TCodedException(500);
        TFLEventLogGuard EvLogGuard;
    public:
        TGuard(IFrontendReportBuilder::TPtr report, const int code);
        ~TGuard();

        IFrontendReportBuilder::TPtr Release();

        IFrontendReportBuilder::TPtr GetReport() {
            return Report;
        }

        template <class T>
        T& MutableReportAs() {
            return *VerifyDynamicCast<T*>(Report.Get());
        }

        int GetCode() const {
            return ErrorsInfo.GetCode();
        }

        void SetCode(const TCodedException& e) {
            ErrorsInfo = e;
        }

        void SetCode(const int code) {
            ErrorsInfo.SetCode(code);
        }

        const TCodedException& GetErrors() const {
            return ErrorsInfo;
        }

        TCodedException& MutableErrors() {
            return ErrorsInfo;
        }

    private:
        void Flush();
    };
};
