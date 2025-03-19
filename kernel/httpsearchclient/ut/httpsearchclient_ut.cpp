#include "httpsearchclient.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

#include <array>

using namespace NHttpSearchClient;

Y_UNIT_TEST_SUITE(THttpSearchClientTest) {
    struct TEnv {
    public:
        explicit TEnv(const TString& script)
            : Script(script)
        {
        }
    public:
        TString Script;
        TBalancingOptions BOptions;
        TString GroupId;

        TClientOptions Opts = {&Script, &BOptions, &GroupId, true};

        TConnGroup ConnGroup;
    };

    struct TUrlTestCase {
        TString Scheme = "";
        TString Host = "";
        ui16 Port = 0;
        TString Path = "";
        TString Url = "";

        TUrlTestCase(const TString& scheme, const TString& host, ui16 port, const TString& path)
            : Scheme(scheme)
            , Host(host)
            , Port(port == 0 && (Scheme == "http" || Scheme == "https") ? 80 : port)
        {
            Path = path + "?";
            if (path == "/") {
                Path = "?";
            }

            Url = TStringBuilder() << Scheme << "://" << Host;
            if (Port) {
                Url = TStringBuilder() << Url << ":" << Port;
            }
            if (path == "/") {
                Url = TStringBuilder() << Url << "/";
            } else if (!path.empty()) {
                Url = TStringBuilder() << Url << "/" << path;
            }
        }
    };

    Y_UNIT_TEST(CreateSimpleClient) {
        TEnv env("http://localhost:1111/yandsearch");

        THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);

        THolder<IConnIterator> it = client->PossibleConnections(0);

        const TConnData* connData = it->Next(0);
        UNIT_ASSERT(connData);
        UNIT_ASSERT(connData->Host() == "localhost");
        UNIT_ASSERT(connData->Ip() == "::1");
        UNIT_ASSERT(connData->Port() == 1111);
        UNIT_ASSERT(!it->Next(1));

        UNIT_ASSERT_VALUES_EQUAL(connData->Address().RealAddress, "http://[::1]:1111/yandsearch");
        UNIT_ASSERT_VALUES_EQUAL(connData->Address().LoggedAddress, "http://localhost:1111/yandsearch");
    }

    Y_UNIT_TEST(CreateSimpleClientUnixSocket) {
        TEnv env("http+unix://[/tmp/unixsocket]/yandsearch");

        THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);

        THolder<IConnIterator> it = client->PossibleConnections(0);

        const TConnData* connData = it->Next(0);
        UNIT_ASSERT(connData);
        UNIT_ASSERT(!connData->HasUnresolvedHost());
        UNIT_ASSERT(connData->Host() == "[/tmp/unixsocket]");
        UNIT_ASSERT(connData->Ip() == "[/tmp/unixsocket]");
        UNIT_ASSERT(connData->Port() == 0);
        UNIT_ASSERT(!it->Next(1));
    }

    Y_UNIT_TEST(CreateWeightedClient) {
        TEnv env("http://localhost:1111/yandsearch@1 http://localhost:2222/yandsearch@2");

        THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);

        THolder<IConnIterator> it = client->PossibleConnections(0);

        TVector<ui32> ports;
        for (size_t i = 0; i < 2; ++i) {
            const TConnData* connData = it->Next(i);
            UNIT_ASSERT(connData);
            UNIT_ASSERT_VALUES_EQUAL(connData->Host(), "localhost");
            UNIT_ASSERT_VALUES_EQUAL(connData->Ip(), "::1");
            ports.push_back(connData->Port());
        }

        Sort(ports.begin(), ports.end());

        TVector<ui32> expected = {1111, 2222};
        UNIT_ASSERT(ports == expected);
        UNIT_ASSERT(!it->Next(2));
    }

    Y_UNIT_TEST(CreateWeightedClientWithGroupIds) {
            TEnv env("((http://localhost:1111/? http://localhost:2222/?@1)group1_1@1 (http://localhost:3333/?)group1_2@1)group1@1 ((http://localhost:4444/?@1)group2_1@1)group2@1 (((http://localhost:5555/?@1)group3_1_1@1)group3_1@1)group3@1");
        THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);

        THolder<IConnIterator> it = client->PossibleConnections(0);

        TVector<std::pair<ui32, TStringBuf>> portsAndDescriptions;
        const size_t expectedNumberOfHosts = 5;
        for (size_t i = 0; i < expectedNumberOfHosts; ++i) {
            const TConnData* connData = it->Next(i);
            UNIT_ASSERT(connData);
            UNIT_ASSERT_VALUES_EQUAL(connData->Host(), "localhost");
            UNIT_ASSERT_VALUES_EQUAL(connData->Ip(), "::1");
            portsAndDescriptions.push_back({connData->Port(), connData->GroupDescr()});
        }

        Sort(portsAndDescriptions.begin(), portsAndDescriptions.end());

        std::array<ui32, expectedNumberOfHosts> expectedPorts = {1111, 2222, 3333, 4444, 5555};
        std::array<TStringBuf, expectedNumberOfHosts> expectedDescriptions = {
            TStringBuf(".group1.group1_1.0"),
            TStringBuf(".group1.group1_1.1"),
            TStringBuf(".group1.group1_2"),
            TStringBuf(".group2.group2_1.0"),
            TStringBuf(".group3.group3_1.group3_1_1.0"),
        };
        for (size_t i = 0; i < expectedNumberOfHosts; ++i) {
            UNIT_ASSERT(portsAndDescriptions[i].first == expectedPorts[i]);
            UNIT_ASSERT(portsAndDescriptions[i].second == expectedDescriptions[i]);
        }
        UNIT_ASSERT(!it->Next(expectedNumberOfHosts));
    }

    Y_UNIT_TEST(CreateDynamicWeightedClient) {
        for (TStringBuf type : {"", "Default", "Pendulum", "Steady", "Robust", "Unknown"}) {

            TEnv env("http://localhost:1111/yandsearch@1 http://localhost:2222/yandsearch@2");
            env.BOptions.AllowDynamicWeights = true;
            env.BOptions.DynBalancerType = type;


            THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);

            THolder<IConnIterator> it = client->PossibleConnections(0);

            size_t i = 0;
            while (it->Next(i)) {
                ++i;
            }

            UNIT_ASSERT_VALUES_EQUAL(i, 2);

            TStringStream ss;
            client->DoReportStats(ss);

            TStringBuf expected;
            if (IsIn({"", "Default", "Unknown"}, type)) {
                expected = "Pendulum";
            } else {
                expected = type;
            }

            UNIT_ASSERT_C(ss.Str().Contains(TStringBuilder() << "<type>" << expected << "</type>"), TStringBuilder() << type << " failed: " << ss.Str());
        }
    }

    Y_UNIT_TEST(CreateFlatWeightedClient) {
        TVector<TGroupData> clients;
        clients.push_back({"http://localhost:1111/yandsearch", "@1", "", "127.0.0.1"});
        clients.push_back({"http://localhost:2222/yandsearch", "@2", "", "127.0.0.5"});
        TEnv env("");

        THolder<THttpSearchClientBase> client = CreateWeightedClient(clients, env.Opts, &env.ConnGroup);

        THolder<IConnIterator> it = client->PossibleConnections(0);

        TVector<ui32> ports;
        for (size_t i = 0; i < 2; ++i) {
            const TConnData* connData = it->Next(i);
            UNIT_ASSERT(connData);
            UNIT_ASSERT(connData->Host() == "localhost");
            ports.push_back(connData->Port());
            if (connData->Port() == 1111) {
                UNIT_ASSERT_VALUES_EQUAL(connData->Ip(), "127.0.0.1");
            } else {
                UNIT_ASSERT_VALUES_EQUAL(connData->Ip(), "127.0.0.5");
            }
        }

        Sort(ports.begin(), ports.end());

        TVector<ui32> expected = {1111, 2222};
        UNIT_ASSERT(ports == expected);
        UNIT_ASSERT(!it->Next(2));
    }

    Y_UNIT_TEST(SearchScriptParse1) {
        TEnv env("http://localhost:1111/yandsearch?cgi1=xxx&cgi2=yyy@1");

        THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);
        THolder<IConnIterator> it = client->PossibleConnections(0);
        const TConnData* connData = it->Next(0);
        UNIT_ASSERT(connData);
        UNIT_ASSERT_VALUES_EQUAL(connData->Host(), "localhost");
        UNIT_ASSERT_VALUES_EQUAL(connData->Path(), "yandsearch?cgi1=xxx&cgi2=yyy&");
        UNIT_ASSERT_VALUES_EQUAL(connData->Port(), 1111);
    }

    Y_UNIT_TEST(SearchScriptParse2) {
        TEnv env("(http://localhost:1111/yandsearch?cgi1=xxx&cgi2=yyy)@1");

        THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);
        THolder<IConnIterator> it = client->PossibleConnections(0);
        const TConnData* connData = it->Next(0);
        UNIT_ASSERT(connData);
        UNIT_ASSERT_VALUES_EQUAL(connData->Host(), "localhost");
        UNIT_ASSERT_VALUES_EQUAL(connData->Path(), "yandsearch?cgi1=xxx&cgi2=yyy&");
        UNIT_ASSERT_VALUES_EQUAL(connData->Port(), 1111);
    }
    Y_UNIT_TEST(MainSource) {
        TEnv env("(http://localhost:1111/?@1)@1 (http://localhost:2222/?@0 http://localhost:3333/?@1)@1 (http://localhost:4444/?@1)@0 ((http://localhost:5555/?@1)@0)@1");

        THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);
        THolder<IConnIterator> it = client->PossibleConnections(0);

        size_t attempt = 0;
        while (const TConnData* connData = it->Next(attempt)) {
            UNIT_ASSERT(connData);
            if (IsIn({1111, 3333}, connData->Port())) {
                UNIT_ASSERT(connData->IsMain());
            } else if (IsIn({2222, 4444, 5555}, connData->Port())) {
                UNIT_ASSERT(!connData->IsMain());
            } else {
                UNIT_ASSERT(false);
            }
        }
    }

    Y_UNIT_TEST(Scheme) {
        TEnv env("http://localhost:1111/? udp://127.0.0.1:2222/? netliba://[::1]:3333/?");

        THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);
        THolder<IConnIterator> it = client->PossibleConnections(0);

        THashMap<size_t, TString> expected = {{1111, "http"}, {2222, "udp"}, {3333, "netliba"}};

        size_t attempt = 0;
        while (const TConnData* connData = it->Next(attempt)) {
            UNIT_ASSERT_VALUES_EQUAL(connData->Scheme(), expected[connData->Port()]);
        }
    }

    Y_UNIT_TEST(SplitToGroups) {
        {
            TString script = "http://localhost:1111/? udp://127.0.0.1:2222/?@1@2 netliba://[::1]:3333/?@1@2@3 sd://xx@zz/? sd://xx@xx/1234 sd://xx@yy@4.3@2@1";

            TVector<TGroupData> groups;
            const bool weighted = SplitToGroups(script, groups, true);
            UNIT_ASSERT(weighted);
            UNIT_ASSERT_VALUES_EQUAL(groups.size(), 6);
            UNIT_ASSERT_VALUES_EQUAL(groups[0].Group, "http://localhost:1111/?");
            UNIT_ASSERT_VALUES_EQUAL(groups[0].GroupWeights, "@1");
            UNIT_ASSERT_VALUES_EQUAL(groups[1].Group, "udp://127.0.0.1:2222/?");
            UNIT_ASSERT_VALUES_EQUAL(groups[1].GroupWeights, "1@2");
            UNIT_ASSERT_VALUES_EQUAL(groups[2].Group, "netliba://[::1]:3333/?");
            UNIT_ASSERT_VALUES_EQUAL(groups[2].GroupWeights, "1@2@3");
            UNIT_ASSERT_VALUES_EQUAL(groups[3].Group, "sd://xx@zz/?");
            UNIT_ASSERT_VALUES_EQUAL(groups[3].GroupWeights, "@1");
            UNIT_ASSERT_VALUES_EQUAL(groups[4].Group, "sd://xx@xx/1234");
            UNIT_ASSERT_VALUES_EQUAL(groups[4].GroupWeights, "@1");
            UNIT_ASSERT_VALUES_EQUAL(groups[5].Group, "sd://xx@yy");
            UNIT_ASSERT_VALUES_EQUAL(groups[5].GroupWeights, "4.3@2@1");
        }
        {
            TString script = "(http://localhost:1111/?)@3@3 (udp://127.0.0.1:2222/?@1@2 netliba://[::1]:3333/?@1@2@3)@5@5 (sd://xx@zz/?@5)";

            TVector<TGroupData> groups;
            const bool weighted = SplitToGroups(script, groups, true);
            UNIT_ASSERT(weighted);
            UNIT_ASSERT_VALUES_EQUAL(groups.size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(groups[0].Group, "http://localhost:1111/?");
            UNIT_ASSERT_VALUES_EQUAL(groups[0].GroupWeights, "@3@3");
            UNIT_ASSERT_VALUES_EQUAL(groups[1].Group, "udp://127.0.0.1:2222/?@1@2 netliba://[::1]:3333/?@1@2@3");
            UNIT_ASSERT_VALUES_EQUAL(groups[1].GroupWeights, "@5@5");
            UNIT_ASSERT_VALUES_EQUAL(groups[2].Group, "sd://xx@zz/?@5");
            UNIT_ASSERT_VALUES_EQUAL(groups[2].GroupWeights, "");
        }

        {
            TString script = "http://localhost:1111/?@3@3";

            TVector<TGroupData> groups;
            const bool weighted = SplitToGroups(script, groups, true);
            UNIT_ASSERT(weighted);

        }

        {
            TString script = "http://localhost:1111/?";

            TVector<TGroupData> groups;
            const bool weighted = SplitToGroups(script, groups, true);
            UNIT_ASSERT(!weighted);
        }
        {
            TString script = "sd://sas@test/?";

            TVector<TGroupData> groups;
            const bool weighted = SplitToGroups(script, groups, true);
            UNIT_ASSERT(!weighted);
        }

        {
            TString script = "(http://localhost:1111/?)first (http://localhost:2222/?@2@2)second (http://localhost:3333/?)third@5";

            TVector<TGroupData> groups;
            const bool weighted = SplitToGroups(script, groups, false);
            UNIT_ASSERT(weighted);
            UNIT_ASSERT_VALUES_EQUAL(groups.size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(groups[0].Group, "http://localhost:1111/?");
            UNIT_ASSERT_VALUES_EQUAL(groups[0].GroupWeights, "first");
            UNIT_ASSERT_VALUES_EQUAL(groups[1].Group, "http://localhost:2222/?@2@2");
            UNIT_ASSERT_VALUES_EQUAL(groups[1].GroupWeights, "second");
            UNIT_ASSERT_VALUES_EQUAL(groups[2].Group, "http://localhost:3333/?");
            UNIT_ASSERT_VALUES_EQUAL(groups[2].GroupWeights, "third@5");
        }

        {
            TString script = "((http://localhost:1111/?)group1_1 (http://localhost:1111/?)group1_2)group1 ((http://localhost:2222/?)group2_1)group2 (((http://localhost:3333/?)group3_1_1)group3_1)group3";

            TVector<TGroupData> groups;
            const bool weighted = SplitToGroups(script, groups, false);
            UNIT_ASSERT(weighted);
            UNIT_ASSERT_VALUES_EQUAL(groups.size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(groups[0].Group, "(http://localhost:1111/?)group1_1 (http://localhost:1111/?)group1_2");
            UNIT_ASSERT_VALUES_EQUAL(groups[0].GroupWeights, "group1");
            UNIT_ASSERT_VALUES_EQUAL(groups[1].Group, "(http://localhost:2222/?)group2_1");
            UNIT_ASSERT_VALUES_EQUAL(groups[1].GroupWeights, "group2");
            UNIT_ASSERT_VALUES_EQUAL(groups[2].Group, "((http://localhost:3333/?)group3_1_1)group3_1");
            UNIT_ASSERT_VALUES_EQUAL(groups[2].GroupWeights, "group3");
        }
    }

    Y_UNIT_TEST(SplitToGroupsOverride) {
        TEnv env("http://localhost:1111/? udp://127.0.0.1:2222/? xxx://[::1]:3333/?");
        env.BOptions.MaxAttempts = 100;

        auto splitter = [](const TString& str, TVector<TGroupData>& groups, bool rawIpAddr) {
            bool weighted = SplitToGroups(str, groups, rawIpAddr);
            if (weighted) {
                const size_t s = groups.size();
                for (size_t i = 0; i < s; ++i) {
                    if (groups[i].Group.StartsWith("xxx://")) {
                        groups[i].Group.replace(0, 3, "netliba");
                    }
                    groups.push_back(groups[i]);
                }
            }
            return weighted;
        };

        THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup, splitter);
        THolder<IConnIterator> it = client->PossibleConnections(0);

        THashMap<size_t, TString> expected = {{1111, "http"}, {2222, "udp"}, {3333, "netliba"}};

        size_t attempt = 0;
        size_t count = 0;
        while (const TConnData* connData = it->Next(attempt)) {
            ++count;
            UNIT_ASSERT_VALUES_EQUAL(connData->Scheme(), expected[connData->Port()]);
        }
        UNIT_ASSERT_VALUES_EQUAL(count, 6);
    }

    Y_UNIT_TEST(TestParseSearchScript) {
        TVector<TString> hosts{"[::1]", "[2a02:6b8:0:1410::5f6c:f3c2]", "localhost"};
        TVector<TString> unixPaths{"[/tmp/unixsocket]"};
        TVector<ui16> ports{0, 12345};
        TVector<TString> services{"", "/", "service"};

        TVector<TUrlTestCase> testCases;

        for (const TString& host : hosts) {
            for (const ui16 port : ports) {
                for (const TString& service : services) {
                    testCases.emplace_back("http", host, port, service);
                }
            }
        }

        for (const TString& unixPath : unixPaths) {
            for (const TString& service : services) {
                testCases.emplace_back("http+unix", unixPath, 0, service);
            }
        }

        for (const auto& testCase : testCases) {
            TString url = testCase.Url;
            TConnGroup::TConnData connData(url, true, true, true, 0, true);

            UNIT_ASSERT_C(connData.Scheme() == testCase.Scheme, TStringBuilder() << connData.Scheme() << " != " << testCase.Scheme);
            if (testCase.Scheme.EndsWith("+unix")) {
                UNIT_ASSERT_C(connData.Ip() == testCase.Host, TStringBuilder() << connData.Ip() << " != " << testCase.Host << " " << url);
            }
            UNIT_ASSERT_C(connData.Host() == testCase.Host, TStringBuilder() << connData.Host() << " != " << testCase.Host << " " << url);
            UNIT_ASSERT_C(connData.Port() == testCase.Port, TStringBuilder() << connData.Port() << " != " << testCase.Port << " " << url);
            UNIT_ASSERT_C(connData.Path() == testCase.Path, TStringBuilder() << connData.Path() << " != " << testCase.Path << " " << url);
        }
    }

    Y_UNIT_TEST(UnresolvedHost) {
        bool catched = false;
        try {
            TEnv env("http://xxx.yyy.1111/yandsearch?cgi1=xxx&cgi2=yyy@1");
            THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);
            UNIT_ASSERT(false);
        } catch (const TNetworkResolutionError&) {
            catched = true;
        } catch (...) {
            UNIT_ASSERT(false);
        }

        UNIT_ASSERT(catched);

        {
            TEnv env("http://xxx.yyy:1111/yandsearch?cgi1=xxx&cgi2=yyy@1");
            env.BOptions.EnableUnresolvedHosts = true;

            THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);
            THolder<IConnIterator> it = client->PossibleConnections(0);
            const TConnData* connData = it->Next(0);
            UNIT_ASSERT(connData);
            UNIT_ASSERT(connData->HasUnresolvedHost());
            UNIT_ASSERT_VALUES_EQUAL(connData->Host(), "xxx.yyy");
            UNIT_ASSERT_VALUES_EQUAL(connData->Path(), "yandsearch?cgi1=xxx&cgi2=yyy&");
            UNIT_ASSERT_VALUES_EQUAL(connData->Port(), 0); //?

            UNIT_ASSERT_VALUES_EQUAL(connData->Address().RealAddress, "http://xxx.yyy:0/yandsearch");
            UNIT_ASSERT_VALUES_EQUAL(connData->Address().LoggedAddress, "http://xxx.yyy:0/yandsearch");
        }
    }

    Y_UNIT_TEST(OptionInheritance) {
        {
            TEnv env("(http://localhost:1111/? http://localhost:2222/?)@1");
            env.BOptions.RandomGroupSelection = true;

            THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);

            THashSet<ui32> ports;
            for (size_t i = 0; i < 100; ++i) {
                THolder<IConnIterator> it = client->PossibleConnections(0);
                const TConnData* connData = it->Next(0);
                UNIT_ASSERT(connData);
                ports.insert(connData->Port());
            }

            UNIT_ASSERT_VALUES_EQUAL(ports.size(), 2);
        }

        {
            TEnv env("(http://localhost:1111/? http://localhost:2222/?)@1");
            env.BOptions.RandomGroupSelection = true;
            env.BOptions.EnableInheritance = false;

            THolder<THttpSearchClientBase> client = CreateClient(env.Opts, &env.ConnGroup);

            THashSet<ui32> ports;
            for (size_t i = 0; i < 100; ++i) {
                THolder<IConnIterator> it = client->PossibleConnections(0);
                const TConnData* connData = it->Next(0);
                UNIT_ASSERT(connData);
                ports.insert(connData->Port());
            }

            UNIT_ASSERT_VALUES_EQUAL(ports.size(), 1);
        }
    }
}
