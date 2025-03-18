package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public class SubscribersSucceededEvent extends JobLaunchEvent {
    private static final Set<StatusChangeType> SUPPORTED_STATUSES =
            Set.of(
                    StatusChangeType.EXECUTOR_SUCCEEDED,
                    StatusChangeType.EXECUTOR_FAILED,
                    StatusChangeType.EXECUTOR_EXPECTED_FAILED,
                    StatusChangeType.SUBSCRIBERS_FAILED,
                    StatusChangeType.FORCED_EXECUTOR_SUCCEEDED,
                    StatusChangeType.INTERRUPTING,
                    StatusChangeType.EXECUTOR_INTERRUPTED,
                    StatusChangeType.EXECUTOR_KILLED
            );

    public SubscribersSucceededEvent(String jobId, int jobLaunchNumber) {
        super(jobId, jobLaunchNumber, SUPPORTED_STATUSES);
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.subscribersSucceeded();
    }
}
