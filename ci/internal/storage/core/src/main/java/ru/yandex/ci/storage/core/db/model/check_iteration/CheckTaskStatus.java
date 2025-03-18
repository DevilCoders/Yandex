package ru.yandex.ci.storage.core.db.model.check_iteration;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum CheckTaskStatus {

    // No Large tests should be executed
    NOT_REQUIRED,

    // Discovering is not complete but we don't know if we execute something
    // MAYBE_DISCOVERING -> SCHEDULED -> COMPLETE if have tasks to execute
    // MAYBE_DISCOVERING -> NOT_REQUIRED if no runLargeTestsAfterDiscovery configured
    MAYBE_DISCOVERING,

    // Only if some tests should be executed by autostart or runLargeTestsAfterDiscovery launch
    // DISCOVERING -> SCHEDULED -> COMPLETE
    DISCOVERING,

    // Large tests execution is scheduled
    SCHEDULED,

    // All large tests are started and now executing
    COMPLETE;

    public boolean isDiscovering() {
        return this == DISCOVERING || this == MAYBE_DISCOVERING;
    }
}
