package ru.yandex.ci.core.project;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.google.common.annotations.VisibleForTesting;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Getter;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchTable;
import ru.yandex.lang.NonNullApi;

@NonNullApi
@AllArgsConstructor
public class ProjectCounters {
    private static final ProjectCounters EMPTY = new ProjectCounters(Map.of(), Map.of());

    @Getter(value = AccessLevel.PACKAGE, onMethod_ = @VisibleForTesting)
    private final Map<ProcessIdBranch, Map<LaunchState.Status, Long>> processIdBranchMap;
    @Getter(value = AccessLevel.PACKAGE, onMethod_ = @VisibleForTesting)
    private final Map<CiProcessId, Map<LaunchState.Status, Long>> processIdMap;

    public ProjectCounters() {
        this(new HashMap<>(), new HashMap<>());
    }

    public void add(LaunchTable.CountByProcessIdAndStatus count) {
        var processId = count.getProcessId();
        processIdBranchMap.computeIfAbsent(
                ProcessIdBranch.of(processId, count.getBranch()),
                k -> new HashMap<>()
        ).merge(count.getStatus(), count.getCount(), Long::sum);

        processIdMap.computeIfAbsent(processId, k -> new HashMap<>())
                .merge(count.getStatus(), count.getCount(), Long::sum);
    }

    public Map<LaunchState.Status, Long> getCount(ProcessIdBranch processIdBranch) {
        var count = processIdBranchMap.getOrDefault(processIdBranch, Map.of());
        return Map.copyOf(count);
    }

    public Map<LaunchState.Status, Long> getCount(CiProcessId processId) {
        var count = processIdMap.getOrDefault(processId, Map.of());
        return Map.copyOf(count);
    }

    public static ProjectCounters empty() {
        return EMPTY;
    }

    public static ProjectCounters create(List<LaunchTable.CountByProcessIdAndStatus> counters) {
        var projectCounters = new ProjectCounters();
        counters.forEach(projectCounters::add);
        return projectCounters;
    }

}
