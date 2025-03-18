package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public class ExecutorInterruptingEvent extends JobLaunchEvent {
    private static final Set<StatusChangeType> SUPPORTED_STATUSES = Set.of(
            StatusChangeType.RUNNING
    );

    private final String interruptedBy;

    public ExecutorInterruptingEvent(String jobId, int jobLaunchNumber, String interruptedBy) {
        super(jobId, jobLaunchNumber, SUPPORTED_STATUSES);
        this.interruptedBy = interruptedBy;
    }

    public String getInterruptedBy() {
        return interruptedBy;
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.interrupting();
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobLaunch.setInterruptedBy(this.interruptedBy);
    }
}
