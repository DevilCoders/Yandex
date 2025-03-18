package ru.yandex.ci.flow.spring;

import java.time.Duration;

import org.apache.commons.lang3.StringUtils;
import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.CuratorFrameworkFactory;
import org.apache.curator.retry.RetryForever;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.engine.runtime.JobInterruptionService;
import ru.yandex.ci.flow.engine.runtime.state.revision.FlowStateRevisionService;
import ru.yandex.ci.flow.zookeeper.CuratorFactory;
import ru.yandex.commune.zk2.ZkConfiguration;

@Configuration
@Import(ZkConfig.class)
public class FlowZkConfig {

    @Bean(initMethod = "start")
    public CuratorFramework curatorFramework(
            ZkConfiguration zkConfiguration,
            @Value("${ci.curatorFramework.zookeeperPrefix}") String zookeeperPrefix,
            @Value("${ci.curatorFramework.zookeeperTimeout}") Duration zookeeperTimeout
    ) {
        int timeoutMillis = (int) zookeeperTimeout.toMillis();

        return CuratorFrameworkFactory.builder()
                .connectString(zkConfiguration.getConnectionUrl())
                .retryPolicy(new RetryForever(timeoutMillis))
                .connectionTimeoutMs(timeoutMillis)
                .sessionTimeoutMs(timeoutMillis)
                .namespace(StringUtils.stripStart(zookeeperPrefix, "/"))
                .zk34CompatibilityMode(true)
                .dontUseContainerParents() // For ZK 3.4
                .build();
    }

    @Bean
    public CuratorFactory curatorFactory(CuratorFramework curatorFramework) {
        return new CuratorFactory(curatorFramework);
    }

    @Bean
    public FlowStateRevisionService flowStateRevisionService(
            CuratorFramework curatorFramework,
            @Value("${ci.flowStateRevisionService.maxCasAttempts}") int maxCasAttempts,
            @Value("${ci.flowStateRevisionService.sleepBetweenRetriesMillis}") int sleepBetweenRetriesMillis
    ) {
        return new FlowStateRevisionService(curatorFramework, maxCasAttempts, sleepBetweenRetriesMillis);
    }

    @Bean
    public JobInterruptionService jobInterruptionService(CuratorFactory curatorFactory) {
        return new JobInterruptionService(curatorFactory);
    }
}
