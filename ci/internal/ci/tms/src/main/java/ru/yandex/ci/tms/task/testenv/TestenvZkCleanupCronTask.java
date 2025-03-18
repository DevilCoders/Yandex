package ru.yandex.ci.tms.task.testenv;

import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.ci.tms.zk.ZkCleaner;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class TestenvZkCleanupCronTask extends CiEngineCronTask {

    private static final Duration MAX_AGE = Duration.ofMinutes(30);
    private final ZkCleaner zkCleaner;
    private final String zkCleanupDir;

    public TestenvZkCleanupCronTask(CuratorFramework curator, ZkCleaner zkCleaner, String zkCleanupDir) {
        super(Duration.ofMinutes(15), Duration.ofMinutes(30), curator);
        this.zkCleaner = zkCleaner;
        this.zkCleanupDir = zkCleanupDir;
    }

    @Override
    protected void executeImpl(ExecutionContext executionContext) throws Exception {
        zkCleaner.cleanDir(zkCleanupDir, MAX_AGE);
    }

}
