package ru.yandex.ci.flow.engine.runtime.state.calculator;

import javax.annotation.Nullable;

import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;

/**
 * Рассчитаный статус джобы.
 */
public class JobStatus {
    @Nullable
    private final StatusChange lastStatusChange;
    private final boolean isOutdated;
    private final boolean isReadyToRun;

    private JobStatus(@Nullable StatusChange lastStatusChange, boolean isOutdated, boolean isReadyToRun) {
        this.lastStatusChange = lastStatusChange;
        this.isOutdated = isOutdated;
        this.isReadyToRun = isReadyToRun;
    }

    public static JobStatus outdated(StatusChange statusChange, boolean readyToRun) {
        return new JobStatus(statusChange, true, readyToRun);
    }

    public static JobStatus actual(StatusChange statusChange, boolean readyToRun) {
        return new JobStatus(statusChange, false, readyToRun);
    }

    public static JobStatus noLaunches(boolean readyToRun) {
        return new JobStatus(null, false, readyToRun);
    }

    /**
     * Джоба является outdated, если соблюдается хотя бы одно условие:
     * <ul>
     * <li>её последний запуск зависит от старых запусков апстримов;</li>
     * <li>один или более апстрим outdated.</li>
     * </ul>
     */
    public boolean isOutdated() {
        return isOutdated;
    }

    public boolean isReadyToRun() {
        return isReadyToRun;
    }

    @Nullable
    public StatusChange getStatusChange() {
        return lastStatusChange;
    }
}
