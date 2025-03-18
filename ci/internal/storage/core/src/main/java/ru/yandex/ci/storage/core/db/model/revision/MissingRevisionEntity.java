package ru.yandex.ci.storage.core.db.model.revision;

import lombok.AllArgsConstructor;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@AllArgsConstructor
@Table(name = "MissingRevisions")
public class MissingRevisionEntity implements Entity<MissingRevisionEntity> {
    Id id;

    @Override
    public Id getId() {
        return this.id;
    }

    @Value
    public static class Id implements Entity.Id<MissingRevisionEntity> {
        @Column(dbType = DbType.UINT64)
        Long number;
    }
}
