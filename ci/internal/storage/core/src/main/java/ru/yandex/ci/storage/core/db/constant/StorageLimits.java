package ru.yandex.ci.storage.core.db.constant;


public class StorageLimits {
    public static final int ITERATIONS_LIMIT = 255;
    public static final int TASKS_LIMIT = 16536;

    // We don't expect more than 100 000 large tests (suits)
    // We'll split it for multiple iteration
    public static final int MAX_LARGE_TESTS = 100_000;

    private StorageLimits() {

    }
}
