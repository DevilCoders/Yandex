#pragma once

//#include <kernel/common_server/library/sharding/sharding.h>
#include <library/cpp/logger/global/global.h>
#include <kernel/common_server/library/tasks_graph/script.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/object_factory/object_factory.h>
#include <util/string/cast.h>
#include <util/generic/string.h>

namespace NDaemonController {

    enum TAsyncPolicy { apNoAsync, apStartAndWait, apStart, apWait };
    enum THttpMethod { hmAuto, hmGet, hmPost, hmPut, hmDelete };

    class TAction {
    public:
        enum TStatus { asNotStarted, asInProgress, asFail, asSuccess };
        enum TLockType { ltNoLock, ltReadLock, ltWriteLock };
        typedef NObjectFactory::TObjectFactory<TAction, TString> TFactory;
    protected:

        virtual TString DoBuildCommand() const = 0;
        virtual void DoInterpretResult(const TString& result) = 0;
        virtual NJson::TJsonValue DoSerializeToJson() const = 0;
        virtual void DoDeserializeFromJson(const NJson::TJsonValue& json) = 0;
        virtual bool GetPostContent(TString& /*result*/) const {
            return false;
        }

        virtual TString GetCustomUri() const {
            return TString();
        }

        mutable TString Info;
        mutable TStatus Status;
        TString Parent;
        mutable TInstant TimeStart;
        mutable TInstant TimeFinish;
        TString AdditionParams;
    public:

        typedef TAtomicSharedPtr<TAction> TPtr;

        TAction() {
            Status = asNotStarted;
        }

        virtual bool IsLogicAction() const {
            return false;
        }

        virtual TString GetCurrentInfo() const;

        virtual TString ActionId() const {
            return ActionName();
        }

        virtual TString ActionName() const = 0;

        void SetParent(const TString& parent) {
            Parent = parent;
        }

        virtual ~TAction() {};

        virtual TLockType GetLockType() const {
            return ltNoLock;
        }

        virtual NJson::TJsonValue SerializeExecutionInfo() const;
        virtual NJson::TJsonValue Serialize() const;
        virtual void DeserializeExecutionInfo(const NJson::TJsonValue& value);
        virtual void Deserialize(const NJson::TJsonValue& value);

        virtual void AddPrevActionsResult(const NRTYScript::ITasksInfo& /*info*/) {}

        static TAction::TPtr BuildAction(const NJson::TJsonValue& value);

        virtual ui32 GetAttemptionsMaxResend() const {
            return 5;
        }

        virtual TDuration GetTimeoutDuration() const {
            return TDuration::Seconds(1);
        }

        bool IsStarted() const {
            return (Status != asNotStarted);
        }

        bool IsFinished() const {
            return (Status == asFail) || (Status == asSuccess);
        }

        virtual bool IsFailed() const;

        virtual TAsyncPolicy AsyncPolicy() const {
            return apNoAsync;
        }

        TString BuildCommand(TString& postData, const TString& uriPrefix) const;

        virtual THttpMethod GetHttpMethod() const {
            return hmAuto;
        }

        void InterpretResult(const TString& result) {
            try {
                DoInterpretResult(result);
            } catch (...) {
                Fail("Exception on interpret result: \"" + result + "\": " + CurrentExceptionMessage());
            }
        }

        virtual void Fail(const TString& info) const;
        virtual void Success(const TString& info);

        TString GetInfo() const;

        void SetAdditionParams(const TString& params) {
            AdditionParams = "&" + params;
        }

    };

    class TSimpleAsyncAction : public TAction {
    protected:
        enum TActionState {
            asStarted     = 0x01,
            asStarting    = 0x02,
            asWaiting     = 0x04,
            asWaitTimeout = 0x08,
            asSuspended   = 0x10,
        };
        mutable ui32 ActionState = 0;
        mutable TInstant TimeWaitStart;
        TAsyncPolicy Policy;
        TString IdTask;
        TString WaitActionName;
        TRWMutex RWMutex;
    public:

        TSimpleAsyncAction() {
            Policy = apStartAndWait;
        }

        TSimpleAsyncAction(TAsyncPolicy policy) {
            VERIFY_WITH_LOG(policy != apWait, "do not use apWait without waitActionName");
            VERIFY_WITH_LOG(policy != apNoAsync, "it is SPARTA! no apNoAsync in async task, use apStartAndWait if you very want it");
            Policy = policy;
        }

        TSimpleAsyncAction(const TString& waitActionName) {
            Policy = apWait;
            SetFlag(asStarted);
            WaitActionName = waitActionName;
        }

        void SetFlag(ui32 flag, ui32 mask = 0) const {
            ActionState &= ~mask;
            ActionState |= flag;
        }

        bool HasFlag(ui32 flag) const {
            return (ActionState & flag) == flag;
        }

        virtual bool GetNotContinuableTaskOnStarting() const {
            return true;
        }

        void SetAsyncPolicy(TAsyncPolicy policy) {
            Policy = policy;
        }

        virtual TAsyncPolicy AsyncPolicy() const override {
            return Policy;
        }

        virtual TString ActionId() const override {
            return ActionName() + "(" + ToString(Policy) + ")";
        }

        const TString& GetIdTask() const {
            return IdTask;
        }

        void SetIdTask(const TString& idTask) {
            IdTask = idTask;
        }

        virtual void AddPrevActionsResult(const NRTYScript::ITasksInfo& info) override;

        virtual NJson::TJsonValue Serialize() const override {
            NJson::TJsonValue result = TAction::Serialize();
            result.InsertValue("action_state", ActionState);
            result.InsertValue("async_policy", ToString(Policy));
            if (HasFlag(asWaiting))
                result.InsertValue("time_wait_start", TimeWaitStart.GetValue());
            {
                TReadGuard g(RWMutex);
                if (!!IdTask) {
                    result.InsertValue("id_task", IdTask);
                }
            }
            if (!!WaitActionName)
                result.InsertValue("wait_action_name", WaitActionName);
            return result;
        }

        virtual TDuration GetWaitTimeoutDuration() const {
            return TDuration::Minutes(100);
        }

        virtual void Deserialize(const NJson::TJsonValue& value) override {
            NJson::TJsonValue::TMapType map;
            CHECK_WITH_LOG(value.GetMap(&map));
            CHECK_WITH_LOG(ActionState == 0);
            SetFlag(map["action_state"].GetUIntegerRobust(), asStarting | asStarted | asSuspended);
            if (HasFlag(asWaiting))
                TimeWaitStart = TInstant::MicroSeconds(map["time_wait_start"].GetUIntegerRobust());
            Policy = FromString<TAsyncPolicy>(map["async_policy"].GetStringRobust());
            NJson::TJsonValue::TMapType::const_iterator i = map.find("id_task");
            if (i != map.end())
                IdTask = i->second.GetStringRobust();
            i = map.find("wait_action_name");
            if (i != map.end())
                WaitActionName = i->second.GetStringRobust();
            TAction::Deserialize(value);
        }

        virtual TString DoBuildCommandStart() const = 0;
        virtual TString DoBuildCommandWait() const = 0;
        virtual void DoInterpretResultStart(const TString& result) = 0;
        virtual void DoInterpretResultWait(const TString& result) = 0;
        virtual bool GetPostContentStart(TString& /*result*/) const {
            return false;
        }
        virtual bool GetPostContentWait(TString& /*result*/) const {
            return false;
        }
        virtual THttpMethod GetHttpMethodStart() const {
            return hmAuto;
        }
        virtual THttpMethod GetHttpMethodWait() const {
            return hmAuto;
        }
        virtual TString GetCustomUriStart() const {
            return TString();
        }
        virtual TString GetCustomUriWait() const {
            return TString();
        }

        virtual bool GetPostContent(TString& result) const override {
            if (HasFlag(asStarting))
                return GetPostContentStart(result);
            return GetPostContentWait(result);
        }
        virtual THttpMethod GetHttpMethod() const override {
            if (HasFlag(asStarting))
                return GetHttpMethodStart();
            return GetHttpMethodWait();
        }

        virtual TString GetCustomUri() const override {
            if (HasFlag(asStarting))
                return GetCustomUriStart();
            return GetCustomUriWait();
        }

        virtual TString DoBuildCommand() const override {
            CHECK_WITH_LOG(!IsFinished());
            if (HasFlag(asStarting) && GetNotContinuableTaskOnStarting()) {
                ythrow yexception() << "Cannot proceed with a task stuck at STARTING";
            }
            if (!HasFlag(asStarted)) {
                CHECK_WITH_LOG(!HasFlag(asStarting) || !GetNotContinuableTaskOnStarting());
                SetFlag(asStarting);
                return DoBuildCommandStart() + (HasFlag(asSuspended) ? "&suspended=1" : "") + "&timeout=" + GetWaitTimeoutDuration().ToString();
            } else {
                if (!HasFlag(asWaiting))
                    TimeWaitStart = Now();
                SetFlag(asWaiting);
                return DoBuildCommandWait();
            }
        }

        virtual void DoInterpretResult(const TString& result) override {
            if (HasFlag(asStarting)) {
                SetFlag(0, asStarting);
                DoInterpretResultStart(result);
                SetFlag(asStarted);
                TimeWaitStart = Now();
            } else {
                CHECK_WITH_LOG(HasFlag(asStarted | asWaiting));
                DoInterpretResultWait(result);
                if (!IsFinished() && (Now() - TimeWaitStart > GetWaitTimeoutDuration())) {
                    DEBUG_LOG << "Action " + ActionId() + " waiting timeout: " << TimeWaitStart << " + " << GetWaitTimeoutDuration() << " > " << Now() << Endl;
                    SetFlag(asWaitTimeout);
                    Fail("Timeouted: " + ToString(GetWaitTimeoutDuration()) + "/" + ToString(TimeWaitStart));
                }
            }
        }

        virtual TDuration GetTimeoutDuration() const override {
            return TDuration::Seconds(10);
        }

        virtual ui32 GetAttemptionsMaxResendStart() const {
            return 1;
        }

        virtual ui32 GetAttemptionsMaxResendWait() const {
            return Max<ui32>();
        }

        virtual ui32 GetAttemptionsMaxResend() const override {
            return HasFlag(asStarted) ? GetAttemptionsMaxResendWait() : GetAttemptionsMaxResendStart();
        }

        void SetSuspended(bool suspended) {
            SetFlag(suspended ? asSuspended : 0, asSuspended);
        }
    };

    class IControllerAgentCallback {
    public:

        struct TActionContext {
            TString Host;
            ui32 Port;
            TString UriPrefix;
            TActionContext(const TString& host, ui16 port, const TString& uriPrefix) {
                Host = host;
                Port = port;
                UriPrefix = uriPrefix;
            }
            TString ToString() const {
                return Host + ":" + ::ToString(Port) + "/" + UriPrefix;
            }

        };

        virtual void OnAfterActionStep(const TActionContext& context, NDaemonController::TAction& action) = 0;
        virtual void OnBeforeActionStep(const TActionContext& context, NDaemonController::TAction& action) = 0;
        virtual bool IsExecutable() const { return true; }
        virtual ~IControllerAgentCallback() {}
    };

    class TControllerAgent: public IControllerAgentCallback::TActionContext {
    public:
        class IRequestWaiter {
        public:
            virtual ~IRequestWaiter() {}
            virtual bool WaitResult(TString& result) = 0;
            typedef TSimpleSharedPtr<IRequestWaiter> TPtr;
        };

    private:
        IControllerAgentCallback* Callback;
    public:
        TControllerAgent(const TString& host, ui16 port, IControllerAgentCallback* callback = nullptr, const TString& uriPrefix = TString())
            : IControllerAgentCallback::TActionContext(host, port, uriPrefix)
        {
            Callback = callback;
        }

        bool ExecuteCommand(const TString& command, TString& result, ui32 connectionTimeoutMs, ui32 sendAttemptionsMax, const TString& postData, THttpMethod method = hmAuto);
        IRequestWaiter::TPtr ExecuteCommand(const TString& command, const TString& postData, TDuration connectionTimeout, TDuration socketTimeout, THttpMethod method = hmAuto);

        bool ExecuteAction(TAction& action) {
            if (action.IsFinished())
                return true;

            TString result;
            TString postData;
            THttpMethod method = action.GetHttpMethod();
            CHECK_WITH_LOG(!action.IsLogicAction() || action.AsyncPolicy() == apNoAsync);

            if (action.AsyncPolicy() == apNoAsync) {
                if (Callback)
                    Callback->OnBeforeActionStep(*this, action);
                if (!action.IsLogicAction()) {
                    TString command = action.BuildCommand(postData, UriPrefix);
                    if (!action.IsFinished()) {
                        if (ExecuteCommand(command, result, action.GetTimeoutDuration().MilliSeconds(), action.GetAttemptionsMaxResend(), postData, method)) {
                            action.InterpretResult(result);
                            if (Callback)
                                Callback->OnAfterActionStep(*this, action);
                            return true;
                        } else {
                            action.Fail(result);
                            if (Callback)
                                Callback->OnAfterActionStep(*this, action);
                            return false;
                        }
                    } else
                        return !action.IsFailed();
                } else {
                    if (!action.IsFinished()) {
                        action.InterpretResult(result);
                        INFO_LOG << "Logic action processed: " << action.ActionId() << Endl;
                        if (Callback)
                            Callback->OnAfterActionStep(*this, action);
                        return true;
                    } else
                        return !action.IsFailed();
                }
            }

            if (action.AsyncPolicy() == apStart || action.AsyncPolicy() == apStartAndWait) {
                if (Callback)
                    Callback->OnBeforeActionStep(*this, action);
                TString command = action.BuildCommand(postData, UriPrefix);
                if (!action.IsFinished()) {
                    if (ExecuteCommand(command, result, action.GetTimeoutDuration().MilliSeconds(), action.GetAttemptionsMaxResend(), postData, method)) {
                        action.InterpretResult(result);
                        if (Callback)
                            Callback->OnAfterActionStep(*this, action);
                    } else {
                        action.Fail(result);
                        if (Callback)
                            Callback->OnAfterActionStep(*this, action);
                        return false;
                    }
                } else
                    return !action.IsFailed();
            }

            if (action.AsyncPolicy() == apWait || action.AsyncPolicy() == apStartAndWait) {
                if (Callback && !action.IsFinished())
                    Callback->OnBeforeActionStep(*this, action);
                while (!action.IsFinished()) {
                    TString command = action.BuildCommand(postData, UriPrefix);
                    if (!action.IsFinished()) {
                        if (ExecuteCommand(command, result, action.GetTimeoutDuration().MilliSeconds(), action.GetAttemptionsMaxResend(), postData, method)) {
                            action.InterpretResult(result);
                        } else {
                            if (!Callback || Callback->IsExecutable()) {
                                action.Fail(result);
                                if (Callback)
                                    Callback->OnAfterActionStep(*this, action);
                            }
                            return false;
                        }
                        if (!action.IsFinished())
                            Sleep(TDuration::MilliSeconds(500));
                    }
                }
                if (Callback)
                    Callback->OnAfterActionStep(*this, action);
            }
            return true;
        }
    };

    TString ASToString(const TAction::TStatus& status);
    TAction::TStatus ASFromString(const TString& status);

}

template <>
NDaemonController::TAsyncPolicy FromStringImpl<NDaemonController::TAsyncPolicy, char>(const char* data, size_t len);
