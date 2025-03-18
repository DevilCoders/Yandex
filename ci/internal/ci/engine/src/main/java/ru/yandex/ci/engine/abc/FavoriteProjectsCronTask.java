package ru.yandex.ci.engine.abc;

import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.abc.AbcFavoriteProjectsService;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class FavoriteProjectsCronTask extends CiEngineCronTask {

    private final AbcFavoriteProjectsService abcFavoriteProjectsService;

    public FavoriteProjectsCronTask(
            AbcFavoriteProjectsService abcFavoriteProjectsService,
            Duration runDelay,
            Duration timeout,
            CuratorFramework curator
    ) {
        super(runDelay, timeout, curator);
        this.abcFavoriteProjectsService = abcFavoriteProjectsService;
    }

    @Override
    public void executeImpl(ExecutionContext executionContext) {
        abcFavoriteProjectsService.syncFavoriteProjects();
    }

}
