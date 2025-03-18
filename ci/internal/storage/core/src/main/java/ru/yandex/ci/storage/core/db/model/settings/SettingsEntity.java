package ru.yandex.ci.storage.core.db.model.settings;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

@Value
@Builder(toBuilder = true)
@Table(name = "Settings")
public class SettingsEntity implements Entity<SettingsEntity> {
    public static final SettingsEntity.Id ID = new Id("settings");

    public static final SettingsEntity EMTPY = SettingsEntity.builder()
            .id(ID)
            .reader(ReaderSettings.EMTPY)
            .shard(ShardSettings.EMPTY)
            .build();

    Id id;

    ReaderSettings reader;
    ShardSettings shard;
    ShardSettings postProcessor;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<SettingsEntity> {
        String id;
    }
}
