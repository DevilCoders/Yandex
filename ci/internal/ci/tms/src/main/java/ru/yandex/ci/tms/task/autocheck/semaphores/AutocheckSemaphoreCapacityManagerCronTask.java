package ru.yandex.ci.tms.task.autocheck.semaphores;

import org.apache.curator.framework.CuratorFramework;
import org.joda.time.DateTimeZone;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.model.Semaphore;
import ru.yandex.ci.flow.engine.runtime.bazinga.AbstractMutexCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleCron;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleWithRetry;

public abstract class AutocheckSemaphoreCapacityManagerCronTask extends AbstractMutexCronTask {
    private final int maxRetries;
    private final boolean dryRun;
    private final SandboxClient sandboxClient;
    private final String cronExpression;
    private final Mode mode;
    private final String semaphoreId;
    private final int targetCapacity;

    protected AutocheckSemaphoreCapacityManagerCronTask(int maxRetries,
                                                        boolean dryRun,
                                                        SandboxClient sandboxClient,
                                                        String cronExpression,
                                                        Mode mode,
                                                        String semaphoreId,
                                                        int targetCapacity,
                                                        CuratorFramework curator) {
        super(curator);
        this.maxRetries = maxRetries;
        this.dryRun = dryRun;
        this.sandboxClient = sandboxClient;
        this.cronExpression = cronExpression;
        this.mode = mode;
        this.semaphoreId = semaphoreId;
        this.targetCapacity = targetCapacity;
    }

    @Override
    public Schedule cronExpression() {
        return new ScheduleWithRetry(new ScheduleCron(cronExpression, DateTimeZone.UTC), maxRetries);
    }

    @Override
    public void executeImpl(ExecutionContext executionContext) {
        if (!dryRun) {
            Semaphore semaphore = sandboxClient.getSemaphore(semaphoreId);

            if ((mode == Mode.INCREASE && semaphore.getCapacity() < targetCapacity)
                    || (mode == Mode.DECREASE && semaphore.getCapacity() > targetCapacity)) {
                sandboxClient.setSemaphoreCapacity(semaphoreId, targetCapacity, "");
            }
        }
    }

    public enum Mode {
        INCREASE,
        DECREASE
    }
}
