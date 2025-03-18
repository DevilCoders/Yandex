package ru.yandex.ci.engine.launch.auto;


import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class PostponeActionCronTask extends CiEngineCronTask {

    private final PostponeActionService postponeActionService;

    public PostponeActionCronTask(
            PostponeActionService postponeActionService,
            Duration runDelay,
            Duration timeout,
            CuratorFramework curator
    ) {
        super(runDelay, timeout, curator);
        this.postponeActionService = postponeActionService;
    }


    @Override
    public void executeImpl(ExecutionContext executionContext) {
        postponeActionService.executePostponeActions((int) getRunDelay().toSeconds());
    }

}
