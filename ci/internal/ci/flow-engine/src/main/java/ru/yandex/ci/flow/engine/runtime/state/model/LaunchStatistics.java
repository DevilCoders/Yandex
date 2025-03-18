package ru.yandex.ci.flow.engine.runtime.state.model;

import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@AllArgsConstructor
public class LaunchStatistics {

    public static final LaunchStatistics EMPTY = LaunchStatistics.builder().build();

    long total;
    long running;
    long completed;
    long failed;
    long failedOnActiveStages;
    long cancelled;
    long skipped;
    long ready;
    long waiting;
    long waitingForSchedule;
    long cleaning;

    public static LaunchStatistics fromLaunch(FlowLaunchEntity launch, StageGroupState stageGroupState) {
        Set<String> acquiredStages = stageGroupState.getAcquiredStagesIds(launch.getFlowLaunchId());
        List<String> allStages = launch.getStages().stream()
                .map(StoredStage::getId)
                .collect(Collectors.toList());

        List<String> completedStages;
        if (acquiredStages.isEmpty()) {
            completedStages = allStages;
        } else {
            completedStages = allStages.stream()
                    .limit(acquiredStages.stream().map(allStages::indexOf).min(Integer::compareTo).get())
                    .collect(Collectors.toList());
        }
        return fromLaunch(launch, acquiredStages, completedStages);
    }

    public static LaunchStatistics fromLaunch(FlowLaunchEntity launch, Collection<String> activeStages,
                                              Collection<String> completedStages) {
        List<JobState> visibleJobs = launch.getJobs().values()
                .stream()
                .filter(JobState::isVisible)
                .collect(Collectors.toList());

        List<JobState> jobStates = visibleJobs.stream()
                .filter(job -> !job.isOutdated())
                .collect(Collectors.toList());

        var jobLaunches = jobStates.stream()
                .filter(state -> state.getLastLaunch() != null)
                .collect(Collectors.toMap(JobState::getLastLaunch, Function.identity()));

        //


        long completed = filter(jobLaunches.keySet(), job ->
                job.getLastStatusChangeType() == StatusChangeType.SUCCESSFUL);

        long failed = filter(jobLaunches.keySet(), JobLaunch::isLastStatusChangeTypeFailed);

        long waiting = filter(jobLaunches.keySet(), job ->
                job.getLastStatusChangeType() == StatusChangeType.WAITING_FOR_STAGE);

        long waitingForSchedule = filter(jobLaunches.keySet(), job ->
                job.getLastStatusChangeType() == StatusChangeType.WAITING_FOR_SCHEDULE);

        long cancelled = filter(jobLaunches.keySet(), job ->
                job.getLastStatusChangeType() == StatusChangeType.INTERRUPTED ||
                        job.getLastStatusChangeType() == StatusChangeType.KILLED);

        long running = jobLaunches.size();
        long cleaning = jobLaunches.values().stream()
                .filter(job -> job.getJobType() == JobType.CLEANUP)
                .count();

        //

        long skipped = completedStages.isEmpty() ?
                0 :
                visibleJobs.stream()
                        .filter(x -> x.getLaunches().isEmpty() && x.getStage() != null &&
                                completedStages.contains(x.getStage().getId()))
                        .count();
        long failedOnActiveStages = activeStages.isEmpty() ? failed :
                jobStates.stream()
                        .filter(state -> state.getStage() != null)
                        .filter(state -> activeStages.contains(state.getStage().getId()))
                        .filter(JobState::isLastStatusChangeTypeFailed)
                        .count();

        long ready = visibleJobs.stream()
                .filter(job -> job.isReadyToRun() && (job.getLastLaunch() == null || job.isOutdated()))
                .count();

        return builder()
                .total(visibleJobs.size())
                .running(running)
                .completed(completed)
                .failed(failed)
                .failedOnActiveStages(failedOnActiveStages)
                .cancelled(cancelled)
                .skipped(skipped)
                .ready(ready)
                .waiting(waiting)
                .waitingForSchedule(waitingForSchedule)
                .cleaning(cleaning)
                .build();
    }

    private static long filter(Collection<JobLaunch> launches, Predicate<JobLaunch> filter) {
        var tmp = new Object() {
            long count;
        };
        launches.removeIf(launch -> {
            if (filter.test(launch)) {
                tmp.count++;
                return true;
            } else {
                return false;
            }
        });
        return tmp.count;
    }

}
