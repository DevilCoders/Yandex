package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public class ExecutorInterruptedEvent extends JobLaunchEvent {
    private static final Set<StatusChangeType> SUPPORTED_STATUSES = Set.of(
            StatusChangeType.INTERRUPTING
    );

    private final ResourceRefContainer producedResources;

    public ExecutorInterruptedEvent(String jobId, int jobLaunchNumber, ResourceRefContainer producedResources) {
        super(jobId, jobLaunchNumber, SUPPORTED_STATUSES);
        this.producedResources = producedResources;
    }

    public ResourceRefContainer getProducedResources() {
        return producedResources;
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.executorInterrupted();
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobLaunch.setProducedResources(this.producedResources);
    }
}
