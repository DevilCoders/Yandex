package ru.yandex.ci.engine.launch.auto;


import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class LargePostCommitCronTask extends CiEngineCronTask {

    private final BinarySearchExecutor binarySearchExecutor;
    private final LargePostCommitHandler largePostCommitService;

    public LargePostCommitCronTask(
            BinarySearchExecutor binarySearchExecutor,
            LargePostCommitHandler largePostCommitService,
            Duration runDelay,
            Duration timeout,
            CuratorFramework curator
    ) {
        super(runDelay, timeout, curator);
        this.binarySearchExecutor = binarySearchExecutor;
        this.largePostCommitService = largePostCommitService;
    }


    @Override
    public void executeImpl(ExecutionContext executionContext) {
        binarySearchExecutor.execute(largePostCommitService);
    }

}
