package ru.yandex.ci.core.launch;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;

@Value
@Table(name = "main/LaunchByProcessIdAndBranch")
public class LaunchByProcessIdAndArcBranch implements Entity<LaunchByProcessIdAndArcBranch> {

    public static final String PROCESS_ID_FIELD = "id.processId";
    public static final String BRANCH_FIELD = "id.branch";

    Id id;

    @Override
    public Id getId() {
        return id;
    }

    public static LaunchByProcessIdAndArcBranch of(Launch launch) {
        return new LaunchByProcessIdAndArcBranch(
                LaunchByProcessIdAndArcBranch.Id.of(launch)
        );
    }


    @Value
    @JsonSerialize(using = GsonJacksonSerializer.class)
    @JsonDeserialize(using = GsonJacksonDeserializer.class)
    public static class Id implements Entity.Id<LaunchByProcessIdAndArcBranch> {

        @Column(name = "idx_processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "idx_branch", dbType = DbType.UTF8)
        String branch;

        @Column(name = "idx_launchNumber")
        int launchNumber;

        public static Id of(Launch launch) {
            return new Id(
                    launch.getId().getProcessId(),
                    launch.getVcsInfo().getRevision().getBranch().asString(),
                    launch.getId().getLaunchNumber()
            );
        }
    }

}
