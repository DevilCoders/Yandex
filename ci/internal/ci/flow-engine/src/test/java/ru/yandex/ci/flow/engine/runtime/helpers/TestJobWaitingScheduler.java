package ru.yandex.ci.flow.engine.runtime.helpers;

import java.time.Instant;
import java.util.ArrayDeque;
import java.util.Queue;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobScheduleTask;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobScheduleTaskParameters;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobWaitingScheduler;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;

@RequiredArgsConstructor
public class TestJobWaitingScheduler implements JobWaitingScheduler {
    @Nonnull
    private final CiDb db;

    private final Queue<SchedulerTriggeredJob> triggeredJobs = new ArrayDeque<>();

    public static class SchedulerTriggeredJob {
        private final FullJobLaunchId jobLaunchId;

        SchedulerTriggeredJob(FullJobLaunchId jobLaunchId) {
            this.jobLaunchId = jobLaunchId;
        }

        public FullJobLaunchId getJobLaunchId() {
            return jobLaunchId;
        }

        @Override
        public String toString() {
            return "SchedulerTriggeredJob{" +
                    "jobLaunchId=" + jobLaunchId +
                    '}';
        }
    }

    @Override
    public void schedule(FullJobLaunchId fullJobLaunchId, @Nullable Instant date) {
        // нужно добавить в тесты, что джоба действительно в статусе ожидания.
        // однако находясь в транзакции ydb это сделать не так просто
        // потому что чтение после обновления таблицы недопустимо
        //TODO checkJobIsWaitingForSchedule (pochemuto)
        triggeredJobs.add(new SchedulerTriggeredJob(fullJobLaunchId));
    }

    @Override
    public void retry(JobScheduleTask jobScheduleTask, Instant date) {
        JobScheduleTaskParameters parameters = (JobScheduleTaskParameters) jobScheduleTask.getParameters();
        FullJobLaunchId fullJobLaunchId = new FullJobLaunchId(
                parameters.getFlowLaunchId(),
                parameters.getJobId(),
                parameters.getJobLaunchNumber()
        );

        schedule(fullJobLaunchId, null);
    }

    @SuppressWarnings("UnusedMethod")
    private void checkJobIsWaitingForSchedule(FullJobLaunchId fullJobLaunchId) {
        FlowLaunchId flowLaunchId = fullJobLaunchId.getFlowLaunchId();
        String jobId = fullJobLaunchId.getJobId();
        FlowLaunchEntity flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId));
        Preconditions.checkNotNull(flowLaunch);
        JobState jobState = flowLaunch.getJobState(jobId);
        JobLaunch lastLaunch = jobState.getLastLaunch();
        Preconditions.checkNotNull(lastLaunch);
        Preconditions.checkState(
                jobState.isWaitingForScheduleChangeType()
        );
    }

    public Queue<SchedulerTriggeredJob> getTriggeredJobs() {
        return triggeredJobs;
    }
}
