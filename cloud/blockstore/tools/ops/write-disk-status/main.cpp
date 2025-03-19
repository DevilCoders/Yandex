#include "options.h"

#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <kikimr/persqueue/sdk/deprecated/cpp/v2/credentials_provider.h>
#include <kikimr/persqueue/sdk/deprecated/cpp/v2/iproducer.h>
#include <kikimr/persqueue/sdk/deprecated/cpp/v2/logger.h>
#include <kikimr/persqueue/sdk/deprecated/cpp/v2/persqueue.h>
#include <kikimr/persqueue/sdk/deprecated/cpp/v2/types.h>

#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/printf.h>
#include <util/system/env.h>
#include <util/system/hostname.h>

using namespace NPersQueue;

////////////////////////////////////////////////////////////////////////////////

auto CreateLogger(const TOptions& options)
{
    return MakeIntrusive<TCerrLogger>(static_cast<int>(options.VerboseLevel));
}

auto CreatePayload(const TOptions& options)
{
    using namespace NCloud::NBlockStore;

    NProto::TDiskState state;

    state.SetDiskId(options.DiskId);
    state.SetState(static_cast<NProto::EDiskState>(options.DiskState));
    state.SetStateMessage(options.Message);

    TString payload;
    TStringOutput out(payload);

    state.SerializeToArcadiaStream(&out);

    return payload;
}

auto CreateLibSettings(const TOptions& options)
{
    TPQLibSettings libSettings;

    libSettings.ThreadsCount = 1;
    libSettings.CompressionPoolThreads = 1;
    libSettings.DefaultLogger = CreateLogger(options);

    return libSettings;
}

auto CreateCredentialsProvider(const TOptions& options)
{
    if (options.UseOAuth) {
        auto token = GetEnv("LB_OAUTH_TOKEN");
        Y_ENSURE(token);
        return CreateOAuthCredentialsProvider(token);
    }

    if (options.IamJwtKeyFilename) {
        return CreateIAMJwtFileCredentialsForwarder(
            options.IamJwtKeyFilename,
            CreateLogger(options),
            options.IamEndpoint);
    }

    return CreateIAMCredentialsForwarder(CreateLogger(options));
}

auto CreateProducer(TPQLib& pq, const TOptions& options)
{
    TProducerSettings settings;
    settings.Server.Address = options.Endpoint;
    settings.Server.Database = options.Database;

    if (options.CaCertFilename) {
        TFileInput file(options.CaCertFilename);
        settings.Server.EnableSecureConnection(file.ReadAll());
    } else {
        settings.Server.EnableSecureConnection("");
    }

    settings.Topic = options.Topic;
    settings.SourceId = options.SourceId
        ? options.SourceId
        : Sprintf("nbs:%s:%lu", GetFQDNHostName(), TInstant::Now().MilliSeconds());
    settings.ReconnectOnFailure = options.ReconnectOnFailure;
    settings.CredentialsProvider = CreateCredentialsProvider(options);

    return pq.CreateProducer(settings);
}

int main(int argc, char** argv)
{
    using namespace NCloud::NBlockStore;

    TOptions options;
    try {
        options.Parse(argc, argv);

        auto loggingService = NCloud::CreateLoggingService("console", {
            .FiltrationLevel = static_cast<ELogPriority>(options.VerboseLevel),
            .UseLocalTimestamps = true
        });

        auto Log = loggingService->CreateLog("BLOCKSTORE_WRITE_DISK_STATUS");
        STORAGE_DEBUG("Disk: " << options.DiskId);
        STORAGE_DEBUG("Endpoint: " << options.Endpoint);
        STORAGE_DEBUG("Database: " << options.Database);
        STORAGE_DEBUG("IamEndpoint: " << options.IamEndpoint);
        STORAGE_DEBUG("Topic: " << options.Topic);

        try {

            TPQLib pq(CreateLibSettings(options));

            auto producer = CreateProducer(pq, options);
            auto start = producer->Start().GetValue(options.Timeout).Response;

            if (start.HasError()) {
                STORAGE_ERROR("Fail to start producer: " << start.GetError());
                return 1;
            }

            TData data(CreatePayload(options), TInstant::Now());

            auto response = producer->Write(options.SeqNo, std::move(data))
                .GetValue(options.Timeout).Response;

            if (response.HasError()) {
                STORAGE_ERROR("Fail to write message: " << response.GetError());
                return 1;
            }
        } catch (...) {
            STORAGE_ERROR(CurrentExceptionMessage());
            return 1;
        }
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }

    return 0;
}
