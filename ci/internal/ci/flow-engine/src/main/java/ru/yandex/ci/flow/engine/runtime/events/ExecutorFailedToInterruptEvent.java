package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public class ExecutorFailedToInterruptEvent extends JobLaunchEvent {
    private static final Set<StatusChangeType> SUPPORTED_STATUSES = Set.of(
            StatusChangeType.INTERRUPTING
    );

    public ExecutorFailedToInterruptEvent(String jobId, int jobLaunchNumber) {
        super(jobId, jobLaunchNumber, SUPPORTED_STATUSES);
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.running();
    }
}
