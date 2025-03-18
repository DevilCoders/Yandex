package ru.yandex.ci.flow.engine.runtime.helpers;

import java.util.ArrayDeque;
import java.util.Queue;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.Value;

import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.JobScheduler;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@RequiredArgsConstructor
public class TestJobScheduler implements JobScheduler {
    @Nonnull
    private final CiDb db;

    private final Queue<TriggeredJob> triggeredJobs = new ArrayDeque<>();
    private final Queue<TriggeredFlow> triggeredFlows = new ArrayDeque<>();

    @Value
    public static class TriggeredJob {
        @Nonnull
        FullJobLaunchId jobLaunchId;
    }

    @Value
    public static class TriggeredFlow {
        @Nonnull
        FlowLaunchId flowLaunchId;
    }

    @Override
    public void scheduleFlow(String project,
                             LaunchId launchId,
                             FullJobLaunchId jobLaunchId) {
        // Эта проверка нужна, чтобы убедиться, что исполнитель джобы
        // не увидит флоу в несохранённом состоянии. Проще говоря,
        // пересчёт состояния НЕ должен работать так:
        // 1. пересчёт состояния
        // 2. планирование джоб
        // 3. сохранение состояния

//        checkJobIsQueuedOrForcedExecutorSucceeded(jobLaunchId);

        triggeredJobs.add(new TriggeredJob(jobLaunchId));
    }

    @Override
    public void scheduleStageRecalc(String project, FlowLaunchId flowLaunchId) {
        triggeredFlows.add(new TriggeredFlow(flowLaunchId));
    }

    @SuppressWarnings("UnusedMethod")
    private void checkJobIsQueuedOrForcedExecutorSucceeded(FullJobLaunchId jobLaunchId) {
        FlowLaunchId flowLaunchId = jobLaunchId.getFlowLaunchId();
        String jobId = jobLaunchId.getJobId();
        FlowLaunchEntity flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId));
        Preconditions.checkNotNull(flowLaunch);
        JobState jobState = flowLaunch.getJobState(jobId);
        JobLaunch lastLaunch = jobState.getLastLaunch();
        Preconditions.checkNotNull(lastLaunch);
        Preconditions.checkState(
                lastLaunch.getLastStatusChange().getType().equals(StatusChangeType.QUEUED)
                        || lastLaunch.getLastStatusChange().getType().equals(StatusChangeType.FORCED_EXECUTOR_SUCCEEDED)
        );
    }

    public Queue<TriggeredJob> getTriggeredJobs() {
        return triggeredJobs;
    }

    public Queue<TriggeredFlow> getTriggeredFlows() {
        return triggeredFlows;
    }
}
