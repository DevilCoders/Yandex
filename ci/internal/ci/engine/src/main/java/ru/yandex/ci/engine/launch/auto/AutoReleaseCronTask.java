package ru.yandex.ci.engine.launch.auto;


import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class AutoReleaseCronTask extends CiEngineCronTask {

    private final AutoReleaseService autoReleaseService;

    public AutoReleaseCronTask(
            AutoReleaseService autoReleaseService,
            Duration runDelay,
            Duration timeout,
            CuratorFramework curator
    ) {
        super(runDelay, timeout, curator);
        this.autoReleaseService = autoReleaseService;
    }

    @Override
    public void executeImpl(ExecutionContext executionContext) {
        autoReleaseService.processAutoReleaseQueue();
    }

}
