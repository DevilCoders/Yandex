package ru.yandex.ci.flow.engine.runtime.events;

import java.time.Instant;
import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.definition.common.ManualTriggerModifications;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;

@ToString
public class EnableJobManualSwitchEvent extends JobEvent {
    private final String modifiedBy;
    private final Instant timestamp;

    public EnableJobManualSwitchEvent(String jobId, String modifiedBy, Instant timestamp) {
        super(jobId, Set.of());
        this.modifiedBy = modifiedBy;
        this.timestamp = timestamp;
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
        if (!jobState.isManualTrigger()) {
            jobState.setManualTrigger(true);
            jobState.setManualTriggerModifications(new ManualTriggerModifications(this.modifiedBy, this.timestamp));
        }
    }
}
