package ru.yandex.ci.engine.notification.xiva;

import java.util.Optional;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.ToString;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;

/**
 * This event informs that release counters which show number of releases in different states are changed
 */
@ToString
public class ProjectStatisticsChangedEvent extends XivaBaseEvent {

    public ProjectStatisticsChangedEvent(String project) {
        super("project@" + project);
    }

    @Override
    public Type getType() {
        return Type.PROJECT_STATISTICS_CHANGED;
    }

    static Optional<ProjectStatisticsChangedEvent> onLaunchStateChanged(@Nonnull Launch updatedLaunch,
                                                                        @Nullable Launch oldLaunch) {
        return onLaunchStateChanged(
                updatedLaunch.getProject(), updatedLaunch.getProcessId(), updatedLaunch.getStatus(),
                oldLaunch != null ? oldLaunch.getStatus() : null
        );
    }

    static Optional<ProjectStatisticsChangedEvent> onLaunchStateChanged(@Nonnull String project,
                                                                        @Nonnull CiProcessId processId,
                                                                        @Nonnull LaunchState.Status newStatus,
                                                                        @Nullable LaunchState.Status oldStatus) {
        if (isTriggered(processId, newStatus, oldStatus)) {
            return Optional.of(new ProjectStatisticsChangedEvent(project));
        }
        return Optional.empty();
    }

    private static boolean isTriggered(@Nonnull CiProcessId processId,
                                       @Nonnull LaunchState.Status newStatus,
                                       @Nullable LaunchState.Status oldStatus) {
        if (!processId.getType().isRelease()) {
            return false;
        }
        return oldStatus == null || StatusCategory.of(newStatus) != StatusCategory.of(oldStatus);
    }

    private enum StatusCategory {
        STARTING,
        RUNNING,
        FAILURE,
        WAITING,
        TERMINAL;

        public static StatusCategory of(LaunchState.Status status) {
            return switch (status) {
                case STARTING -> STARTING;
                case RUNNING, WAITING_FOR_SCHEDULE, CANCELLING, CLEANING -> RUNNING;
                case DELAYED, POSTPONE, WAITING_FOR_MANUAL_TRIGGER, WAITING_FOR_STAGE, WAITING_FOR_CLEANUP -> WAITING;
                case RUNNING_WITH_ERRORS, FAILURE -> FAILURE;
                case SUCCESS, CANCELED, IDLE -> TERMINAL;
            };
        }
    }

}
