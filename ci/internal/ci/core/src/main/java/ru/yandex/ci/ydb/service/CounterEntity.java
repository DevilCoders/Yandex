package ru.yandex.ci.ydb.service;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value(staticConstructor = "of")
@Table(name = "main/Counter")
public class CounterEntity implements Entity<CounterEntity> {
    public static final String DEFAULT_NAMESPACE = "default";

    Id id;

    @Column(dbType = DbType.INT64)
    long value;

    @Override
    public Id getId() {
        return id;
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<CounterEntity> {
        @Column(name = "ns", dbType = DbType.UTF8)
        String ns;

        @Column(name = "key", dbType = DbType.UTF8)
        String key;

        public static Id of(Enum<?> key) {
            return new Id(DEFAULT_NAMESPACE, key.name());
        }
    }

    public CounterEntity increment() {
        return CounterEntity.of(id, value + 1);
    }

    public CounterEntity increment(int amount) {
        return CounterEntity.of(id, value + amount);
    }

    public CounterEntity withLowerBound(long min) {
        return CounterEntity.of(id, Math.max(min, value));
    }
}
