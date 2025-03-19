#include "logbroker.h"

#include "config.h"

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <kikimr/persqueue/sdk/deprecated/cpp/v2/credentials_provider.h>
#include <kikimr/persqueue/sdk/deprecated/cpp/v2/iproducer.h>
#include <kikimr/persqueue/sdk/deprecated/cpp/v2/logger.h>
#include <kikimr/persqueue/sdk/deprecated/cpp/v2/persqueue.h>
#include <kikimr/persqueue/sdk/deprecated/cpp/v2/types.h>
#include <kikimr/public/sdk/cpp/client/iam/iam.h>

#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/system/mutex.h>

namespace NCloud::NBlockStore::NLogbroker {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TIAMCredentialsForwarder
    : public NPersQueue::ICredentialsProvider
{
private:
    TIntrusivePtr<NPersQueue::ILogger> Logger;
    std::shared_ptr<NYdb::ICredentialsProvider> Provider;

public:
    explicit TIAMCredentialsForwarder(
            TStringBuf address,
            TIntrusivePtr<NPersQueue::ILogger> logger)
        : Logger(std::move(logger))
    {
        TStringBuf hostRef;
        TStringBuf portRef;
        address.Split(':', hostRef, portRef);

        const TString host = TString(hostRef);
        const ui32 port = FromStringWithDefault<ui32>(portRef, 6770);

        const NYdb::TCredentialsProviderFactoryPtr factory =
            NYdb::CreateIamCredentialsProviderFactory({host, port});

        Provider = factory->CreateProvider();
    }

    void FillAuthInfo(NPersQueue::TCredentials* authInfo) const override {
        try {
            TString ticket = Provider->GetAuthInfo();
            authInfo->SetTvmServiceTicket(ticket);
        } catch (...) {
            if (Logger) {
                Logger->Log(TStringBuilder()
                    << "Can't get ticket: "
                    << CurrentExceptionMessage()
                    << "\n", "", "", TLOG_ERR
                );
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TLogger final
    : public NPersQueue::ILogger
{
private:
    TLog Impl;

public:
    TLogger(ILoggingServicePtr logging)
        : Impl(logging->CreateLog("BLOCKSTORE_LOGBROKER"))
    {}

    void Log(
        const TString& msg,
        const TString& sourceId,
        const TString& sessionId,
        int level) override
    {
        Impl.Write(static_cast<ELogPriority>(level), TStringBuilder() <<
            sourceId << "/" << sessionId << " " << msg);
    }

    bool IsEnabled(int level) const override
    {
        return level <= Impl.FiltrationLevel();
    }
};

TIntrusivePtr<NPersQueue::ILogger> CreateLogger(ILoggingServicePtr logging)
{
    return MakeIntrusive<TLogger>(std::move(logging));
}

TString FormatError(const NPersQueue::TError& error)
{
    TString r;
    TStringOutput out(r);

    out << NPersQueue::NErrorCode::EErrorCode_Name(error.GetCode())
        << " (" << static_cast<int>(error.GetCode()) << ") "
        << error.GetDescription();

    return r;
}

EWellKnownResultCodes TranslateErrorCode(ui32 errorCode)
{
    switch (errorCode) {
        case NPersQueue::NErrorCode::OK:
            return S_OK;
        case NPersQueue::NErrorCode::INITIALIZING:
        case NPersQueue::NErrorCode::OVERLOAD:
        case NPersQueue::NErrorCode::READ_TIMEOUT:
        case NPersQueue::NErrorCode::TABLET_IS_DROPPED:
        case NPersQueue::NErrorCode::CREATE_TIMEOUT:
        case NPersQueue::NErrorCode::ERROR:
        case NPersQueue::NErrorCode::CLUSTER_DISABLED:
            return E_REJECTED;
        default:
            return E_FAIL;
    }
}

NProto::TError MakeError(
    const TFuture<NPersQueue::TProducerCreateResponse>& future)
{
    try {
        const auto& response = future.GetValue().Response;

        if (response.HasError()) {
            const auto& error = response.GetError();
            return MakeError(TranslateErrorCode(error.GetCode()), FormatError(error));
        }
    } catch (...) {
        return MakeError(E_FAIL, CurrentExceptionMessage());
    }

    return {};
}

NProto::TError MakeError(
    const TFuture<NPersQueue::TProducerCommitResponse>& future)
{
    try {
        const auto& response = future.GetValue().Response;

        if (response.HasError()) {
            const auto& error = response.GetError();
            return MakeError(TranslateErrorCode(error.GetCode()), FormatError(error));
        }

        if (response.HasAck() && response.GetAck().GetAlreadyWritten()) {
            return MakeError(S_ALREADY, {});
        }
    } catch (...) {
        return MakeError(E_FAIL, CurrentExceptionMessage());
    }

    return {};
}

////////////////////////////////////////////////////////////////////////////////

NPersQueue::TPQLibSettings CreateLibSettings(ILoggingServicePtr logging)
{
    NPersQueue::TPQLibSettings settings;

    settings.CompressionPoolThreads = 1,
    settings.DefaultLogger = CreateLogger(logging);

    return settings;
}

NPersQueue::TProducerSettings CreateProducerSettings(
    const TLogbrokerConfig& config,
    ILoggingServicePtr logging)
{
    NPersQueue::TProducerSettings settings;

    settings.Server.Address = config.GetAddress();
    settings.Server.Port = config.GetPort();
    settings.Server.Database = config.GetDatabase();
    settings.Server.UseLogbrokerCDS = config.GetUseLogbrokerCDS()
        ? NPersQueue::EClusterDiscoveryUsageMode::Use
        : NPersQueue::EClusterDiscoveryUsageMode::DontUse;

    if (auto address = config.GetMetadataServerAddress()) {
        settings.CredentialsProvider = std::make_shared<TIAMCredentialsForwarder>(
            address,
            CreateLogger(logging));

        settings.Server.EnableSecureConnection(
            config.GetCaCertFilename()
                ? TFileInput(config.GetCaCertFilename()).ReadAll()
                : TString());
    }

    settings.Topic = config.GetTopic();
    settings.SourceId = config.GetSourceId();
    settings.ReconnectOnFailure = false;

    return settings;
}

////////////////////////////////////////////////////////////////////////////////

class TServiceStub final
    : public IService
{
public:
    TFuture<NProto::TError> Write(TVector<TMessage> messages, TInstant now) override
    {
        Y_UNUSED(messages);
        Y_UNUSED(now);

        return MakeFuture(NProto::TError());
    }

    void Start() override
    {}

    void Stop() override
    {}
};

////////////////////////////////////////////////////////////////////////////////

class TServiceNull final
    : public IService
{
private:
    const ILoggingServicePtr Logging;
    TLog Log;

public:
    explicit TServiceNull(ILoggingServicePtr logging)
         : Logging(std::move(logging))
    {}

    TFuture<NProto::TError> Write(TVector<TMessage> messages, TInstant now) override
    {
        Y_UNUSED(now);

        for (const auto& m: messages) {
            STORAGE_WARN("Discard message #" << m.SeqNo);
        }

        return MakeFuture(NProto::TError());
    }

    void Start() override
    {
        Log = Logging->CreateLog("BLOCKSTORE_LOGBROKER");
    }

    void Stop() override
    {}
};

////////////////////////////////////////////////////////////////////////////////

class TService final
    : public IService
{
    using IProducerPtr = std::shared_ptr<NPersQueue::IProducer>;

private:
    const TLogbrokerConfigPtr Config;
    const ILoggingServicePtr Logging;

    std::unique_ptr<NPersQueue::TPQLib> Pq;

public:
    TService(
            TLogbrokerConfigPtr config,
            ILoggingServicePtr logging)
        : Config(std::move(config))
        , Logging(std::move(logging))
    {}

    TFuture<NProto::TError> Write(
        TVector<TMessage> messages,
        TInstant now) override
    {
        if (messages.empty()) {
            return MakeFuture(NProto::TError());
        }

        Y_VERIFY(Pq);

        IProducerPtr producer(Pq->CreateProducer(CreateProducerSettings(
            *Config, Logging)).Release());

        return producer->Start().Apply(
            [=] (const auto& future) mutable {
                return HandleProducerCreateResponse(
                    producer,
                    future,
                    std::move(messages),
                    now);
            });
    }

    void Start() override
    {
        Pq = std::make_unique<NPersQueue::TPQLib>(CreateLibSettings(Logging));
    }

    void Stop() override
    {
        Pq = nullptr;
    }

private:
    static NProto::TError HandleProducerWriteResponse(
        IProducerPtr producer,
        const TVector<TFuture<NPersQueue::TProducerCommitResponse>>& futures)
    {
        Y_UNUSED(producer);

        for (auto& f: futures) {
            const auto error = MakeError(f);

            if (HasError(error)) {
                return error;
            }
        }

        return {};
    }

    static TFuture<NProto::TError> HandleProducerCreateResponse(
        IProducerPtr producer,
        const TFuture<NPersQueue::TProducerCreateResponse>& future,
        TVector<TMessage> messages,
        TInstant now)
    {
        if (auto error = MakeError(future); HasError(error)) {
            return MakeFuture(error);
        }

        TVector<TFuture<NPersQueue::TProducerCommitResponse>> futures;

        futures.reserve(messages.size());

        for (auto& m: messages) {
            NPersQueue::TData data{ std::move(m.Payload), now };
            futures.push_back(producer->Write(m.SeqNo, std::move(data)));
        }

        return WaitAll(futures).Apply([=] (const auto& future) mutable {
            Y_UNUSED(future);

            return HandleProducerWriteResponse(std::move(producer), futures);
        });
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IServicePtr CreateService(
    TLogbrokerConfigPtr config,
    ILoggingServicePtr logging)
{
    return std::make_shared<TService>(std::move(config), std::move(logging));
}

IServicePtr CreateServiceStub()
{
    return std::make_shared<TServiceStub>();
}

IServicePtr CreateServiceNull(ILoggingServicePtr logging)
{
    return std::make_shared<TServiceNull>(std::move(logging));
}

}   // namespace NCloud::NBlockStore::NLogbroker
