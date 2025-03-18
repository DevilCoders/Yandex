package ru.yandex.ci.storage.core.db.model.groups;

import java.util.Set;

import lombok.Value;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

@Value
@Table(name = "Groups")
public class GroupEntity implements Entity<GroupEntity> {
    Id id;

    Set<String> users;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<GroupEntity> {
        String name;
    }
}
