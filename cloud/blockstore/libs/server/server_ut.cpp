#include "server.h"

#include "server_test.h"

#include <cloud/blockstore/libs/client/client.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats_test.h>
#include <cloud/blockstore/libs/service/service_test.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/folder/path.h>
#include <util/generic/scope.h>

namespace NCloud::NBlockStore::NServer {

using namespace NThreading;

using namespace NCloud::NBlockStore::NClient;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TServerTest)
{
    Y_UNIT_TEST(ShouldHandleRequests)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                Y_UNUSED(request);
                return MakeFuture<NProto::TPingResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetPort(port)
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetPort(port)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->Ping(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TPingRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    Y_UNIT_TEST(ShouldHandleAuthRequests)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                UNIT_ASSERT_VALUES_EQUAL(
                    "test",
                    request->GetHeaders().GetInternal().GetAuthToken()
                );
                return MakeFuture<NProto::TPingResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetSecureEndpoint(
                port,
                "certs/server.crt",
                "certs/server.crt",
                "certs/server.key")
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetSecureEndpoint(
                port,
                "certs/server.crt",
                "test")
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->Ping(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TPingRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    // NBS-3132
    // Y_UNIT_TEST(ShouldHandleAuthRequestsWithCustomClientCertificate)
    // {
    //     TPortManager portManager;
    //     ui16 port = portManager.GetPort(9001);

    //     auto service = std::make_shared<TTestService>();
    //     service->PingHandler =
    //         [&] (std::shared_ptr<NProto::TPingRequest> request) {
    //             UNIT_ASSERT_VALUES_EQUAL(
    //                 "test",
    //                 request->GetHeaders().GetInternal().GetAuthToken()
    //             );
    //             return MakeFuture<NProto::TPingResponse>();
    //         };

    //     TTestFactory testFactory;

    //     auto server = testFactory.CreateServerBuilder()
    //         .SetSecureEndpoint(
    //             port,
    //             "certs/server.crt",
    //             "certs/server.crt",
    //             "certs/server.key")
    //         .BuildServer(service);

    //     auto client = testFactory.CreateClientBuilder()
    //         .SetSecureEndpoint(
    //             port,
    //             "certs/server.crt",
    //             "test")
    //         .SetCertificate(
    //             "certs/server.crt",
    //             "certs/server.key")
    //         .BuildClient();

    //     server->Start();
    //     client->Start();
    //     Y_DEFER {
    //         client->Stop();
    //         server->Stop();
    //     };

    //     auto endpoint = client->CreateEndpoint();
    //     endpoint = testFactory.CreateDurableClient(std::move(endpoint));

    //     endpoint->Start();
    //     Y_DEFER {
    //         endpoint->Stop();
    //     };

    //     auto future = endpoint->Ping(
    //         MakeIntrusive<TCallContext>(),
    //         std::make_shared<NProto::TPingRequest>()
    //     );

    //     const auto& response = future.GetValue(TDuration::Seconds(5));
    //     UNIT_ASSERT_C(!HasError(response), response.GetError());
    // }

    Y_UNIT_TEST(ShouldHandleAuthRequestsWithMultipleCertificates)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                UNIT_ASSERT_VALUES_EQUAL(
                    "test",
                    request->GetHeaders().GetInternal().GetAuthToken()
                );
                return MakeFuture<NProto::TPingResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetSecureEndpoint(
                port,
                "certs/server.crt",
                {},
                {})
            .AddCert(
                "certs/server_fallback.crt",
                "certs/server.key")
            .AddCert(
                "certs/server.crt",
                "certs/server.key")
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetSecureEndpoint(
                port,
                "certs/server.crt",
                "test")
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->Ping(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TPingRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    Y_UNIT_TEST(ShouldHandleSecureAndInsecureClientsSimultaneously)
    {
        TPortManager portManager;
        ui16 securePort = portManager.GetPort(9001);
        ui16 insecurePort = portManager.GetPort(9002);

        auto service = std::make_shared<TTestService>();
        service->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                Y_UNUSED(request);
                return MakeFuture<NProto::TPingResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetPort(insecurePort)
            .SetSecureEndpoint(
                securePort,
                "certs/server.crt",
                "certs/server.crt",
                "certs/server.key")
            .BuildServer(service);

        auto secureClient = testFactory.CreateClientBuilder()
            .SetSecureEndpoint(
                securePort,
                "certs/server.crt",
                "test")
            .BuildClient();

        auto insecureClient = testFactory.CreateClientBuilder()
            .SetPort(insecurePort)
            .BuildClient();

        server->Start();
        secureClient->Start();
        insecureClient->Start();
        Y_DEFER {
            insecureClient->Stop();
            secureClient->Stop();
            server->Stop();
        };

        auto secureEndpoint = secureClient->CreateEndpoint();
        auto insecureEndpoint = insecureClient->CreateEndpoint();

        secureEndpoint->Start();
        insecureEndpoint->Start();
        Y_DEFER {
            insecureEndpoint->Stop();
            secureEndpoint->Stop();
        };

        auto futureFromSecure = secureEndpoint->Ping(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TPingRequest>()
        );

        auto futureFromInsecure = insecureEndpoint->Ping(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TPingRequest>()
        );

        const auto& responseFromSecure =
            futureFromSecure.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(responseFromSecure), responseFromSecure.GetError());

        const auto& responseFromInsecure =
            futureFromInsecure.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(responseFromInsecure), responseFromInsecure.GetError());
    }

    Y_UNIT_TEST(ShouldIgnoreAuthTokenInHeader)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                UNIT_ASSERT_VALUES_EQUAL(
                    "",
                    request->GetHeaders().GetInternal().GetAuthToken()
                );
                return MakeFuture<NProto::TPingResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetPort(port)
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetPort(port)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto request = std::make_shared<NProto::TPingRequest>();
        request->MutableHeaders()->MutableInternal()->SetAuthToken("test");
        auto future = endpoint->Ping(
            MakeIntrusive<TCallContext>(),
            std::move(request)
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    Y_UNIT_TEST(ShouldPropagateClientId)
    {
        TString clientId = "testClientId";
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                UNIT_ASSERT(request->HasHeaders());
                UNIT_ASSERT_VALUES_EQUAL(clientId, request->GetHeaders().GetClientId());
                return MakeFuture<NProto::TPingResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetPort(port)
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetPort(port)
            .SetClientId(clientId)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->Ping(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TPingRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    Y_UNIT_TEST(ShouldFailRequestsToWrongServiceFromControlClient)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->WriteBlocksHandler =
            [&] (std::shared_ptr<NProto::TWriteBlocksRequest> request) {
                Y_UNUSED(request);
                return MakeFuture<NProto::TWriteBlocksResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetDataPort(port)
            .BuildServer(service);

        // mismatched Client/DataClient
        auto client = testFactory.CreateClientBuilder()
            .SetPort(port)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->WriteBlocks(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TWriteBlocksRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_VALUES_EQUAL(
            E_GRPC_UNIMPLEMENTED,
            response.GetError().GetCode()
        );
    }

    Y_UNIT_TEST(ShouldFailRequestsToWrongServiceFromDataClient)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->WriteBlocksHandler =
            [&] (std::shared_ptr<NProto::TWriteBlocksRequest> request) {
                Y_UNUSED(request);
                return MakeFuture<NProto::TWriteBlocksResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetPort(port)
            .BuildServer(service);

        // mismatched Client/DataClient
        auto client = testFactory.CreateClientBuilder()
            .SetDataPort(port)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateDataEndpoint();
        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->WriteBlocks(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TWriteBlocksRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_VALUES_EQUAL(
            E_GRPC_UNIMPLEMENTED,
            response.GetError().GetCode()
        );
    }

    Y_UNIT_TEST(ShouldCancelRequestsOnServerShutdown)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto writePromise = NewPromise<NProto::TWriteBlocksResponse>();

        auto service = std::make_shared<TTestService>();
        service->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                Y_UNUSED(request);
                return MakeFuture<NProto::TPingResponse>();
            };
        service->WriteBlocksHandler =
            [&] (std::shared_ptr<NProto::TWriteBlocksRequest> request) {
                Y_UNUSED(request);
                return writePromise;   // will hang until value is set
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetPort(port)
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetPort(port)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->WriteBlocks(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TWriteBlocksRequest>()
        );

        // control request to ensure client and server completely started
        {
            auto future = endpoint->Ping(
                MakeIntrusive<TCallContext>(),
                std::make_shared<NProto::TPingRequest>()
            );
            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(response), response.GetError());
        }

        server->Stop();

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_VALUES_EQUAL_C(
            EErrorKind::ErrorRetriable,
            GetErrorKind(response.GetError()),
            response.GetError()
        );

        writePromise.SetValue(NProto::TWriteBlocksResponse());
    }

    Y_UNIT_TEST(ShouldCancelRequestsOnClientShutdown)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto writePromise = NewPromise<NProto::TWriteBlocksResponse>();

        auto service = std::make_shared<TTestService>();
        service->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                Y_UNUSED(request);
                return MakeFuture<NProto::TPingResponse>();
            };
        service->WriteBlocksHandler =
            [&] (std::shared_ptr<NProto::TWriteBlocksRequest> request) {
                Y_UNUSED(request);
                return writePromise;   // will hang until value is set
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetPort(port)
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetPort(port)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->WriteBlocks(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TWriteBlocksRequest>()
        );

        // control request to ensure client and server completely started
        {
            auto future = endpoint->Ping(
                MakeIntrusive<TCallContext>(),
                std::make_shared<NProto::TPingRequest>()
            );
            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(response), response.GetError());
        }

        endpoint->Stop();
        client->Stop();

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_VALUES_EQUAL(
            E_GRPC_CANCELLED,
            response.GetError().GetCode()
        );

        writePromise.SetValue(NProto::TWriteBlocksResponse());
    }

    Y_UNIT_TEST(ShouldIdentifyInsecureControlChannelSource)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->ReadBlocksHandler =
            [&] (std::shared_ptr<NProto::TReadBlocksRequest> request) {
                UNIT_ASSERT_VALUES_EQUAL(
                    int(NProto::SOURCE_INSECURE_CONTROL_CHANNEL),
                    int(request->GetHeaders().GetInternal().GetRequestSource())
                );
                return MakeFuture<NProto::TReadBlocksResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetPort(port)
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetPort(port)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->ReadBlocks(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TReadBlocksRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    Y_UNIT_TEST(ShouldIdentifySecureControlChannelSource)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->ReadBlocksHandler =
            [&] (std::shared_ptr<NProto::TReadBlocksRequest> request) {
                UNIT_ASSERT_VALUES_EQUAL(
                    int(NProto::SOURCE_SECURE_CONTROL_CHANNEL),
                    int(request->GetHeaders().GetInternal().GetRequestSource())
                );
                return MakeFuture<NProto::TReadBlocksResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetSecureEndpoint(
                port,
                "certs/server.crt",
                "certs/server.crt",
                "certs/server.key")
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetSecureEndpoint(
                port,
                "certs/server.crt",
                "test")
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->ReadBlocks(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TReadBlocksRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    Y_UNIT_TEST(ShouldIdentifyTcpDataChannelSource)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->ReadBlocksHandler =
            [&] (std::shared_ptr<NProto::TReadBlocksRequest> request) {
                UNIT_ASSERT_VALUES_EQUAL(
                    int(NProto::SOURCE_TCP_DATA_CHANNEL),
                    int(request->GetHeaders().GetInternal().GetRequestSource())
                );
                return MakeFuture<NProto::TReadBlocksResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetDataPort(port)
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetDataPort(port)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateDataEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->ReadBlocks(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TReadBlocksRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    Y_UNIT_TEST(ShouldIdentifyFdDataChannelSource)
    {
        TFsPath unixSocket("./test_socket");

        auto service = std::make_shared<TTestService>();
        service->ReadBlocksHandler =
            [&] (std::shared_ptr<NProto::TReadBlocksRequest> request) {
                UNIT_ASSERT_VALUES_EQUAL(
                    int(NProto::SOURCE_FD_DATA_CHANNEL),
                    int(request->GetHeaders().GetInternal().GetRequestSource())
                );
                return MakeFuture<NProto::TReadBlocksResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetUnixSocketPath(unixSocket.GetPath())
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetUnixSocketPath(unixSocket.GetPath())
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateDataEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->ReadBlocks(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TReadBlocksRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    Y_UNIT_TEST(ShouldHandleRequestsWithUnixSocketOnDataChannel)
    {
        TPortManager portManager;
        ui16 serverPort = portManager.GetPort(9001);
        ui16 clientPort = portManager.GetPort(9002);
        TFsPath unixSocket("./test_socket");

        auto service = std::make_shared<TTestService>();
        service->ReadBlocksHandler =
            [&] (std::shared_ptr<NProto::TReadBlocksRequest> request) {
                Y_UNUSED(request);
                return MakeFuture<NProto::TReadBlocksResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetDataPort(serverPort)
            .SetUnixSocketPath(unixSocket.GetPath())
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetDataPort(clientPort)
            .SetUnixSocketPath(unixSocket.GetPath())
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateDataEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        auto future = endpoint->ReadBlocks(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TReadBlocksRequest>()
        );

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response.GetError());
    }

    Y_UNIT_TEST(ShouldCleanVolumeStatsWhileUnmountVolume)
    {
        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);
        TString diskId = "testDiskId";

        auto service = std::make_shared<TTestService>();
        service->MountVolumeHandler =
            [&] (std::shared_ptr<NProto::TMountVolumeRequest> request) {
                NProto::TMountVolumeResponse response;
                response.MutableVolume()->SetDiskId(request->GetDiskId());
                return MakeFuture(response);
            };
        service->UnmountVolumeHandler =
            [&] (std::shared_ptr<NProto::TUnmountVolumeRequest> request) {
                Y_UNUSED(request);
                return MakeFuture<NProto::TUnmountVolumeResponse>();
            };

        auto serverVolumeStats = std::make_shared<TTestVolumeStats<>>();
        auto clientVolumeStats = std::make_shared<TTestVolumeStats<>>();

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetPort(port)
            .SetVolumeStats(serverVolumeStats)
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetPort(port)
            .SetVolumeStats(clientVolumeStats)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        {
            auto request = std::make_shared<NProto::TMountVolumeRequest>();
            request->SetDiskId(diskId);

            auto future = endpoint->MountVolume(
                MakeIntrusive<TCallContext>(),
                std::move(request));

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(response), response.GetError());
        }

        UNIT_ASSERT_VALUES_EQUAL(
            TSet<TString>{diskId},
            serverVolumeStats->DiskIds
        );
        UNIT_ASSERT_VALUES_EQUAL(
            TSet<TString>{diskId},
            clientVolumeStats->DiskIds
        );

        {
            auto request = std::make_shared<NProto::TUnmountVolumeRequest>();
            request->SetDiskId(diskId);

            auto future = endpoint->UnmountVolume(
                MakeIntrusive<TCallContext>(),
                std::move(request));

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(response), response.GetError());
        }

        UNIT_ASSERT_VALUES_EQUAL(
            TSet<TString>(),
            serverVolumeStats->DiskIds
        );
        UNIT_ASSERT_VALUES_EQUAL(
            TSet<TString>(),
            clientVolumeStats->DiskIds
        );
    }

    Y_UNIT_TEST(ShouldHandleRequestsWithUnixSocketOnDataChannelAfterRestart)
    {
        TPortManager portManager;
        ui16 serverPort = portManager.GetPort(9001);
        ui16 clientPort = portManager.GetPort(9002);
        TFsPath unixSocket("./test_socket");

        auto service = std::make_shared<TTestService>();
        service->ReadBlocksHandler =
            [&] (std::shared_ptr<NProto::TReadBlocksRequest> request) {
                Y_UNUSED(request);
                return MakeFuture<NProto::TReadBlocksResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetDataPort(serverPort)
            .SetUnixSocketPath(unixSocket.GetPath())
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetDataPort(clientPort)
            .SetUnixSocketPath(unixSocket.GetPath())
            .BuildClient();

        server->Start();

        client->Start();

        auto endpoint = client->CreateDataEndpoint();
        endpoint->Start();

        {
            server->Stop();
            server = testFactory.CreateServerBuilder()
                .SetDataPort(serverPort)
                .SetUnixSocketPath(unixSocket.GetPath())
                .BuildServer(service);
            server->Start();
        }

        bool success = false;
        for (int i = 0; i < 3; ++i) {
            auto future = endpoint->ReadBlocks(
                MakeIntrusive<TCallContext>(),
                std::make_shared<NProto::TReadBlocksRequest>()
            );

            const auto& response = future.GetValue(TDuration::Seconds(5));
            if (!HasError(response)) {
                success = true;
                break;
            }
        }

        UNIT_ASSERT(success);

        endpoint->Stop();
        client->Stop();
        server->Stop();
    }

    Y_UNIT_TEST(ShouldHandleLocalIORequestsWithOverheadBuffer)
    {
        ui32 blocksCount = 42;
        ui32 blockSize = 512;
        ui32 overheadSize = 1234;
        char content = 'x';

        TPortManager portManager;
        ui16 port = portManager.GetPort(9001);

        auto service = std::make_shared<TTestService>();
        service->ReadBlocksHandler =
            [&] (std::shared_ptr<NProto::TReadBlocksRequest> request) {
                UNIT_ASSERT_VALUES_EQUAL(blocksCount, request->GetBlocksCount());

                NProto::TReadBlocksResponse response;
                auto& buffers = *response.MutableBlocks()->MutableBuffers();
                for (size_t i = 0; i < blocksCount; ++i) {
                    auto* buf = buffers.Add();
                    buf->ReserveAndResize(blockSize);
                    memset(
                        const_cast<char*>(buf->data()),
                        content,
                        buf->size());
                }

                return MakeFuture(std::move(response));
            };
        service->WriteBlocksHandler =
            [&] (std::shared_ptr<NProto::TWriteBlocksRequest> request) {
                UNIT_ASSERT_VALUES_EQUAL(
                    blocksCount,
                    request->GetBlocks().GetBuffers().size());

                for (const auto& block: request->GetBlocks().GetBuffers()) {
                    UNIT_ASSERT_VALUES_EQUAL(block.size(), blockSize);
                    UNIT_ASSERT_VALUES_EQUAL(
                        TString(blockSize, content),
                        block.Data());
                }

                return MakeFuture<NProto::TWriteBlocksResponse>();
            };

        TTestFactory testFactory;

        auto server = testFactory.CreateServerBuilder()
            .SetDataPort(port)
            .BuildServer(service);

        auto client = testFactory.CreateClientBuilder()
            .SetDataPort(port)
            .BuildClient();

        server->Start();
        client->Start();
        Y_DEFER {
            client->Stop();
            server->Stop();
        };

        auto endpoint = client->CreateDataEndpoint();
        endpoint = testFactory.CreateDurableClient(std::move(endpoint));

        endpoint->Start();
        Y_DEFER {
            endpoint->Stop();
        };

        {
            TString buffer(blocksCount * blockSize + overheadSize, 0);
            auto sglist = TSgList{{buffer.Data(), buffer.Size()}};

            auto request = std::make_shared<NProto::TReadBlocksLocalRequest>();
            request->SetBlocksCount(blocksCount);
            request->BlockSize = blockSize;
            request->Sglist = TGuardedSgList(sglist);

            auto future = endpoint->ReadBlocksLocal(
                MakeIntrusive<TCallContext>(),
                std::move(request));

            const auto& response = future.GetValue(TDuration::Seconds(5));

            for (size_t i = 0; i < buffer.Size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(
                    i < blocksCount * blockSize ? content : 0,
                    buffer[i]);
            }
            UNIT_ASSERT_C(!HasError(response), response.GetError());
        }

        {
            TString buffer(blocksCount * blockSize + overheadSize, 0);
            memset(const_cast<char*>(buffer.Data()), content, blocksCount * blockSize);

            auto request = std::make_shared<NProto::TWriteBlocksLocalRequest>();
            request->BlocksCount = blocksCount;
            request->BlockSize = blockSize;
            request->Sglist = TGuardedSgList({{buffer.Data(), buffer.Size()}});

            auto future = endpoint->WriteBlocksLocal(
                MakeIntrusive<TCallContext>(),
                std::move(request));

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(response), response.GetError());
        }
    }
}

}   // namespace NCloud::NBlockStore::NServer
