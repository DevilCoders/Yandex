package ru.yandex.ci.core.launch;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@Table(name = "main/LaunchByProcessIdAndStatus")
public class LaunchByProcessIdAndStatus implements Entity<LaunchByProcessIdAndStatus> {

    public static final String STATUS_FIELD = "id.status";

    Id id;

    @Override
    public Id getId() {
        return id;
    }

    public static LaunchByProcessIdAndStatus of(Launch launch) {
        return new LaunchByProcessIdAndStatus(
                LaunchByProcessIdAndStatus.Id.of(launch)
        );
    }


    @Value
    public static class Id implements Entity.Id<LaunchByProcessIdAndStatus> {

        @Column(name = "idx_processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "idx_status", dbType = DbType.UTF8)
        LaunchState.Status status;

        @Column(name = "idx_launchNumber")
        int launchNumber;

        public static LaunchByProcessIdAndStatus.Id of(Launch launch) {
            return new LaunchByProcessIdAndStatus.Id(
                    launch.getId().getProcessId(),
                    launch.getStatus(),
                    launch.getId().getLaunchNumber()
            );
        }
    }

}
