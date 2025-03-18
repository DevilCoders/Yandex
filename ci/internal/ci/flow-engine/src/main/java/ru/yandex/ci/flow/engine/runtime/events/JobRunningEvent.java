package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.TmsTaskId;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public class JobRunningEvent extends JobLaunchEvent {
    private static final Set<StatusChangeType> SUPPORTED_STATUSES = Set.of(
            StatusChangeType.QUEUED
    );

    private final TmsTaskId tmsTaskId;

    public JobRunningEvent(String jobId, int jobLaunchNumber, TmsTaskId tmsTaskId) {
        super(jobId, jobLaunchNumber, SUPPORTED_STATUSES);
        this.tmsTaskId = tmsTaskId;
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.running();
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobLaunch.setTmsTaskId(this.tmsTaskId);
    }
}
