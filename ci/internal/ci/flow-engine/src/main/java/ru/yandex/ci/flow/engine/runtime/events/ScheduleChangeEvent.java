package ru.yandex.ci.flow.engine.runtime.events;

import java.util.Set;

import lombok.ToString;

import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

@ToString
public class ScheduleChangeEvent extends JobEvent {
    private static final Set<StatusChangeType> SUPPORTED_STATUSES = Set.of(
            StatusChangeType.WAITING_FOR_SCHEDULE
    );

    public ScheduleChangeEvent(String jobId) {
        super(jobId, SUPPORTED_STATUSES);
    }

    @Override
    public StatusChange createNextStatusChange() {
        return null;
    }
}
