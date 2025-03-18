package ru.yandex.ci.tms.spring.tasks;

import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.clients.SandboxClientConfig;
import ru.yandex.ci.flow.spring.FlowZkConfig;
import ru.yandex.ci.tms.task.testenv.TestenvZkCleanupCronTask;
import ru.yandex.ci.tms.zk.ZkCleaner;

@Configuration
@Import({
        FlowZkConfig.class,
        SandboxClientConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class TestenvTaskConfig {

    @Bean
    public ZkCleaner zkCleaner(CuratorFramework curatorFramework) {
        return new ZkCleaner(curatorFramework);
    }

    @Bean
    public TestenvZkCleanupCronTask testenvZkCleanupCronTask(
            CuratorFramework curatorFramework,
            ZkCleaner zkCleaner,
            @Value("${ci.testenvZkCleanupCronTask.zkCleanupDir}") String zkCleanupDir) {
        return new TestenvZkCleanupCronTask(curatorFramework, zkCleaner, zkCleanupDir);
    }
}
