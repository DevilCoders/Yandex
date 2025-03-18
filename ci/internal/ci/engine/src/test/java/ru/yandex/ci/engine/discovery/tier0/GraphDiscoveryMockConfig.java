package ru.yandex.ci.engine.discovery.tier0;

import java.time.Clock;
import java.time.Duration;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.engine.spring.AutocheckBlacklistConfig;
import ru.yandex.ci.engine.spring.DiscoveryProgressConfig;
import ru.yandex.ci.engine.spring.clients.StaffClientTestConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.mockito.Mockito.mock;

@Configuration
@Import({
        CommonTestConfig.class,
        AutocheckBlacklistConfig.class,
        DiscoveryProgressConfig.class,
        StaffClientTestConfig.class,
        GraphDiscoveryMockConfig.Config.class,
})
public class GraphDiscoveryMockConfig {

    @Bean
    public SandboxClient sandboxClient() {
        return mock(SandboxClient.class);
    }

    @Bean
    public BazingaTaskManager bazingaTaskManager() {
        return mock(BazingaTaskManager.class);
    }

    @Bean
    public GraphDiscoveryService graphDiscoveryService(
            DiscoveryProgressService discoveryProgressService,
            CiMainDb db,
            Clock clock,
            SandboxClient sandboxClient,
            BazingaTaskManager bazingaTaskManager,
            GraphDiscoveryServiceProperties graphDiscoveryServiceProperties,
            AutocheckBlacklistService autocheckBlacklistService
    ) {
        return new GraphDiscoveryService(
                discoveryProgressService,
                db,
                sandboxClient,
                bazingaTaskManager,
                clock,
                graphDiscoveryServiceProperties,
                autocheckBlacklistService
        );
    }

    @Bean
    public GraphDiscoveryServiceProperties graphDiscoveryServiceProperties() {
        return GraphDiscoveryServiceProperties.builder()
                .enabled(true)
                .sandboxTaskType("GRAPH_DISCOVERY_FAKE_TASK")
                .sandboxTaskOwner("sandboxTaskOwner")
                .useDistbuildTestingCluster(true)
                .secretWithYaToolToken("secretWithYaToolToken")
                .distBuildPool("//sas_gg/autocheck/precommits/public")
                .delayBetweenSandboxTaskRestarts(Duration.ofSeconds(30))
                .build();
    }

    @Configuration
    public static class Config {
        @Bean
        public ArcService arcService() {
            return new ArcServiceStub(
                    "test-repos/autocheck-blacklist",
                    TestData.TRUNK_COMMIT_2,
                    TestData.TRUNK_COMMIT_3,
                    TestData.TRUNK_COMMIT_4
            );
        }

    }

}
