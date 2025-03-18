package ru.yandex.ci.engine.abc;

import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.abc.AbcServiceImpl;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class AbcRefreshCronTask extends CiEngineCronTask {

    private final AbcServiceImpl abcService;

    public AbcRefreshCronTask(
            AbcServiceImpl abcService,
            Duration runDelay,
            Duration timeout,
            CuratorFramework curator
    ) {
        super(runDelay, timeout, curator);
        this.abcService = abcService;
    }

    @Override
    public void executeImpl(ExecutionContext executionContext) {
        abcService.syncServices();
    }

}
