package ru.yandex.ci.flow.engine.runtime.state.revision;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.flow.zookeeper.EntityVersionService;

public class FlowStateRevisionService extends EntityVersionService {
    private static final String BASE_PATH = "/flow/launch/";

    public FlowStateRevisionService(CuratorFramework curatorFramework,
                                    int maxCasAttempts, int sleepBetweenRetryMillis) {
        super(curatorFramework, BASE_PATH, maxCasAttempts, sleepBetweenRetryMillis);
    }
}
