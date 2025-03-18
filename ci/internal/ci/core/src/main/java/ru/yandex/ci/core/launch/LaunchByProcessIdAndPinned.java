package ru.yandex.ci.core.launch;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@Table(name = "main/LaunchByProcessIdAndPinned")
public class LaunchByProcessIdAndPinned implements Entity<LaunchByProcessIdAndPinned> {

    Id id;

    @Override
    public Id getId() {
        return id;
    }

    public static LaunchByProcessIdAndPinned of(Launch launch) {
        return new LaunchByProcessIdAndPinned(
                LaunchByProcessIdAndPinned.Id.of(launch)
        );
    }


    @Value
    public static class Id implements Entity.Id<LaunchByProcessIdAndPinned> {

        @Column(name = "idx_processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "idx_pinned")
        boolean pinned;

        @Column(name = "idx_launchNumber")
        int launchNumber;

        public static LaunchByProcessIdAndPinned.Id of(Launch launch) {
            return new LaunchByProcessIdAndPinned.Id(
                    launch.getId().getProcessId(),
                    launch.isPinned(),
                    launch.getId().getLaunchNumber()
            );
        }
    }

}
