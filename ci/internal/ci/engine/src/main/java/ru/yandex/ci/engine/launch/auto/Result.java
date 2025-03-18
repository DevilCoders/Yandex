package ru.yandex.ci.engine.launch.auto;

import java.time.Instant;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.EqualsAndHashCode;
import lombok.Value;

@Value
@EqualsAndHashCode(doNotUseGetters = true)
public class Result {
    @Nonnull
    Action action;
    @Nullable
    Instant scheduledAt;

    public Instant getScheduledAt() {
        Preconditions.checkState(action == Action.SCHEDULE);
        Preconditions.checkNotNull(scheduledAt);
        return scheduledAt;
    }

    public static Result waitCommits() {
        return new Result(Action.WAIT_COMMITS, null);
    }

    public static Result launchRelease() {
        return new Result(Action.LAUNCH_RELEASE, null);
    }

    public static Result launchAndReschedule() {
        return new Result(Action.LAUNCH_AND_RESCHEDULE, null);
    }

    public static Result scheduleAt(Instant instant) {
        return new Result(Action.SCHEDULE, instant);
    }

    @Override
    public String toString() {
        return switch (action) {
            case SCHEDULE -> action + " at " + scheduledAt;
            default -> action.toString();
        };
    }
}
