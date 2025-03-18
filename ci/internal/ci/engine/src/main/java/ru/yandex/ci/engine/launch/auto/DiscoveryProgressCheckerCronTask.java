package ru.yandex.ci.engine.launch.auto;

import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class DiscoveryProgressCheckerCronTask extends CiEngineCronTask {

    private final DiscoveryProgressChecker checker;

    public DiscoveryProgressCheckerCronTask(
            DiscoveryProgressChecker checker,
            Duration runDelay,
            Duration timeout,
            CuratorFramework curator
    ) {
        super(runDelay, timeout, curator);
        this.checker = checker;
    }

    @Override
    public void executeImpl(ExecutionContext executionContext) {
        checker.check();
    }
}
