package ru.yandex.ci.engine.discovery;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Predicate;

import javax.annotation.Nonnull;

import com.google.common.collect.ImmutableSet;
import lombok.Value;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.lang.NonNullApi;

import static java.util.stream.Collectors.collectingAndThen;
import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.mapping;
import static java.util.stream.Collectors.toCollection;
import static java.util.stream.Collectors.toSet;

@Value
@NonNullApi
public class TriggeredProcesses {

    @Nonnull
    Set<Triggered> triggered;

    public Map<CiProcessId, Set<FilterConfig>> getActions() {
        return groupTriggeredByProcessId(t -> !t.getProcessId().getType().isRelease());
    }

    public Map<CiProcessId, Set<FilterConfig>> getReleases() {
        return groupTriggeredByProcessId(t -> t.getProcessId().getType().isRelease());
    }

    public Map<Path, TriggeredProcesses> groupByAYamlPath() {
        return triggered.stream()
                .collect(groupingBy(
                        t -> t.getProcessId().getPath(),
                        collectingAndThen(toSet(), TriggeredProcesses::of))
                );
    }

    public Set<Path> getTriggeredAYamls() {
        return triggered.stream()
                .map(t -> t.getProcessId().getPath())
                .collect(toSet());
    }

    private Map<CiProcessId, Set<FilterConfig>> groupTriggeredByProcessId(Predicate<Triggered> predicate) {
        return triggered.stream()
                .filter(predicate)
                .collect(groupingBy(
                        Triggered::getProcessId,
                        mapping(Triggered::getConfig,
                                collectingAndThen(toCollection(LinkedHashSet::new), Collections::unmodifiableSet))
                ));
    }

    public TriggeredProcesses merge(TriggeredProcesses newProcesses) {
        List<Triggered> merged = new ArrayList<>(this.triggered.size() + newProcesses.triggered.size());
        merged.addAll(this.triggered);
        merged.addAll(newProcesses.triggered);
        return of(merged);
    }

    public static TriggeredProcesses empty() {
        return new TriggeredProcesses(Set.of());
    }

    public boolean isEmpty() {
        return triggered.isEmpty();
    }

    public static TriggeredProcesses of(Triggered... triggered) {
        return of(List.of(triggered));
    }

    public static TriggeredProcesses of(@Nonnull Collection<Triggered> triggered) {
        return new TriggeredProcesses(ImmutableSet.copyOf(triggered)); // preserve order just in case
    }

    @Value
    public static class Triggered {
        CiProcessId processId;
        FilterConfig config;
    }
}
