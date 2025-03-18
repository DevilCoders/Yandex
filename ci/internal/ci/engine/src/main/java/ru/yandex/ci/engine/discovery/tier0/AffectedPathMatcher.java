package ru.yandex.ci.engine.discovery.tier0;

import java.nio.file.Path;
import java.util.Set;

import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.discovery.TriggeredProcesses;

public interface AffectedPathMatcher {
    void accept(GraphDiscoveryTask.Platform affectedPlatform, Path affectedPath) throws GraphDiscoveryException;

    void flush();

    Set<TriggeredProcesses.Triggered> getTriggered();

    static AffectedPathMatcher composite(AffectedPathMatcher... matchers) {
        return new AffectedCompositePathMatcher(matchers);
    }

}
