package ru.yandex.ci.flow.engine.runtime.events;

import java.time.Instant;
import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.definition.common.SchedulerConstraintModifications;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;

@ToString
public class ToggleSchedulerConstraintModifyEvent extends JobEvent {
    private final String modifiedBy;
    private final Instant timestamp;

    public ToggleSchedulerConstraintModifyEvent(String jobId, String modifiedBy, Instant timestamp) {
        super(jobId, Set.of());
        this.modifiedBy = modifiedBy;
        this.timestamp = timestamp;
    }

    public String getModifiedBy() {
        return modifiedBy;
    }

    public Instant getTimestamp() {
        return timestamp;
    }

    @Override
    public boolean doesSupportDisabledJobs() {
        return false;
    }

    @Override
    public StatusChange createNextStatusChange() {
        return null;
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobState.setEnableJobSchedulerConstraint(!jobState.isEnableJobSchedulerConstraint());
        jobState.setSchedulerConstraintModifications(
            new SchedulerConstraintModifications(this.modifiedBy, this.timestamp));
    }
}
