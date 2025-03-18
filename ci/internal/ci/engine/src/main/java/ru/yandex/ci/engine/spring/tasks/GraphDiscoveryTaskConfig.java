package ru.yandex.ci.engine.spring.tasks;

import java.time.Clock;
import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.sandbox.ProxySandboxClient;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.spring.clients.SandboxProxyClientConfig;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommits;
import ru.yandex.ci.engine.discovery.tier0.ConfigDiscoveryDirCache;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryResultProcessorTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;
import ru.yandex.ci.engine.discovery.tier0.ResultProcessorParameters;
import ru.yandex.ci.engine.spring.DiscoveryConfig;

@Configuration
@Import({
        DiscoveryConfig.class,
        SandboxProxyClientConfig.class,
})
public class GraphDiscoveryTaskConfig {

    @Bean
    public GraphDiscoveryResultProcessorTask graphDiscoveryResultProcessorTask(
            ArcService arcService,
            CiMainDb db,
            Clock clock,
            ConfigurationService configurationService,
            DiscoveryServicePostCommits discoveryServicePostCommits,
            ProxySandboxClient proxySandboxClient,
            RevisionNumberService revisionNumberService,
            SandboxClient sandboxClient,
            GraphDiscoveryService graphDiscoveryService,
            @Value("${ci.graphDiscoveryResultProcessorTask.timeout}") Duration timeout,
            ConfigDiscoveryDirCache configDiscoveryDirCache,
            ResultProcessorParameters graphDiscoveryResultProcessorParameters
    ) {
        return new GraphDiscoveryResultProcessorTask(
                arcService,
                db,
                clock,
                configurationService,
                discoveryServicePostCommits,
                proxySandboxClient,
                revisionNumberService,
                sandboxClient,
                graphDiscoveryService,
                timeout,
                configDiscoveryDirCache,
                graphDiscoveryResultProcessorParameters
        );
    }

    @Bean
    public ConfigDiscoveryDirCache configDiscoveryDirCache(CiMainDb db) {
        return new ConfigDiscoveryDirCache(db, Duration.ofMinutes(1));
    }

    @Bean
    public ResultProcessorParameters graphDiscoveryResultProcessorParameters(
            @Value("${ci.graphDiscoveryResultProcessorTask.sandboxResultResourceType}")
            String sandboxResultResourceType,
            @Value("${ci.graphDiscoveryResultProcessorTask.fileExistsCacheSize}") int fileExistsCacheSize,
            @Value("${ci.graphDiscoveryResultProcessorTask.configBundleCacheSize}") int configBundleCacheSize,
            @Value("${ci.graphDiscoveryResultProcessorTask.batchSize}") int batchSize
    ) {
        return ResultProcessorParameters.builder()
                .sandboxResultResourceType(sandboxResultResourceType)
                .defaultConfigBundleCacheSize(configBundleCacheSize)
                .defaultFileExistsCacheSize(fileExistsCacheSize)
                .defaultBatchSize(batchSize)
                .build();
    }

}
