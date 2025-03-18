package ru.yandex.ci.flow.engine.runtime.state;

import java.util.List;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.JobStateChangedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobTaskStateChangeEvent;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

@Slf4j
@RequiredArgsConstructor
public class JobProgressService {
    @Nonnull
    private final FlowStateService stateService;
    @Nonnull
    private final CiDb db;

    /**
     * Checks that one of the following parameters changed latest launch of given jobContext.
     *
     * @param jobContext    verifiable jobContext
     * @param statusText    new status value
     * @param totalProgress new progress value
     * @param taskBadges    new or updated tasks state list
     * @return {@code true} when one of parameters differs from latest launch params,
     * otherwise returns {@code false}
     */
    private static boolean hasChanges(
            JobContext jobContext,
            String statusText,
            Float totalProgress,
            @Nonnull List<TaskBadge> taskBadges
    ) {
        var lastLaunch = jobContext.getJobState().getLastLaunch();
        if (lastLaunch == null) {
            return false;
        }
        return hasChanges(
                lastLaunch,
                statusText,
                totalProgress,
                taskBadges
        );
    }

    private static boolean hasChanges(
            JobLaunch lastLaunch,
            String statusText,
            Float totalProgress,
            @Nonnull List<TaskBadge> taskBadges
    ) {
        return !Objects.equals(lastLaunch.getStatusText(), statusText)
                || !Objects.equals(lastLaunch.getTotalProgress(), totalProgress)
                || tasksDiffers(lastLaunch.getTaskStates(), taskBadges);
    }

    /**
     * Checks two {@code TaskBadge} lists for differences.
     *
     * @param prevStates previous states of last launch
     * @param newStates  new states of last launch
     * @return {@code true} when lists contains any difference in states
     * otherwise returns {@code false}
     */
    private static boolean tasksDiffers(@Nonnull List<TaskBadge> prevStates, @Nonnull List<TaskBadge> newStates) {
        if (prevStates.size() != newStates.size()) {
            return true;
        }
        for (int i = 0; i < prevStates.size(); i++) {
            TaskBadge left = prevStates.get(i);
            TaskBadge right = newStates.get(i);
            if (!left.equals(right)
                    || left.getStatus() != right.getStatus()
                    || !Objects.equals(left.getProgress(), right.getProgress())) {
                return true;
            }
        }
        return false;
    }

    /**
     * Updates state of last job launch if there are upcoming changes.
     *
     * @param jobContext    context of changeable job
     * @param statusText    message that represent current job status
     * @param totalProgress float from 0.0...1.0, that means part of completed work
     * @param taskBadges    zero or more subtask states of launched job
     */

    public FlowLaunchEntity changeProgress(
            JobContext jobContext,
            String statusText,
            Float totalProgress,
            @Nonnull List<TaskBadge> taskBadges
    ) {
        if (hasChanges(jobContext, statusText, totalProgress, taskBadges)) {
            var lastLaunch = jobContext.getJobState().getLastLaunch();
            Preconditions.checkState(lastLaunch != null, "lastLaunch cannot be null at this point");
            int latestJobLaunchNumber = lastLaunch.getNumber();

            JobStateChangedEvent event = new JobStateChangedEvent(
                    jobContext.getJobState().getJobId(),
                    latestJobLaunchNumber,
                    statusText,
                    totalProgress,
                    taskBadges
            );
            stateService.recalc(jobContext.getFlowLaunch().getFlowLaunchId(), event);
        }
        return jobContext.getFlowLaunch();
    }

    public FlowLaunchEntity changeTaskBadge(FullJobLaunchId jobLaunchId, TaskBadge taskBadge) {
        log.info("Updating task badge: {}", taskBadge);

        FlowLaunchId flowLaunchId = jobLaunchId.getFlowLaunchId();
        FlowLaunchEntity flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId));

        JobLaunch lastLaunch = flowLaunch.getJobState(jobLaunchId.getJobId()).getLastLaunch();
        Optional<TaskBadge> updated = Optional.of(taskBadge);
        Optional<TaskBadge> currentState = lastLaunch != null ?
                lastLaunch.getTaskState(taskBadge.getId()) : Optional.empty();

        if (!currentState.equals(updated)) {
            String jobId = jobLaunchId.getJobId();

            JobTaskStateChangeEvent event = new JobTaskStateChangeEvent(
                    jobId,
                    jobLaunchId.getJobLaunchNumber(),
                    taskBadge
            );

            return stateService.recalc(flowLaunchId, event);
        }

        return flowLaunch;
    }
}
