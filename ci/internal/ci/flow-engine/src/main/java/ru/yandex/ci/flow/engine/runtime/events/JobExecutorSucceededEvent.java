package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public class JobExecutorSucceededEvent extends JobLaunchEvent {
    private static final Set<StatusChangeType> SUPPORTED_STATUSES = Set.of(
            StatusChangeType.RUNNING,
            StatusChangeType.INTERRUPTING,
            StatusChangeType.FORCED_EXECUTOR_SUCCEEDED
    );

    private final ResourceRefContainer producedResources;

    public JobExecutorSucceededEvent(String jobId, int jobLaunchNumber) {
        super(jobId, jobLaunchNumber, SUPPORTED_STATUSES);
        this.producedResources = ResourceRefContainer.empty();
    }

    public JobExecutorSucceededEvent(String jobId, int jobLaunchNumber, ResourceRefContainer producedResources) {
        super(jobId, jobLaunchNumber, SUPPORTED_STATUSES);
        this.producedResources = producedResources;
    }

    public ResourceRefContainer getProducedResources() {
        return producedResources;
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.executorSucceeded();
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobLaunch.setProducedResources(producedResources);
    }

}
