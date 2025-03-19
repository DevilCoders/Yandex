#include "session.h"

#include "config.h"

#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <util/generic/ptr.h>
#include <util/system/mutex.h>

namespace NCloud::NFileStore::NClient {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_DECLARE_METHOD(name, ...)                                    \
    struct T##name##Method                                                     \
    {                                                                          \
        using TRequest = NProto::T##name##Request;                             \
        using TResponse = NProto::T##name##Response;                           \
                                                                               \
        template <typename T, typename ...TArgs>                               \
        static TFuture<TResponse> Execute(T& client, TArgs&& ...args)          \
        {                                                                      \
            return client.name(std::forward<TArgs>(args)...);                  \
        }                                                                      \
    };                                                                         \
// FILESTORE_DECLARE_METHOD

FILESTORE_SERVICE(FILESTORE_DECLARE_METHOD)

#undef FILESTORE_DECLARE_METHOD

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct TRequestState
    : public TAtomicRefCount<TRequestState<T>>
{
    TCallContextPtr CallContext;
    std::shared_ptr<typename T::TRequest> Request;
    TPromise<typename T::TResponse> Response;

    // session context
    TString SessionId;

    TRequestState(
            TCallContextPtr callContext,
            std::shared_ptr<typename T::TRequest> request,
            TPromise<typename T::TResponse> promise = NewPromise<typename T::TResponse>())
        : CallContext(std::move(callContext))
        , Request(std::move(request))
        , Response(std::move(promise))
    {}
};

template <typename T>
using TRequestStatePtr = TIntrusivePtr<TRequestState<T>>;

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void ExtractResponse(TFuture<T>& future, T& response)
{
    try {
        response = future.ExtractValue();
    } catch (const TServiceError& e) {
        auto& error = *response.MutableError();
        error.SetCode(e.GetCode());
        error.SetMessage(e.what());
    } catch (...) {
        auto& error = *response.MutableError();
        error.SetCode(E_FAIL);
        error.SetMessage(CurrentExceptionMessage());
    }
}

TString GetSessionId(const NProto::TCreateSessionResponse& response)
{
    Y_VERIFY_DEBUG(!response.HasError());
    return response.GetSession().GetSessionId();
}

TResultOrError<TString> TryGetSessionId(
    const TFuture<NProto::TCreateSessionResponse>& future)
{
    Y_VERIFY_DEBUG(future.HasValue());
    try {
        const auto& response = future.GetValue();
        if (!HasError(response)) {
            return GetSessionId(response);
        }

        return response.GetError();
    } catch (const TServiceError& e) {
        return MakeError(e.GetCode(), e.what());
    } catch (...) {
        return MakeError(E_FAIL, CurrentExceptionMessage());
    }
}

////////////////////////////////////////////////////////////////////////////////

class TSession final
    : public ISession
    , public std::enable_shared_from_this<TSession>
{
    enum ESessionState
    {
        Idle,
        SessionRequested,
        SessionEstablished,
        SessionDestroying,
        SessionBroken,
    };

    enum ECreateSessionFlags
    {
        F_NONE = 0,
        F_RESTORE_SESSION = 1,
        F_FORCE_RECOVER = 2,
        F_TIMEOUT = 4,
    };

private:
    const ILoggingServicePtr Logging;
    const ITimerPtr Timer;
    const ISchedulerPtr Scheduler;
    const IFileStoreServicePtr Client;
    const TSessionConfigPtr Config;

    const TString FsTag;
    TLog Log;

    TMutex SessionLock;
    ESessionState SessionState = Idle;
    TFuture<NProto::TCreateSessionResponse> SessionResponse;

    std::atomic_flag PingScheduled = false;

public:
    TSession(
            ILoggingServicePtr logging,
            ITimerPtr timer,
            ISchedulerPtr scheduler,
            IFileStoreServicePtr client,
            TSessionConfigPtr config)
        : Logging(std::move(logging))
        , Timer(std::move(timer))
        , Scheduler(std::move(scheduler))
        , Client(std::move(client))
        , Config(std::move(config))
        , FsTag(Sprintf("[f:%s][c:%s]",
            Config->GetFileSystemId().Quote().c_str(),
            Config->GetClientId().Quote().c_str()))
    {
        Log = Logging->CreateLog("NFS_CLIENT");
    }

    TFuture<NProto::TCreateSessionResponse> CreateSession() override
    {
        return CreateOrRestoreSession({}, F_RESTORE_SESSION);
    }

    TFuture<NProto::TDestroySessionResponse> DestroySession() override
    {
        auto callContext = MakeIntrusive<TCallContext>();

        auto request = std::make_shared<NProto::TDestroySessionRequest>();

        auto state = MakeIntrusive<TRequestState<TDestroySessionMethod>>(
            std::move(callContext),
            std::move(request));

        ExecuteRequest(state);
        return state->Response;
    }

    TFuture<NProto::TPingSessionResponse> PingSession()
    {
        auto callContext = MakeIntrusive<TCallContext>();

        auto request = std::make_shared<NProto::TPingSessionRequest>();

        auto state = MakeIntrusive<TRequestState<TPingSessionMethod>>(
            std::move(callContext),
            std::move(request));

        ExecuteRequest(state);
        return state->Response;
    }

    //
    // Session methods
    //

#define FILESTORE_IMPLEMENT_METHOD(name, ...)                                  \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) override            \
    {                                                                          \
        auto state = MakeIntrusive<TRequestState<T##name##Method>>(            \
            std::move(callContext),                                            \
            std::move(request));                                               \
                                                                               \
        ExecuteRequest(state);                                                 \
        return state->Response;                                                \
    }                                                                          \
// FILESTORE_IMPLEMENT_METHOD

FILESTORE_DATA_METHODS(FILESTORE_IMPLEMENT_METHOD)

#undef FILESTORE_IMPLEMENT_METHOD

    //
    // Forwarded methods
    //

#define FILESTORE_IMPLEMENT_METHOD(name, ...)                                  \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) override            \
    {                                                                          \
        return ForwardRequest<T##name##Method>(                                \
            std::move(callContext),                                            \
            std::move(request));                                               \
    }                                                                          \

FILESTORE_SESSION_FORWARD(FILESTORE_IMPLEMENT_METHOD)

#undef FILESTORE_IMPLEMENT_METHOD

private:

    TFuture<NProto::TCreateSessionResponse> CreateOrRestoreSession(
        const TString& sessionId,
        int flags)
    {
        // need an allocation up front to keep atomicy between inflight requests restoring session
        auto promise = NewPromise<NProto::TCreateSessionResponse>();
        with_lock (SessionLock) {
            const bool forceRecover = (SessionState == SessionRequested)
                && (flags & F_FORCE_RECOVER);
            const bool recoverCurrent = (SessionState == SessionEstablished) &&
                (sessionId == GetSessionId(SessionResponse.GetValue()));

            if (SessionState != Idle && !forceRecover && !recoverCurrent) {
                STORAGE_DEBUG(LogTag(sessionId) << " not recreating session flags: "
                    << (ui32)flags);

                return MakeFuture<NProto::TCreateSessionResponse>(
                    TErrorResponse(E_INVALID_STATE, "invalid session state"));
            }

            SessionState = SessionRequested;
            SessionResponse = promise;
        }

        STORAGE_INFO(LogTag(sessionId) << " initiating session f: " << (ui32)flags);

        auto callContext = MakeIntrusive<TCallContext>();

        auto request = std::make_shared<NProto::TCreateSessionRequest>();
        if (flags & F_RESTORE_SESSION) {
            request->SetRestoreClientSession(true);
        }

        auto state = MakeIntrusive<TRequestState<TCreateSessionMethod>>(
            std::move(callContext),
            std::move(request),
            std::move(promise));

        state->SessionId = sessionId;
        if (Config->GetSessionRetryTimeout() && (flags & F_TIMEOUT)) {
            auto weak_ptr = weak_from_this();
            Scheduler->Schedule(
                Timer->Now() + Config->GetSessionRetryTimeout(),
                [weak_ptr, state_ = state] {
                    if (auto self = weak_ptr.lock()) {
                        self->ExecuteRequest(state_);
                    } else {
                        state_->Response.SetValue(
                            TErrorResponse(E_INVALID_STATE, "request was cancelled"));
                    }
                });
        } else {
            ExecuteRequest(state);
        }

        return state->Response;
    }

    //
    // CreateSession request
    //

    void ExecuteRequest(TRequestStatePtr<TCreateSessionMethod> state)
    {
        // all bookkeeping should be done before executing create session request
        if (!state->SessionId) {
            STORAGE_INFO(LogTag({}) << " creating session");
        } else if (state->Request->GetRestoreClientSession()) {
            STORAGE_INFO(LogTag(state->SessionId) << " recovering session");
        }

        ExecuteRequestWithSession(std::move(state));
    }

    void HandleResponse(
        TRequestStatePtr<TCreateSessionMethod> state,
        TFuture<NProto::TCreateSessionResponse> future)
    {
        NProto::TCreateSessionResponse response;
        ExtractResponse(future, response);

        if (HasError(response)) {
            const auto& error = response.GetError();
            STORAGE_WARN(LogTag(state->SessionId)
                << " failed to establish session: "
                << FormatError(error));

            if (error.GetCode() == E_FS_INVALID_SESSION) {
                // reset session
                CreateOrRestoreSession({}, F_FORCE_RECOVER);

                // notify requests inflight that actual session failed
                response = TErrorResponse(E_REJECTED, "failed to recover session");
            } else {
                STORAGE_ERROR(LogTag(GetSessionId(response))
                    << " session broken");

                with_lock (SessionLock) {
                    if (SessionState != SessionDestroying) {
                        Y_VERIFY(SessionState == SessionRequested);
                        SessionState = SessionBroken;
                    }
                }
            }
        } else {
            STORAGE_INFO(LogTag(GetSessionId(response))
                << " session established");

            with_lock (SessionLock) {
                if (SessionState != SessionDestroying) {
                    Y_VERIFY(SessionState == SessionRequested);
                    SessionState = SessionEstablished;
                }
            }

            SchedulePingSession();
        }

        state->Response.SetValue(std::move(response));
    }

    //
    // DestroySession request
    //

    void ExecuteRequest(TRequestStatePtr<TDestroySessionMethod> state)
    {
        // ensure session exists
        TFuture<NProto::TCreateSessionResponse> sessionResponse;
        with_lock (SessionLock) {
            if (SessionState != SessionRequested && SessionState != SessionEstablished) {
                state->Response.SetValue(
                    TErrorResponse(E_INVALID_STATE, "invalid session state"));
                return;
            }

            SessionState = SessionDestroying;
            sessionResponse = SessionResponse;
        }

        auto weak_ptr = weak_from_this();
        sessionResponse.Subscribe(
            [state_ = std::move(state), weak_ptr] (const auto& future) {
                auto self = weak_ptr.lock();
                if (!self) {
                    state_->Response.SetValue(
                        TErrorResponse(E_INVALID_STATE, "request was cancelled"));
                    return;
                }

                auto sessionId = TryGetSessionId(future);
                if (HasError(sessionId.GetError())) {
                    state_->Response.SetValue(
                        TErrorResponse(sessionId.GetError()));
                    return;
                }

                state_->SessionId = sessionId.GetResult();

                auto& Log = self->Log;
                STORAGE_INFO(self->LogTag(state_->SessionId)
                    << " destroying session");

                self->ExecuteRequestWithSession(std::move(state_));
            });
    }

    void HandleResponse(
        TRequestStatePtr<TDestroySessionMethod> state,
        TFuture<NProto::TDestroySessionResponse> future)
    {
        NProto::TDestroySessionResponse response;
        ExtractResponse(future, response);

        if (HasError(response)) {
            const auto& error = response.GetError();
            STORAGE_WARN(LogTag(state->SessionId)
                << " failed to destroy session: "
                << FormatError(error));
        } else {
            STORAGE_INFO(LogTag(state->SessionId)
                << " session destroyed");
        }

        state->Response.SetValue(std::move(response));
    }

    //
    // Session methods
    //

    template <typename T>
    void ExecuteRequest(TRequestStatePtr<T> state)
    {
        // ensure session exists
        TFuture<NProto::TCreateSessionResponse> sessionResponse;
        with_lock (SessionLock) {
            if (SessionState != SessionRequested && SessionState != SessionEstablished) {
                state->Response.SetValue(
                    TErrorResponse(E_INVALID_STATE, "invalid session state"));
                return;
            }

            sessionResponse = SessionResponse;
        }

        if (sessionResponse.HasValue()) {
            // fast path for established session
            auto sessionId = TryGetSessionId(sessionResponse);
            if (HasError(sessionId.GetError())) {
                state->Response.SetValue(TErrorResponse(sessionId.GetError()));
                return;
            }

            if (state->SessionId && state->SessionId != sessionId.GetResult()) {
                state->Response.SetValue(
                    TErrorResponse(E_REJECTED, "failed to recover session"));
                return;
            }

            state->SessionId = sessionId.GetResult();
            ExecuteRequestWithSession(std::move(state));
            return;
        }

        // slow path - should wait for session
        auto weak_ptr = weak_from_this();
        sessionResponse.Subscribe(
            [state_ = std::move(state), weak_ptr] (const auto& future) mutable {
                auto self = weak_ptr.lock();
                if (!self) {
                    state_->Response.SetValue(
                        TErrorResponse(E_INVALID_STATE, "request was cancelled"));
                    return;
                }

                auto sessionId = TryGetSessionId(future);
                if (HasError(sessionId.GetError())) {
                    state_->Response.SetValue(
                        TErrorResponse(sessionId.GetError()));
                    return;
                }

                if (state_->SessionId && state_->SessionId != sessionId.GetResult()) {
                    state_->Response.SetValue(
                        TErrorResponse(E_REJECTED, "failed to recover session"));
                    return;
                }

                state_->SessionId = sessionId.GetResult();
                self->ExecuteRequestWithSession(std::move(state_));
            });
    }

    template <typename T>
    void ExecuteRequestWithSession(TRequestStatePtr<T> state)
    {
        state->Request->SetFileSystemId(Config->GetFileSystemId());

        auto* headers = state->Request->MutableHeaders();
        headers->SetClientId(Config->GetClientId());
        headers->SetSessionId(state->SessionId);
        headers->SetRequestId(state->CallContext->RequestId);

        auto weak_ptr = weak_from_this();
        T::Execute(*Client, state->CallContext, state->Request).Subscribe(
            [state_ = std::move(state), weak_ptr] (const auto& future) mutable {
                if (auto self = weak_ptr.lock()) {
                    self->HandleResponse(std::move(state_), future);
                } else {
                    state_->Response.SetValue(
                        TErrorResponse(E_INVALID_STATE, "request was cancelled"));
                }
            });
    }

    template <typename T>
    void HandleResponse(
        TRequestStatePtr<T> state,
        TFuture<typename T::TResponse> future)
    {
        typename T::TResponse response;
        ExtractResponse(future, response);

        if (HasError(response)) {
            const auto& error = response.GetError();
            if (error.GetCode() == E_FS_INVALID_SESSION) {
                CreateOrRestoreSession(state->SessionId, F_TIMEOUT);

                // retry request
                ExecuteRequest(std::move(state));
                return;
            }
        }

        state->Response.SetValue(std::move(response));
    }

    //
    // Forwarded methods
    //

    template <typename T>
    TFuture<typename T::TResponse> ForwardRequest(
        TCallContextPtr callContext,
        std::shared_ptr<typename T::TRequest> request)
    {
        request->SetFileSystemId(Config->GetFileSystemId());

        auto* headers = request->MutableHeaders();
        headers->SetClientId(Config->GetClientId());

        return T::Execute(*Client, std::move(callContext), std::move(request));
    }

    //
    // Session timer
    //

    void SchedulePingSession()
    {
        if (PingScheduled.test_and_set()) {
            return;
        }

        auto weak_ptr = weak_from_this();
        Scheduler->Schedule(
            Timer->Now() + Config->GetSessionPingTimeout(),
            [=] {
                if (auto self = weak_ptr.lock()) {
                    self->DoPing();
                }
            });
    }

    void DoPing()
    {
        auto ping = PingSession();

        auto weak_ptr = weak_from_this();
        ping.Subscribe(
            [=] (const TFuture<NProto::TPingSessionResponse>& future) {
                Y_UNUSED(future);
                if (auto self = weak_ptr.lock()) {
                    self->PingScheduled.clear();
                    self->SchedulePingSession();
                }
            });
    }

    TString LogTag(const TString& sessionId)
    {
        return Sprintf("%s[s:%s]", FsTag.c_str(), sessionId.Quote().c_str());
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

ISessionPtr CreateSession(
    ILoggingServicePtr logging,
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    IFileStoreServicePtr client,
    TSessionConfigPtr config)
{
    return std::make_shared<TSession>(
        std::move(logging),
        std::move(timer),
        std::move(scheduler),
        std::move(client),
        std::move(config));
}

}   // namespace NCloud::NFileStore::NClient
