package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowStateCalculator;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

/**
 * Событие, произошедшее с джобой, которое подаётся на вход
 * {@link FlowStateCalculator}
 * для пересчёта состояния.
 */
@ToString
public abstract class JobEvent implements FlowEvent {
    protected final String jobId;
    protected final Set<StatusChangeType> supportedStatuses;

    protected JobEvent(String jobId, Set<StatusChangeType> supportedStatuses) {
        this.jobId = jobId;
        this.supportedStatuses = supportedStatuses;
    }

    public String getJobId() {
        return jobId;
    }

    public boolean doesSupportStatus(StatusChangeType status) {
        return this.supportedStatuses.contains(status) || this.supportedStatuses.isEmpty();
    }

    public boolean doesSupportDisabledJobs() {
        return true;
    }

    public void checkJobIsInValidState(JobState jobState) {
    }

    public abstract StatusChange createNextStatusChange();

    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
    }
}
