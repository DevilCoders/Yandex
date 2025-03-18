package ru.yandex.ci.engine.launch.auto;


import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class ActionPostCommitCronTask extends CiEngineCronTask {

    private final BinarySearchExecutor binarySearchExecutor;
    private final ActionPostCommitHandler actionPostCommitHandler;

    public ActionPostCommitCronTask(
            BinarySearchExecutor binarySearchExecutor,
            ActionPostCommitHandler actionPostCommitHandler,
            Duration runDelay,
            Duration timeout,
            CuratorFramework curator
    ) {
        super(runDelay, timeout, curator);
        this.binarySearchExecutor = binarySearchExecutor;
        this.actionPostCommitHandler = actionPostCommitHandler;
    }


    @Override
    public void executeImpl(ExecutionContext executionContext) {
        binarySearchExecutor.execute(actionPostCommitHandler);
    }

}
