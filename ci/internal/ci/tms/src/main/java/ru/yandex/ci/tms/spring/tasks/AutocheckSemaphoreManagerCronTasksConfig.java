package ru.yandex.ci.tms.spring.tasks;

import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.clients.SandboxClientConfig;
import ru.yandex.ci.flow.spring.FlowZkConfig;
import ru.yandex.ci.tms.task.autocheck.semaphores.pessimized.AutocheckSemaphorePessimizedDecreaseCapacityTask;
import ru.yandex.ci.tms.task.autocheck.semaphores.pessimized.AutocheckSemaphorePessimizedIncreaseCapacityTask;

@Configuration
@Import({SandboxClientConfig.class, FlowZkConfig.class})
public class AutocheckSemaphoreManagerCronTasksConfig {

    @Value("${ci.AutocheckSemaphoreManagerCronTasksConfig.semaphoreId}")
    String semaphoreId;

    @Value("${ci.AutocheckSemaphoreManagerCronTasksConfig.maxRetries}")
    int maxRetries;

    @Value("${ci.AutocheckSemaphoreManagerCronTasksConfig.dryRun}")
    boolean dryRun;

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    AutocheckSemaphorePessimizedIncreaseCapacityTask autocheckSemaphorePessimizedIncreaseCapacity(
            @Value("${ci.autocheckSemaphorePessimizedIncreaseCapacity.cronExpression}") String cronExpression,
            @Value("${ci.autocheckSemaphorePessimizedIncreaseCapacity.targetCapacity}") int targetCapacity,
            SandboxClient sandboxClient,
            CuratorFramework curator
    ) {
        return new AutocheckSemaphorePessimizedIncreaseCapacityTask(
                maxRetries,
                dryRun,
                sandboxClient,
                cronExpression,
                semaphoreId,
                targetCapacity,
                curator);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    AutocheckSemaphorePessimizedDecreaseCapacityTask autocheckSemaphorePessimizedDecreaseCapacity(
            @Value("${ci.autocheckSemaphorePessimizedDecreaseCapacity.cronExpression}") String cronExpression,
            @Value("${ci.autocheckSemaphorePessimizedDecreaseCapacity.targetCapacity}") int targetCapacity,
            SandboxClient sandboxClient,
            CuratorFramework curator
    ) {
        return new AutocheckSemaphorePessimizedDecreaseCapacityTask(
                maxRetries,
                dryRun,
                sandboxClient,
                cronExpression,
                semaphoreId,
                targetCapacity,
                curator);
    }
}
