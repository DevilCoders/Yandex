package ru.yandex.ci.flow.engine.runtime.events;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;

@ToString
public class ForceSuccessTriggerEvent extends TriggerEvent {
    public ForceSuccessTriggerEvent(String jobId, String triggeredBy) {
        super(jobId, triggeredBy, false);
    }

    @Override
    public boolean doesSupportDisabledJobs() {
        return false;
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.forcedExecutorSucceeded();
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobLaunch.setForceSuccessTriggeredBy(this.getTriggeredBy());
    }

    @Override
    public void checkJobIsInValidState(JobState jobState) {
        if (jobState.isReadyToRun()) {
            super.checkJobIsInValidState(jobState);
        }
    }
}
