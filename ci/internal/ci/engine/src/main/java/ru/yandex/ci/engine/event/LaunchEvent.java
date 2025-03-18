package ru.yandex.ci.engine.event;

import java.time.Clock;

import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;

import static ru.yandex.ci.engine.event.LaunchMappers.launchContext;

public class LaunchEvent {

    private final Launch launch;
    private final Clock clock;

    public LaunchEvent(Launch launch, Clock clock) {
        this.launch = launch;
        this.clock = clock;
    }

    public LaunchId getLaunchId() {
        return launch.getLaunchId();
    }

    public LaunchState.Status getLaunchStatus() {
        return launch.getStatus();
    }

    private Event.Builder context(EventType eventType) {
        return launchContext(launch)
                .setTimestampMillis(clock.instant().toEpochMilli())
                .setEventType(eventType);
    }

    public byte[] getData() {
        var context = switch (launch.getStatus()) {
            case DELAYED, POSTPONE -> context(EventType.ET_CREATED).setDelayed(true); // Don't like it though
            case STARTING -> context(EventType.ET_CREATED).setDelayed(false);
            case RUNNING -> context(EventType.ET_RUNNING);
            case RUNNING_WITH_ERRORS -> context(EventType.ET_RUNNING_WITH_ERRORS);
            case FAILURE -> context(EventType.ET_FINISHED).setSuccess(false);
            case WAITING_FOR_MANUAL_TRIGGER -> context(EventType.ET_WAITING_FOR_MANUAL_TRIGGER);
            case WAITING_FOR_STAGE -> context(EventType.ET_WAITING_FOR_STAGE);
            case WAITING_FOR_SCHEDULE -> context(EventType.ET_WAITING_FOR_SCHEDULE_EVENT);
            case SUCCESS -> context(EventType.ET_FINISHED).setSuccess(true);
            case CANCELED -> context(EventType.ET_CANCELED);
            case IDLE -> context(EventType.ET_IDLE);
            case WAITING_FOR_CLEANUP -> context(EventType.ET_WAITING_FOR_CLEANUP);
            case CLEANING -> context(EventType.ET_CLEANING);
            default -> null;
        };
        return context != null ? context.build().toByteArray() : new byte[0];
    }
}
