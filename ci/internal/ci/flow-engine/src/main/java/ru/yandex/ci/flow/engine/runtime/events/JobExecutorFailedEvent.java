package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.base.Throwables;
import lombok.ToString;

import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.tasklet.api.v2.DataModel;

@ToString
public class JobExecutorFailedEvent extends JobLaunchEvent {
    private static final Set<StatusChangeType> SUPPORTED_STATUSES = Set.of(
            StatusChangeType.RUNNING,
            StatusChangeType.QUEUED,
            StatusChangeType.WAITING_FOR_SCHEDULE
    );

    private final ResourceRefContainer producedResources;
    private final Throwable executionException;
    @Nullable
    private final SandboxTaskStatus sandboxTaskStatus;
    @Nullable
    private final DataModel.ErrorCodes.ErrorCode taskletServerError;

    public JobExecutorFailedEvent(
            String jobId,
            int jobLaunchNumber,
            ResourceRefContainer producedResources,
            Throwable executionException
    ) {
        this(jobId, jobLaunchNumber, producedResources, executionException, null, null);
    }

    public JobExecutorFailedEvent(
            String jobId,
            int jobLaunchNumber,
            ResourceRefContainer producedResources,
            Throwable executionException,
            @Nullable SandboxTaskStatus sandboxTaskStatus,
            @Nullable DataModel.ErrorCodes.ErrorCode taskletServerError
    ) {
        super(jobId, jobLaunchNumber, SUPPORTED_STATUSES);
        this.producedResources = producedResources;
        this.executionException = executionException;
        this.sandboxTaskStatus = sandboxTaskStatus;
        this.taskletServerError = taskletServerError;
    }

    public ResourceRefContainer getProducedResources() {
        return producedResources;
    }

    public Throwable getExecutionException() {
        return executionException;
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.executorFailed();
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobLaunch.setProducedResources(producedResources);
        jobLaunch.setExecutionExceptionStacktrace(Throwables.getStackTraceAsString(this.executionException));
        jobLaunch.setSandboxTaskStatus(sandboxTaskStatus);
        jobLaunch.setTaskletServerError(taskletServerError);
    }
}
