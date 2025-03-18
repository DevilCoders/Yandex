package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import com.google.common.base.Throwables;
import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public class SubscribersFailedEvent extends JobLaunchEvent {
    private static final Set<StatusChangeType> SUPPORTED_STATUSES =
            Set.of(
                    StatusChangeType.EXECUTOR_SUCCEEDED,
                    StatusChangeType.EXECUTOR_FAILED,
                    StatusChangeType.EXECUTOR_EXPECTED_FAILED,
                    StatusChangeType.FORCED_EXECUTOR_SUCCEEDED,
                    StatusChangeType.INTERRUPTING,
                    StatusChangeType.EXECUTOR_INTERRUPTED,
                    StatusChangeType.EXECUTOR_KILLED,
                    StatusChangeType.SUBSCRIBERS_FAILED
            );

    private final Exception subscriberException;

    public SubscribersFailedEvent(String jobId, int jobLaunchNumber, Exception subscriberException) {
        super(jobId, jobLaunchNumber, SUPPORTED_STATUSES);
        this.subscriberException = subscriberException;
    }

    public Exception getSubscriberException() {
        return subscriberException;
    }

    @Override
    public StatusChange createNextStatusChange() {
        return StatusChange.subscribersFailed();
    }

    @Override
    public void afterStatusChange(JobLaunch jobLaunch, JobState jobState) {
        jobLaunch.setSubscriberExceptionStacktrace(Throwables.getStackTraceAsString(this.subscriberException));
    }
}
