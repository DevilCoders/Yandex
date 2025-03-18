package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.time.Clock;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.EnumSet;
import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;

import ru.yandex.ci.flow.engine.definition.job.InterruptMethod;
import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.ExecutorInterruptingEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobKilledEvent;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.FlowCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.InterruptJobCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.LaunchAutoReleaseCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.LockAndUnlockStageCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.PendingCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.RemoveFromStageQueueCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.ScheduleCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.UnlockStageCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.WaitingForScheduleCommand;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

/**
 * Вспомогательный класс для {@link FlowStateCalculator}.
 * Собирает команды от {@link JobRuntime}.
 */
class PendingCommandConsumer {
    private static final EnumSet<StatusChangeType> STATUSES_THAT_ALLOW_SCHEDULING = EnumSet.of(
            StatusChangeType.QUEUED, StatusChangeType.SUBSCRIBERS_FAILED, StatusChangeType.FORCED_EXECUTOR_SUCCEEDED
    );

    private final List<PendingCommand> commands = new ArrayList<>();
    private final Clock clock;

    PendingCommandConsumer(Clock clock) {
        this.clock = clock;
    }

    public void scheduleJob(JobState jobState, String flowLaunchId) {
        JobLaunch jobLaunch = jobState.getLastLaunch();
        StatusChangeType lastStatusChangeType = jobLaunch.getLastStatusChangeType();

        Preconditions.checkState(
                STATUSES_THAT_ALLOW_SCHEDULING.contains(lastStatusChangeType),
                "A job launch (flowLaunchId = %s, jobId = %s, jobLaunchNumber = %s) must be either %s " +
                        "in order to be scheduled, got %s",
                flowLaunchId, jobState.getJobId(), jobLaunch.getNumber(), STATUSES_THAT_ALLOW_SCHEDULING,
                lastStatusChangeType
        );

        FullJobLaunchId jobLaunchId = new FullJobLaunchId(
                flowLaunchId, jobState.getJobId(), jobLaunch.getNumber()
        );
        commands.add(new ScheduleCommand(jobLaunchId));
    }

    public void waitingForScheduleJob(JobState jobState, String flowLaunchId) {
        JobLaunch jobLaunch = jobState.getLastLaunch();
        StatusChangeType lastStatusChangeType = jobLaunch.getLastStatusChangeType();

        Preconditions.checkState(jobState.isWaitingForScheduleChangeType(),
                "A job launch (flowLaunchId = %s, jobId = %s, jobLaunchNumber = %s) must be either %s " +
                        "in order to be scheduled, got %s",
                flowLaunchId, jobState.getJobId(), StatusChangeType.WAITING_FOR_SCHEDULE,
                lastStatusChangeType
        );

        FullJobLaunchId jobLaunchId = new FullJobLaunchId(
                flowLaunchId, jobState.getJobId(), jobLaunch.getNumber()
        );

        var scheduleTime = calcNextScheduleTime(jobState);
        jobLaunch.setScheduleTime(scheduleTime);
        commands.add(new WaitingForScheduleCommand(jobLaunchId, scheduleTime));
    }

    @Nullable
    private Instant calcNextScheduleTime(JobState state) {
        var retry = state.getRetry();
        if (retry == null || retry.getInitialBackoff() == null) {
            return null;
        }

        var backoff = retry.getBackoff();
        var timeout = backoff.next(retry.getInitialBackoff(), state.getAttempt(), retry.getMaxBackoff());
        return Instant.now(clock).plus(timeout);
    }

    public void unlockStage(StageRef stageToUnlock, FlowLaunchEntity flowLaunch) {
        commands.add(new UnlockStageCommand(stageToUnlock, flowLaunch));
        commands.add(new LaunchAutoReleaseCommand(flowLaunch));
    }

    public void unlockAndLockStage(Collection<? extends StageRef> stagesToUnlock,
                                   @Nonnull StageRef stageToLock,
                                   FlowLaunchEntity flowLaunch, boolean skipStagesAllowed) {
        commands.add(new LockAndUnlockStageCommand(stagesToUnlock, stageToLock, flowLaunch, skipStagesAllowed));
    }

    public void removeFromStageQueue(FlowLaunchEntity flowLaunch) {
        commands.add(new RemoveFromStageQueueCommand(flowLaunch));
        commands.add(new LaunchAutoReleaseCommand(flowLaunch));
    }

    public void interruptJob(FullJobLaunchId fullJobLaunchId) {
        commands.add(new InterruptJobCommand(fullJobLaunchId, InterruptMethod.INTERRUPT));
    }

    public void killJob(FullJobLaunchId fullJobLaunchId) {
        commands.add(new InterruptJobCommand(fullJobLaunchId, InterruptMethod.KILL));
    }

    public void killJob(FlowLaunchId flowLaunchId, JobKilledEvent jobKilledEvent) {
        commands.add(new FlowCommand(flowLaunchId, jobKilledEvent));
    }

    public void executorInterruptingJob(FlowLaunchId flowLaunchId,
                                        ExecutorInterruptingEvent executorInterruptingEvent) {
        commands.add(new FlowCommand(flowLaunchId, executorInterruptingEvent));
    }

    public List<PendingCommand> getCommands() {
        return commands;
    }
}
