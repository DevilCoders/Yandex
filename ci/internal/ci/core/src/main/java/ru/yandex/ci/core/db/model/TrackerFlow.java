package ru.yandex.ci.core.db.model;

import java.time.Instant;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchId;

@Value(staticConstructor = "of")
@Table(name = "main/TrackerFlow")
public class TrackerFlow implements Entity<TrackerFlow> {

    Id id;
    int launchNumber;
    Instant statusUpdated;

    @Override
    public Id getId() {
        return id;
    }

    public LaunchId toLaunchId() {
        return LaunchId.of(id.toCiProcessId(), launchNumber);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<TrackerFlow> {
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "issueKey", dbType = DbType.UTF8)
        String issueKey;

        public static Id of(CiProcessId ciProcessId, String issueKey) {
            return of(ciProcessId.asString(), issueKey);
        }

        public CiProcessId toCiProcessId() {
            return CiProcessId.ofString(processId);
        }
    }
}
