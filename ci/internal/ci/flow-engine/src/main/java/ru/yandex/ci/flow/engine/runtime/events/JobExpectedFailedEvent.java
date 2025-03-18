package ru.yandex.ci.flow.engine.runtime.events;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;

@ToString
public class JobExpectedFailedEvent extends JobFailedEvent {
    public JobExpectedFailedEvent(String jobId, int jobLaunchNumber) {
        super(jobId, jobLaunchNumber);
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.expectedFailed();
    }
}
