package ru.yandex.ci.storage.core.db.model.storage_statistics;

import lombok.Value;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

@Value
@Table(name = "StorageStatistics")
public class StorageStatisticsEntity implements Entity<StorageStatisticsEntity> {
    public static final StorageStatisticsEntity EMPTY = new StorageStatisticsEntity(new Id(""), 0);

    Id id;

    double value;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<StorageStatisticsEntity> {
        String name;
    }
}
