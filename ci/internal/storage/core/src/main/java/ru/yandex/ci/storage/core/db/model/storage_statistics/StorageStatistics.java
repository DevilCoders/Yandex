package ru.yandex.ci.storage.core.db.model.storage_statistics;

public final class StorageStatistics {
    public static final StorageStatisticsEntity.Id ACTIVE_CHECKS = new StorageStatisticsEntity.Id("active_checks");

    public static final StorageStatisticsEntity.Id ACTIVE_ITERATIONS
            = new StorageStatisticsEntity.Id("active_iterations");

    public static final StorageStatisticsEntity.Id ACTIVE_LEFT_TASKS
            = new StorageStatisticsEntity.Id("active_left_tasks");

    public static final StorageStatisticsEntity.Id ACTIVE_RIGHT_TASKS
            = new StorageStatisticsEntity.Id("active_right_tasks");

    private StorageStatistics() {

    }
}
