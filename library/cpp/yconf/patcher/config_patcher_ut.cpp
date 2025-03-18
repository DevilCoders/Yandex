#include <library/cpp/yconf/patcher/config_patcher.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {
    const TString ZeroDiff = "{ }";

    const TString SampleConfig = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <DOCFETCHER>
                <Stream>
                    DistributorServers:localhost:20108
                    ProxyType : RTYSERVER
                    ShardMin:0
                    ShardMax:0
                </Stream>
                <Stream>
                    DistributorServers:localhost:20109
                    ProxyType : FUSION
                    ShardMin:123
                    ShardMax:321
                    #include ./replicas.conf
                </Stream>
                <Stream>
                    DistributorServers:localhost:30109
                    ProxyType : BLACK
                    ShardMin:1230
                    ShardMax:3210
                </Stream>
                LogFile: ./df.log
                StateFile: ./df.state
            </DOCFETCHER>
            <Synchronizer>
                DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString RemoveDocfetcher = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <Synchronizer>
                DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString RemoveAllStreams = R"_(
        <ModulesConfig>
        #include ./some_module.conf
        <DOCFETCHER>
        LogFile: ./df.log
        StateFile: ./df.state
        </DOCFETCHER>
        <Synchronizer>
        DetachPath:
        </Synchronizer>
        </ModulesConfig>)_";

    const TString Remove2Streams = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <DOCFETCHER>
                <Stream>
                    DistributorServers:localhost:30109
                    ProxyType : BLACK
                    ShardMin:1230
                    ShardMax:3210
                </Stream>
                LogFile: ./df.log
                StateFile: ./df.state
            </DOCFETCHER>
            <Synchronizer>
                DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString PatchAllStreams = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <DOCFETCHER>
                <Stream>
                    DistributorServers:localhost:20108
                    ProxyType : INCORRECT
                    ShardMin:0
                    ShardMax:0
                </Stream>
                <Stream>
                    DistributorServers:localhost:20109
                    ProxyType : INCORRECT
                    ShardMin:123
                    ShardMax:321
                    #include ./replicas.conf
                </Stream>
                <Stream>
                    DistributorServers:localhost:30109
                    ProxyType : INCORRECT
                    ShardMin:1230
                    ShardMax:3210
                </Stream>
                LogFile: ./df.log
                StateFile: ./df.state
            </DOCFETCHER>
            <Synchronizer>
                DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString PatchAllStreamsRemove = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <DOCFETCHER>
                <Stream>
                    DistributorServers:localhost:20108
                    ShardMin:0
                    ShardMax:0
                </Stream>
                <Stream>
                    DistributorServers:localhost:20109
                    ShardMin:123
                    ShardMax:321
                    #include ./replicas.conf
                </Stream>
                <Stream>
                    DistributorServers:localhost:30109
                    ShardMin:1230
                    ShardMax:3210
                </Stream>
                LogFile: ./df.log
                StateFile: ./df.state
            </DOCFETCHER>
            <Synchronizer>
                DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString RemoveDirectiveFromSecondStream = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <DOCFETCHER>
                <Stream>
                    DistributorServers:localhost:20108
                    ProxyType : RTYSERVER
                    ShardMin:0
                    ShardMax:0
                </Stream>
                <Stream>
                    DistributorServers:localhost:20109
                    ShardMin:123
                    ShardMax:321
                    #include ./replicas.conf
                </Stream>
                <Stream>
                    DistributorServers:localhost:30109
                    ProxyType : BLACK
                    ShardMin:1230
                    ShardMax:3210
                </Stream>
                LogFile: ./df.log
                StateFile: ./df.state
            </DOCFETCHER>
                <Synchronizer>
            DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString RemoveIncludeDirectiveFromSecondStream = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <DOCFETCHER>
                <Stream>
                    DistributorServers:localhost:20108
                    ProxyType : RTYSERVER
                    ShardMin:0
                    ShardMax:0
                </Stream>
                <Stream>
                    DistributorServers:localhost:20109
                    ProxyType : FUSION
                    ShardMin:123
                    ShardMax:321
                </Stream>
                <Stream>
                    DistributorServers:localhost:30109
                    ProxyType : BLACK
                    ShardMin:1230
                    ShardMax:3210
                </Stream>
                LogFile: ./df.log
                StateFile: ./df.state
            </DOCFETCHER>
            <Synchronizer>
                DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString AddFourthStream = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <DOCFETCHER>
                <Stream>
                    DistributorServers:localhost:20108
                    ProxyType : RTYSERVER
                    ShardMin:0
                    ShardMax:0
                </Stream>
                <Stream>
                    DistributorServers:localhost:20109
                    ProxyType : FUSION
                    ShardMin:123
                    ShardMax:321
                    #include ./replicas.conf
                </Stream>
                <Stream>
                    DistributorServers:localhost:30109
                    ProxyType : BLACK
                    ShardMin:1230
                    ShardMax:3210
                </Stream>
                <Stream>
                </Stream>
                LogFile: ./df.log
                StateFile: ./df.state
            </DOCFETCHER>
            <Synchronizer>
                DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString PathAllStreamsExcludingFirst = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <DOCFETCHER>
                <Stream>
                    DistributorServers:localhost:20108
                    ProxyType : RTYSERVER
                    ShardMin:0
                    ShardMax:0
                </Stream>
                <Stream>
                    DistributorServers:localhost:20109
                    ProxyType : MapReduce
                    ShardMin:123
                    ShardMax:321
                    #include ./replicas.conf
                </Stream>
                <Stream>
                    DistributorServers:localhost:30109
                    ProxyType : MapReduce
                    ShardMin:1230
                    ShardMax:3210
                </Stream>
                LogFile: ./df.log
                StateFile: ./df.state
            </DOCFETCHER>
            <Synchronizer>
                DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString CreatePath = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <DOCFETCHER>
                <Stream>
                    DistributorServers:localhost:20108
                    ProxyType : RTYSERVER
                    ShardMin:0
                    ShardMax:0
                <TVM>
                    ClientId:42
                </TVM>
                </Stream>
                <Stream>
                    DistributorServers:localhost:20109
                    ProxyType : FUSION
                    ShardMin:123
                    ShardMax:321
                    #include ./replicas.conf
                </Stream>
                <Stream>
                    DistributorServers:localhost:30109
                    ProxyType : BLACK
                    ShardMin:1230
                    ShardMax:3210
                </Stream>
                LogFile: ./df.log
                StateFile: ./df.state
            </DOCFETCHER>
            <Synchronizer>
                DetachPath:
            </Synchronizer>
        </ModulesConfig>)_";

    const TString RemoveDocfetcherAddIndexers = R"_(
        <ModulesConfig>
            #include ./some_module.conf
            <Synchronizer>
                DetachPath:
            </Synchronizer>
            <Indexer>
                <Memory>
                </Memory>
                <Disk>
                    Enabled: true
                </Disk>
            </Indexer>
        </ModulesConfig>)_";

    NConfigPatcher::TOptions options;

    TString Canonize(const TString& string) {
        return NConfigPatcher::Patch(string, ZeroDiff, options);
    }
}

Y_UNIT_TEST_SUITE(TConfigPatcherTests) {
    Y_UNIT_TEST(TestZeroDiff) {
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Diff(SampleConfig, SampleConfig), ZeroDiff);
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Diff(RemoveDocfetcher, RemoveDocfetcher), ZeroDiff);
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Diff(RemoveAllStreams, RemoveAllStreams), ZeroDiff);
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Diff(Remove2Streams, Remove2Streams), ZeroDiff);
    }

    Y_UNIT_TEST(TestRemoveSection) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER\": \"__remove__\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(RemoveDocfetcher));
    }

    Y_UNIT_TEST(TestRemoveAllSection) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER.Stream\": \"__remove_all__\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(RemoveAllStreams));
    }

    Y_UNIT_TEST(TestRemove2Sections) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER.Stream[0:1]\": \"__remove__\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(Remove2Streams));
    }

    Y_UNIT_TEST(TestEndToEnd) {
        {
            const TString patch = NConfigPatcher::Diff(SampleConfig, Remove2Streams);
            UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(Remove2Streams));
        }
        {
            const TString patch = NConfigPatcher::Diff(SampleConfig, RemoveDocfetcherAddIndexers);
            UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(RemoveDocfetcherAddIndexers));
        }
    }

    Y_UNIT_TEST(TestPathAllStreams) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER.Stream[:].ProxyType\": \"INCORRECT\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(PatchAllStreams));
    }

    Y_UNIT_TEST(TestPathAllStreamsRemove) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER.Stream[:].ProxyType\": \"__remove__\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(PatchAllStreamsRemove));
    }

    Y_UNIT_TEST(TestRemoveDirectiveFromSecondStream) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER.Stream[1].ProxyType\": \"__remove__\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(RemoveDirectiveFromSecondStream));
    }

    Y_UNIT_TEST(TestRemoveIncludeDirectiveFromSecondStream) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER.Stream[1].__INCLUDE__\": \"__remove__\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(RemoveIncludeDirectiveFromSecondStream));
    }

    Y_UNIT_TEST(TestAddFourthStream) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER.Stream[3]\": \"__add_section__\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(AddFourthStream));
    }

    Y_UNIT_TEST(TestPathAllStreamsExcludingFirst) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER.Stream[1:].ProxyType\": \"MapReduce\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(PathAllStreamsExcludingFirst));
    }

    Y_UNIT_TEST(TestCreatePath) {
        const TString patch = "{ \"ModulesConfig.DOCFETCHER.Stream.TVM.ClientId\": \"42\"}";
        UNIT_ASSERT_VALUES_EQUAL(NConfigPatcher::Patch(SampleConfig, patch, options), Canonize(CreatePath));
    }
}
