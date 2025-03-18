package ru.yandex.ci.core.db.model;

import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@SuppressWarnings("ReferenceEquality")
@Table(name = "main/KeyValue")
public class KeyValue implements Entity<KeyValue> {

    Id id;

    @With
    @Column(dbType = DbType.UTF8)
    String value;

    @Override
    public Id getId() {
        return id;
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<KeyValue> {
        @Column(name = "ns", dbType = DbType.UTF8)
        String namespace;

        @Column(name = "key", dbType = DbType.UTF8)
        String key;
    }

}
