package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public abstract class JobLaunchEvent extends JobEvent {
    protected final int jobLaunchNumber;

    protected JobLaunchEvent(String jobId, int jobLaunchNumber, Set<StatusChangeType> supportedStatuses) {
        super(jobId, supportedStatuses);
        this.jobLaunchNumber = jobLaunchNumber;
    }

    public int getJobLaunchNumber() {
        return jobLaunchNumber;
    }
}
