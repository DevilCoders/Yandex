package ru.yandex.ci.tms.task.autocheck.semaphores.pessimized;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.tms.task.autocheck.semaphores.AutocheckSemaphoreCapacityManagerCronTask;

public class AutocheckSemaphorePessimizedIncreaseCapacityTask extends AutocheckSemaphoreCapacityManagerCronTask {
    public AutocheckSemaphorePessimizedIncreaseCapacityTask(
            int maxRetries,
            boolean dryRun,
            SandboxClient sandboxClient,
            String cronExpression,
            String semaphoreId,
            int targetCapacity,
            CuratorFramework curator) {
        super(maxRetries,
                dryRun,
                sandboxClient,
                cronExpression,
                Mode.INCREASE,
                semaphoreId,
                targetCapacity,
                curator);
    }
}
