#include "backend_sender.h"

#include "instance_hashing.h"

#include <antirobot/idl/cache_sync.pb.h>

#include <antirobot/lib/http_request.h>
#include <antirobot/lib/test_server.h>

#include <infra/yp_service_discovery/libs/sdlib/server_mock/server.h>

#include <library/cpp/iterator/concatenate.h>
#include <library/cpp/iterator/enumerate.h>

#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/response.h>

#include <library/cpp/testing/unittest/tests_data.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/adaptor.h>
#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/string/builder.h>
#include <util/thread/pool.h>

#include <atomic>


namespace NAntiRobot {


Y_UNIT_TEST_SUITE(TestBackendSender) {
    NYP::NServiceDiscovery::NTesting::TSDServer::TOptions MakeSDOptions() {
        NYP::NServiceDiscovery::NTesting::TSDServer::TOptions sdServerOptions;
        TPortManager portManager;
        sdServerOptions.Port = portManager.GetTcpPort();
        return sdServerOptions;
    }

    THolder<TBackendSender> MakeSender(
        const TVector<THostAddr>& addrs,
        const TVector<TAntirobotDaemonConfig::TEndpointSetDescription>& endpointSets = {},
        ui16 port = 8080,
        bool start = true,
        bool reverseHosts = false
    ) {
        TVector<std::pair<TString, THostAddr>> idAddrs;

        for (const auto& [i, addr] : Enumerate(addrs)) {
            idAddrs.push_back({ToString(i), addr});
        }

        if (reverseHosts) {
            Reverse(idAddrs.begin(), idAddrs.end());
        }

        auto sender = MakeHolder<TBackendSender>(
            "1s", "",
            ANTIROBOT_DAEMON_CONFIG.ForwardRequestTimeout,
            ANTIROBOT_DAEMON_CONFIG.DeadBackendSkipperMaxProbability,
            endpointSets.empty() ? "" : "localhost", port,
            endpointSets,
            idAddrs
        );

        if (start) {
            sender->StartDiscovery();
        }

        return sender;
    }


    Y_UNIT_TEST(EmptyFails) {
        const auto sender = MakeSender({});
        const TIpRangeMap<size_t> ipMap;

        THttpRequest dummy("GET", "localhost", "/");

        TString answer;
        TError err = sender->Send(&ipMap, TAddr("1.1.1.1"), dummy).ExtractValueSync().PutValueTo(answer);

        UNIT_ASSERT(err.Defined());
        UNIT_ASSERT(err.Is<TForwardingFailure>());
    }


    struct TLocationChecker : public TRequestReplier {
        TString Location;

        explicit TLocationChecker(TString location)
            : Location(std::move(location))
        {}

        bool DoReply(const TReplyParams& params) override {
            TParsedHttpFull request(params.Input.FirstLine());
            if (request.Path == Location) {
                params.Output << THttpResponse(HTTP_OK).SetContent(NCacheSyncProto::TMessage().SerializeAsString());
            } else {
                params.Output << THttpResponse(HTTP_NOT_FOUND);
            }
            return true;
        }

        TLocationChecker* Clone() const {
            return new TLocationChecker(Location);
        }
    };


    template <typename ClientRequest>
    struct TCloneCallback : public THttpServer::ICallBack {
        THolder<ClientRequest> Replier;

        explicit TCloneCallback(THolder<ClientRequest> replier)
            : Replier(std::move(replier))
        {}

        TClientRequest* CreateClient() override {
            return Replier->Clone();
        }
    };


    Y_UNIT_TEST(SendsToGivenLocation) {
        const TString location = "/test";
        TCloneCallback<TLocationChecker> callback(MakeHolder<TLocationChecker>(location));
        TTestServer server(callback);

        const TVector<THostAddr> addrs = {server.Host};
        THttpRequest dummy("GET", "localhost", location);

        const auto sender = MakeSender(addrs);
        const TIpRangeMap<size_t> ipMap;
        sender->Send(&ipMap, TAddr("1.1.1.1"), dummy).ExtractValueSync();
    }


    struct TRequestCounter : public TRequestReplier {
        std::atomic<int>* Counter = nullptr;
        std::atomic<bool>* Alive = nullptr;

        explicit TRequestCounter(std::atomic<int>* counter, std::atomic<bool>* alive = nullptr)
            : Counter(counter)
            , Alive(alive)
        {}

        bool DoReply(const TReplyParams& params) override {
            ++*Counter;

            if (Alive && !*Alive) {
                params.Output << THttpResponse(HTTP_BAD_REQUEST);
                return true;
            }

            params.Output << THttpResponse(HTTP_OK).SetContent(NCacheSyncProto::TMessage().SerializeAsString());
            return true;
        }

        TRequestReplier* Clone() const {
            return new TRequestCounter(Counter, Alive);
        }
    };


    class TCountingServer {
    public:
        TCountingServer()
            : Callback(MakeHolder<TRequestCounter>(&Count, &Alive))
            , Base(Callback)
        {}

        TCountingServer(TCountingServer&& that) noexcept
            : Alive(static_cast<bool>(that.Alive))
            , Count(static_cast<int>(that.Count))
            , Callback(std::move(that.Callback))
            , Base(std::move(that.Base))
        {}

        TCountingServer& operator=(TCountingServer&& that) noexcept {
            Alive = static_cast<bool>(that.Alive);
            Count = static_cast<int>(that.Count);
            Callback = std::move(that.Callback);
            Base = std::move(that.Base);
            return *this;
        }

        int GetCount() const {
            return Count;
        }

        const THostAddr& GetHost() const {
            return Base.Host;
        }

    public:
        std::atomic<bool> Alive = true;

    private:
        std::atomic<int> Count = 0;

        TCloneCallback<TRequestCounter> Callback;
        TTestServer Base;
    };


    TVector<THostAddr> CollectHosts(const TVector<TCountingServer>& servers) {
        TVector<THostAddr> addrs;

        for (const auto& server : servers) {
            addrs.push_back(server.GetHost());
        }

        return addrs;
    }


    TVector<TString> CollectNames(const TVector<TCountingServer>& servers) {
        TVector<TString> names;

        for (const auto& server : servers) {
            const auto& host = server.GetHost();
            names.push_back(TStringBuilder() << host.HostOrIp << ':' << host.Port);
        }

        return names;
    }


    TVector<ui16> CollectPorts(const TVector<TCountingServer>& servers) {
        TVector<ui16> ports;

        for (const auto& server : servers) {
            ports.push_back(server.GetHost().Port);
        }

        return ports;
    }


    Y_UNIT_TEST(SendsToGivenBackend) {
        THttpRequest dummy("GET", "", "/");

        TVector<TCountingServer> servers(10);
        TVector<THostAddr> addrs;

        for (const auto& s : servers) {
            addrs.push_back(s.GetHost());
        }

        const auto sender = MakeSender(addrs);
        const TIpRangeMap<size_t> ipMap;

        TVector<TAddr> userAddrs;
        TVector<int> sendCounts(servers.size());

        TReallyFastRng32 random(2);

        for (size_t i = 0; i < servers.size(); ++i) {
            const auto& userAddr = userAddrs.emplace_back(TAddr::FromIp(random.GenRand()));
            ++sendCounts[ChooseInstance(userAddr, 0, servers.size(), ipMap)];
        }

        ShuffleRange(userAddrs, random);

        for (const auto& userAddr : userAddrs) {
            sender->Send(&ipMap, userAddr, dummy).GetValueSync();
        }

        for (size_t i = 0; i < servers.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(servers[i].GetCount(), sendCounts[i]);
        }
    }


    struct TTimeouter : public TRequestReplier {
        TDuration SleepBeforeReply;

        explicit TTimeouter(TDuration sleepBeforeReply)
            : SleepBeforeReply(sleepBeforeReply)
        {}

        bool DoReply(const TReplyParams& params) override {
            Sleep(SleepBeforeReply);
            params.Output << THttpResponse(HTTP_OK).SetContent(NCacheSyncProto::TMessage().SerializeAsString());
            return true;
        }

        TRequestReplier* Clone() const {
            return new TTimeouter(SleepBeforeReply);
        }
    };


    Y_UNIT_TEST(Timeout) {
        const auto timeout = ANTIROBOT_DAEMON_CONFIG.ForwardRequestTimeout;

        TCloneCallback<TTimeouter> callback(MakeHolder<TTimeouter>(timeout * 2));
        TTestServer server(callback);

        const TVector<THostAddr> addrs = {server.Host};
        THttpRequest dummy("GET", "", "/");

        const auto sender = MakeSender(addrs);
        const TIpRangeMap<size_t> ipMap;

        TString answer;
        TError err = sender->Send(&ipMap, TAddr("1.1.1.1"), dummy).ExtractValueSync().PutValueTo(answer);

        UNIT_ASSERT(err.Defined());
        UNIT_ASSERT(err.Is<TForwardingFailure>());
    }


    struct TAlwaysBadRequest : public TRequestReplier {
        bool DoReply(const TReplyParams& params) override {
            params.Output << THttpResponse(HTTP_BAD_REQUEST);
            return true;
        }

        TRequestReplier* Clone() const {
            return new TAlwaysBadRequest;
        }
    };


    Y_UNIT_TEST(CorruptedResponse) {
        TCloneCallback<TAlwaysBadRequest> callback(THolder(new TAlwaysBadRequest));
        TTestServer server(callback);

        const TVector<THostAddr> addrs = {server.Host};
        THttpRequest dummy("GET", "", "/");

        const auto sender = MakeSender(addrs);
        const TIpRangeMap<size_t> ipMap;

        TString answer;
        TError err = sender->Send(&ipMap, TAddr("1.1.1.1"), dummy).ExtractValueSync().PutValueTo(answer);

        UNIT_ASSERT(err.Defined());
        UNIT_ASSERT(err.Is<TForwardingFailure>());
    }


    void SendRequestsWith(
        size_t requestCount,
        const TBackendSender& sender,
        const TIpRangeMap<size_t>* ipMap
    ) {
        THttpRequest dummy("GET", "localhost", "/");

        auto queue = CreateThreadPool(5);

        for (size_t i = 0; i < requestCount; ++i) {
            UNIT_ASSERT(queue->AddFunc([&sender, ipMap, dummy] () {
                sender.Send(ipMap, TAddr("1.1.1.1"), dummy).GetValueSync();
            }));
        }
    }


    Y_UNIT_TEST(DoesntSkipIfRespond) {
        TCountingServer server;
        const auto sender = MakeSender({server.GetHost()});
        const TIpRangeMap<size_t> ipMap;

        const size_t sendCount = 100;
        SendRequestsWith(sendCount, *sender, &ipMap);

        UNIT_ASSERT_VALUES_EQUAL(server.GetCount(), sendCount);
    }


    Y_UNIT_TEST(SkipsIfDoesntRespond) {
        const auto maxSkipProbability = ANTIROBOT_DAEMON_CONFIG.DeadBackendSkipperMaxProbability;

        TCountingServer server;
        const auto sender = MakeSender({server.GetHost()});
        const TIpRangeMap<size_t> ipMap;

        server.Alive = false;
        const size_t sendCount = 1000;
        SendRequestsWith(sendCount, *sender, &ipMap);

        // Отброшенных запросов должно быть порядка maxSkipProbability * sendCount. Но их может
        // оказаться меньше maxSkipProbability * sendCount, так как TDeadBackendSkipper
        // отбрасывается запросы с непостоянной вероятностью. Поэтому мы требуем, чтобы он отбросил
        // хотя бы половину из maxSkipProbability * sendCount запросов.
        auto skippedRequests = sendCount - server.GetCount();
        UNIT_ASSERT(skippedRequests >= 0.5 * maxSkipProbability * sendCount);
    }


    Y_UNIT_TEST(SometimesSendsToDeadBackend) {
        TCountingServer server;
        const auto sender = MakeSender({server.GetHost()});
        const TIpRangeMap<size_t> ipMap;

        server.Alive = false;
        SendRequestsWith(100, *sender, &ipMap);

        auto requestsReceived = server.GetCount();

        SendRequestsWith(100, *sender, &ipMap);

        UNIT_ASSERT(server.GetCount() > requestsReceived);
    }


    Y_UNIT_TEST(SendsEverythingIfStartsToRespond) {
        TCountingServer server;
        const auto sender = MakeSender({server.GetHost()});
        const TIpRangeMap<size_t> ipMap;

        server.Alive = false;
        const size_t sendCount = 100;
        SendRequestsWith(sendCount, *sender, &ipMap);

        // "Оживляем" хост и ждём, когда sender об этом узнает
        server.Alive = true;
        for (
            auto previousReceived = server.GetCount();
            server.GetCount() == previousReceived;
        ) {
            SendRequestsWith(1, *sender, &ipMap);
        }

        // Запоминаем, сколько запросов было фактически отправлено
        auto requestsReceived = server.GetCount();
        SendRequestsWith(sendCount, *sender, &ipMap);
        UNIT_ASSERT(server.GetCount() == requestsReceived + sendCount);
    }


    Y_UNIT_TEST(DoesntGiveUp) {
        TVector<TCountingServer> servers(3);
        TVector<THostAddr> addrs;

        for (const auto& server : servers) {
            addrs.push_back(server.GetHost());
        }

        const auto sender = MakeSender(addrs);
        const TIpRangeMap<size_t> ipMap;

        const TAddr userAddr("192.168.1.183");

        const auto deadIdx = ChooseInstance(userAddr, 0, servers.size(), ipMap);
        servers[deadIdx].Alive = false;

        sender->Send(&ipMap, userAddr, THttpRequest("GET", "localhost", "/"))
            .GetValueSync();

        UNIT_ASSERT_VALUES_EQUAL(servers[deadIdx].GetCount(), 1);

        bool foundNonZeroCount = false;

        for (size_t i = 0; i < servers.size(); ++i) {
            const auto count = servers[i].GetCount();

            if (i != deadIdx && count > 0) {
                UNIT_ASSERT_VALUES_EQUAL(servers[i].GetCount(), 1);

                UNIT_ASSERT(!foundNonZeroCount);
                foundNonZeroCount = true;
            }
        }

        UNIT_ASSERT(foundNonZeroCount);
    }


    template <typename TPorts = TVector<ui16>>
    std::pair<
        NYP::NServiceDiscovery::NApi::TReqResolveEndpoints,
        NYP::NServiceDiscovery::NApi::TRspResolveEndpoints
    > MakeDiscoveryPair(
        const TString& cluster,
        const TString& endpointSetId,
        const TPorts& ports
    ) {
        static size_t endpointIndex = 0;

        NYP::NServiceDiscovery::NApi::TReqResolveEndpoints req;
        req.set_cluster_name(cluster);
        req.set_endpoint_set_id(endpointSetId);

        NYP::NServiceDiscovery::NApi::TRspResolveEndpoints rsp;

        auto& endpointSet = *rsp.mutable_endpoint_set();
        endpointSet.set_endpoint_set_id(endpointSetId);

        for (ui16 port : ports) {
            auto& endpoint = *endpointSet.add_endpoints();
            endpoint.set_id(ToString(endpointIndex));
            endpoint.set_protocol("tcp");
            endpoint.set_fqdn("localhost");
            endpoint.set_ip4_address("127.0.0.1");
            endpoint.set_port(port);
            endpoint.set_ready(true);

            ++endpointIndex;
        }

        return {req, rsp};
    }


    Y_UNIT_TEST(DiscoveryWorks) {
        TCountingServer server;

        const auto [discoveryRequest, discoveryResponse] = MakeDiscoveryPair(
            "vla", "bork",
            {server.GetHost().Port}
        );

        auto sdOptions = MakeSDOptions();
        NYP::NServiceDiscovery::NTesting::TSDServer discoveryServer(sdOptions);
        discoveryServer.Set(discoveryRequest, discoveryResponse);
        discoveryServer.Start();

        auto sender = MakeSender({}, {
            TAntirobotDaemonConfig::TEndpointSetDescription("vla", "bork")
        }, sdOptions.Port);

        sender->GetUpdateEvent().Wait();

        const TIpRangeMap<size_t> ipMap;

        THttpRequest dummy("GET", "localhost", "/");

        sender->Send(&ipMap, TAddr("1.1.1.1"), dummy).ExtractValueSync();
    }


    Y_UNIT_TEST(BackendsGetSorted) {
        TVector<TCountingServer> servers(3);

        auto sdOptions = MakeSDOptions();
        const auto sender = MakeSender(
            CollectHosts(servers),
            {TAntirobotDaemonConfig::TEndpointSetDescription("vla", "bork")},
            sdOptions.Port,
            false,
            true
        );

        UNIT_ASSERT_VALUES_EQUAL(sender->GetHosts(), CollectNames(servers));

        const auto [discoveryRequest, discoveryResponse] =
            MakeDiscoveryPair("vla", "bork", CollectPorts(servers));

        NYP::NServiceDiscovery::NTesting::TSDServer discoveryServer(sdOptions);
        discoveryServer.Set(discoveryRequest, discoveryResponse);
        discoveryServer.Start();
        sender->StartDiscovery();
        sender->GetUpdateEvent().Wait();

        UNIT_ASSERT_VALUES_EQUAL(sender->GetHosts(), CollectNames(servers));
    }


    Y_UNIT_TEST(BackendsGetMerged) {
        TVector<TCountingServer> servers1(2);
        TVector<TCountingServer> servers2(2);

        const auto hostsRange = Concatenate(CollectNames(servers1), CollectNames(servers2));
        const TVector<TString> hosts(hostsRange.begin(), hostsRange.end());

        const auto [discoveryRequest1, discoveryResponse1] = MakeDiscoveryPair(
            "vla", "bork",
            CollectPorts(servers1)
        );

        const auto [discoveryRequest2, discoveryResponse2] = MakeDiscoveryPair(
            "sas", "woof",
            CollectPorts(servers2)
        );

        auto sdOptions = MakeSDOptions();
        NYP::NServiceDiscovery::NTesting::TSDServer discoveryServer(sdOptions);
        discoveryServer.Set(discoveryRequest1, discoveryResponse1);
        discoveryServer.Set(discoveryRequest2, discoveryResponse2);
        discoveryServer.Start();

        auto sender = MakeSender({}, {
            TAntirobotDaemonConfig::TEndpointSetDescription("vla", "bork"),
            TAntirobotDaemonConfig::TEndpointSetDescription("sas", "woof")
        }, sdOptions.Port);

        sender->GetUpdateEvent().Wait();
        UNIT_ASSERT_VALUES_EQUAL(sender->GetHosts(), hosts);
    }
}


} // namespace NAntiRobot
