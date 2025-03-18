package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import com.google.common.base.Preconditions;
import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public class TriggerEvent extends JobEvent {
    private final String triggeredBy;
    private final boolean shouldRestartIfAlreadyRunning;

    public TriggerEvent(String jobId, String triggeredBy, boolean shouldRestartIfAlreadyRunning) {
        super(jobId, Set.of());
        this.triggeredBy = triggeredBy;
        this.shouldRestartIfAlreadyRunning = shouldRestartIfAlreadyRunning;
    }

    public String getTriggeredBy() {
        return triggeredBy;
    }

    public boolean shouldRestartIfAlreadyRunning() {
        return shouldRestartIfAlreadyRunning;
    }

    @Override
    public boolean doesSupportDisabledJobs() {
        return false;
    }

    @Override
    public void checkJobIsInValidState(JobState jobState) {
        if (shouldRestartIfAlreadyRunning) {
            var lastChange = jobState.getLastStatusChangeType();
            Preconditions.checkState(
                jobState.isReadyToRun()
                        || lastChange == StatusChangeType.RUNNING
                        || (lastChange != null && lastChange.isLikelyToFinishSoon()),
                    "Job {} can't be triggered because isReadyToRun={} and lastStatusChangeType={}",
                    jobState.getJobId(), jobState.isReadyToRun(), jobState.getLastStatusChangeType()
            );
        } else {
            Preconditions.checkState(
                    jobState.isReadyToRun(), "Job must be in readyToRun=true in order to be triggered"
            );
        }
    }

    @Override
    public StatusChange createNextStatusChange() {
        return null;
    }
}
