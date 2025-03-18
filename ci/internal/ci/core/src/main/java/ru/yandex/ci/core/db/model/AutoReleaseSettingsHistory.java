package ru.yandex.ci.core.db.model;

import java.time.Instant;

import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.config.CiProcessId;

@SuppressWarnings("ReferenceEquality")
@Value
@Table(name = "main/AutoReleaseSettingsHistory")
public class AutoReleaseSettingsHistory implements Entity<AutoReleaseSettingsHistory> {

    @With
    Id id;

    @Column
    boolean enabled;
    @Column(dbType = DbType.UTF8)
    String login;
    @Column(dbType = DbType.UTF8)
    String message;

    @Override
    public Id getId() {
        return id;
    }

    public static AutoReleaseSettingsHistory of(
            CiProcessId processId, boolean enabled, String login, String message, Instant timestamp
    ) {
        return new AutoReleaseSettingsHistory(
                Id.of(processId.asString(), timestamp),
                enabled, login, message
        );
    }

    public CiProcessId getCiProcessId() {
        return CiProcessId.ofString(id.processId);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<AutoReleaseSettingsHistory> {

        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;
        @Column(name = "timestamp", dbType = DbType.TIMESTAMP)
        Instant timestamp;
    }

}
