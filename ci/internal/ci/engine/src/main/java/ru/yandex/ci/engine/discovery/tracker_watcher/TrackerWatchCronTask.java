package ru.yandex.ci.engine.discovery.tracker_watcher;

import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class TrackerWatchCronTask extends CiEngineCronTask {

    private final TrackerWatcher watcher;

    public TrackerWatchCronTask(
            TrackerWatcher watcher,
            Duration runDelay,
            Duration timeout,
            CuratorFramework curator
    ) {
        super(runDelay, timeout, curator);
        this.watcher = watcher;
    }

    @Override
    public void executeImpl(ExecutionContext executionContext) {
        watcher.process();
    }
}
