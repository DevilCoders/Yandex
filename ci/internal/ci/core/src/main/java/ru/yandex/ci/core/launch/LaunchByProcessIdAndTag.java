package ru.yandex.ci.core.launch;

import java.util.List;
import java.util.stream.Collectors;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@Table(name = "main/LaunchByProcessIdAndTag")
public class LaunchByProcessIdAndTag implements Entity<LaunchByProcessIdAndTag> {

    public static final String PROCESS_ID_FIELD = "id.processId";
    public static final String TAG_FIELD = "id.tag";

    LaunchByProcessIdAndTag.Id id;

    @Override
    public LaunchByProcessIdAndTag.Id getId() {
        return id;
    }

    public static List<LaunchByProcessIdAndTag> of(Launch launch) {
        return launch.getTags().stream()
                // `distinct` protects `Launch.createProjections` from duplicates
                .distinct()
                .map(tag -> new LaunchByProcessIdAndTag(
                        new LaunchByProcessIdAndTag.Id(
                                launch.getId().getProcessId(),
                                tag,
                                launch.getId().getLaunchNumber()
                        )
                ))
                .collect(Collectors.toList());
    }


    @Value
    public static class Id implements Entity.Id<LaunchByProcessIdAndTag> {

        @Column(name = "idx_processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "idx_tag", dbType = DbType.UTF8)
        String tag;

        @Column(name = "idx_launchNumber")
        int launchNumber;

    }

}
