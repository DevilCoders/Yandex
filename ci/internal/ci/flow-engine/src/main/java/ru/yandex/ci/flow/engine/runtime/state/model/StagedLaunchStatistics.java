package ru.yandex.ci.flow.engine.runtime.state.model;

import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.stream.Collectors;

import lombok.Value;
import lombok.With;

/**
 * Содержит статистику выполнения в разрезе stage-ов.
 */
@Value(staticConstructor = "of")
public class StagedLaunchStatistics {
    Map<String, SingleStat> stats;

    public static StagedLaunchStatistics fromLaunch(FlowLaunchEntity launch) {
        List<String> allStages = launch.getStages().stream()
                .map(StoredStage::getId)
                .collect(Collectors.toList());

        var stats = allStages.stream()
                .map(stage -> {
                    var jobs = launch.getJobs().entrySet().stream()
                            .filter(e -> e.getValue().getStage() != null &&
                                    stage.equals(e.getValue().getStage().getId()))
                            .collect(Collectors.toMap(Map.Entry::getKey, Map.Entry::getValue));

                    var launchStatistics = launchStatistics(jobs);
                    var stageStatistics = stageStatistics(jobs);

                    return new SingleStat(stage, launchStatistics, stageStatistics);
                })
                .sorted(Comparator
                        .comparingLong((SingleStat s) -> s.stageStatistics.getStarted())
                        .thenComparing(s -> s.stage))
                .collect(Collectors.toList());

        if (stats.isEmpty()) {
            return StagedLaunchStatistics.of(Map.of());
        }

        // Make sure not to intersect stage timings
        Map<String, SingleStat> targetStats = new HashMap<>(stats.size());
        SingleStat prev = null;
        for (var current : stats) {
            if (prev != null) {
                var prevStat = prev.stageStatistics;
                var currentStat = current.stageStatistics;
                if (prevStat.getFinished() > currentStat.getStarted()) {
                    targetStats.put(prev.stage,
                            prev.withStageStatistics(prevStat.withFinished(currentStat.getStarted())));
                } else {
                    targetStats.put(prev.stage, prev);
                }
            }
            prev = current;
        }
        targetStats.put(prev.stage, prev);
        return StagedLaunchStatistics.of(targetStats);
    }


    private static StageStatistics stageStatistics(Map<String, JobState> jobs) {
        long start = Long.MAX_VALUE;
        long end = Long.MIN_VALUE;

        for (var job : jobs.values()) {
            for (var launch : job.getLaunches()) {
                for (StatusChange change : launch.getStatusHistory()) {
                    var timestamp = change.getDate().toEpochMilli();
                    start = Math.min(start, timestamp);
                    end = Math.max(end, timestamp);
                }
            }
        }
        if (start == Long.MAX_VALUE) {
            start = 0;
            end = 0;
        }

        // Will fix 'end' if next stage starts before this one later
        return StageStatistics.of(start, end);
    }

    private static Map<StatusChangeType, Integer> launchStatistics(Map<String, JobState> jobs) {
        Map<StatusChangeType, Integer> launchStatistics = new HashMap<>();
        for (var job : jobs.values()) {
            var lastType = job.getLastStatusChangeType();
            var value = Objects.requireNonNullElse(launchStatistics.get(lastType), 0);
            launchStatistics.put(lastType, value + 1);
        }
        return launchStatistics;
    }

    @SuppressWarnings("ReferenceEquality")
    @Value(staticConstructor = "of")
    public static class SingleStat {
        String stage;
        Map<StatusChangeType, Integer> launchStatistics;
        @With
        StageStatistics stageStatistics;
    }
}
