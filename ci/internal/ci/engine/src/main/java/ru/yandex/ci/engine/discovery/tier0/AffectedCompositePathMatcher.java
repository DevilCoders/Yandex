package ru.yandex.ci.engine.discovery.tier0;

import java.nio.file.Path;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import javax.annotation.Nonnull;

import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.discovery.TriggeredProcesses;

public class AffectedCompositePathMatcher implements AffectedPathMatcher {

    @Nonnull
    private final AffectedPathMatcher[] matchers;

    public AffectedCompositePathMatcher(AffectedPathMatcher... matchers) {
        this.matchers = matchers;
    }

    @Override
    public void accept(GraphDiscoveryTask.Platform affectedPlatform, Path affectedPath) throws GraphDiscoveryException {
        for (var matcher : matchers) {
            matcher.accept(affectedPlatform, affectedPath);
        }
    }

    @Override
    public void flush() {
        for (var matcher : matchers) {
            matcher.flush();
        }
    }

    @Override
    public Set<TriggeredProcesses.Triggered> getTriggered() {
        var size = Arrays.stream(matchers).mapToInt(it -> it.getTriggered().size()).sum();
        var triggered = new HashSet<TriggeredProcesses.Triggered>(size);
        for (var matcher : matchers) {
            triggered.addAll(matcher.getTriggered());
        }
        return triggered;
    }
}
