package ru.yandex.ci.engine.launch;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class AutoReleaseServiceIT extends EngineTestBase {

    @Test
    void createCommitDiscoveryProgressTwiceShouldNotThrowException() {
        OrderedArcRevision arcRevision = OrderedArcRevision.fromHash("1", ArcBranch.trunk(), 1, 1);
        discoveryProgressService.createCommitDiscoveryProgress(arcRevision);
        discoveryProgressService.createCommitDiscoveryProgress(arcRevision);
    }

}
