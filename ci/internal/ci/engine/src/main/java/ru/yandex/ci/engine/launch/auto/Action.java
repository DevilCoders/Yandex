package ru.yandex.ci.engine.launch.auto;

public enum Action implements Comparable<Action> {
    LAUNCH_AND_RESCHEDULE,
    LAUNCH_RELEASE,
    SCHEDULE,
    WAIT_COMMITS;

    boolean allowsLaunch() {
        return this == LAUNCH_AND_RESCHEDULE || this == LAUNCH_RELEASE;
    }
}
