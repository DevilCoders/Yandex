package ru.yandex.ci.storage.core.db.model.yt;

import lombok.Value;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

@Value
@Table(name = "YtExport")
public class YtExportEntity implements Entity<YtExportEntity> {
    Id id;

    String lastKeyJson;

    @Override
    public YtExportEntity.Id getId() {
        return id;
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<YtExportEntity> {
        CheckIterationEntity.Id iterationId;
        YtExportEntityType entityType;
    }
}
