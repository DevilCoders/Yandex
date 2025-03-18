package ru.yandex.ci.storage.core.db.model.check_id_generator;

import lombok.Value;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

@Value
@Table(name = "CheckIds")
public class CheckIdGeneratorEntity implements Entity<CheckIdGeneratorEntity> {
    Id id;

    Long value;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<CheckIdGeneratorEntity> {
        long id;
    }
}
