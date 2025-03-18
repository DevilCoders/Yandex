package ru.yandex.ci.flow.engine.runtime.state.model;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;
import java.util.stream.Collectors;

import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum LaunchState {
    RUNNING(Status.RUNNING),
    RUNNING_WITH_ERRORS(Status.RUNNING_WITH_ERRORS),
    FAILURE(Status.FAILURE),
    WAITING_FOR_MANUAL_TRIGGER(Status.WAITING_FOR_MANUAL_TRIGGER),
    WAITING_FOR_STAGE(Status.WAITING_FOR_STAGE),
    WAITING_FOR_SCHEDULE(Status.WAITING_FOR_SCHEDULE),
    CLEANING(Status.CLEANING),
    IDLE(Status.IDLE);

    private static final Map<Status, LaunchState> STATUS_MAP;
    private final Status uiStatus;

    LaunchState(Status uiStatus) {
        this.uiStatus = uiStatus;
    }

    /**
     * Статус flowLaunch нельзя однозначно поставить в соответствие статусу launch.
     * В частности это относится к терминальным статусам launch, они достигаются при некоторых состояниях флоу.
     * Это соответствие можно использовать только в простых случаях.
     */
    public Status getLaunchStatus() {
        return uiStatus;
    }

    static {
        STATUS_MAP = Arrays.stream(LaunchState.values())
                .filter(state -> state != IDLE)
                .collect(Collectors.toMap(s -> s.uiStatus, Function.identity()));
    }

    public static Optional<LaunchState> lookup(Status status) {
        return Optional.ofNullable(STATUS_MAP.get(status));
    }

    public static LaunchState fromStatistics(LaunchStatistics statistics) {
        var result = allFromStatistics(statistics);
        if (result.isEmpty()) {
            return IDLE;
        } else {
            return result.get(0); // Get first acceptable
        }
    }

    public static List<LaunchState> allFromStatistics(LaunchStatistics statistics) {
        var result = new ArrayList<LaunchState>(LaunchState.values().length);
        if (statistics.getCleaning() > 0) {
            result.add(LaunchState.CLEANING);
        }
        if (statistics.getRunning() > 0) {
            result.add(statistics.getFailedOnActiveStages() > 0 ?
                    LaunchState.RUNNING_WITH_ERRORS :
                    LaunchState.RUNNING);
        }
        if (statistics.getFailedOnActiveStages() > 0) {
            result.add(FAILURE);
        }
        if (statistics.getWaiting() > 0) {
            result.add(WAITING_FOR_STAGE);
        }
        if (statistics.getWaitingForSchedule() > 0) {
            result.add(WAITING_FOR_SCHEDULE);
        }
        if (statistics.getReady() > 0) {
            result.add(WAITING_FOR_MANUAL_TRIGGER);
        }
        return result;
    }
}
