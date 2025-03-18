package ru.yandex.ci.core.job;

import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.gson.JsonObject;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.tasklet.api.v2.DataModel;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true)
@Table(name = "flow/JobInstance")
public class JobInstance implements Entity<JobInstance> {

    @With
    @Nonnull
    JobInstance.Id id;

    @With
    @Nonnull
    @Column(dbType = DbType.UTF8)
    JobStatus status;

    @Nonnull
    @Column(dbType = DbType.UTF8)
    TaskletRuntime runtime;

    @With
    @Nullable
    @Column(dbType = DbType.JSON, flatten = false)
    JsonObject output;

    @Nullable
    @Column(dbType = DbType.UTF8)
    String errorDescription;

    @Nullable
    Boolean transientError;

    // For Sandbox/Tasklet v1
    @With
    @Column(dbType = DbType.INT64)
    long sandboxTaskId;

    @With
    @Nullable
    @Column(dbType = DbType.UTF8)
    SandboxTaskStatus sandboxTaskStatus;

    // For Tasklet v2

    @Nullable
    @Column(dbType = DbType.UTF8)
    String executionId;

    @Nullable
    @Column(dbType =  DbType.UTF8)
    DataModel.EExecutionStatus taskletStatus;

    @Nullable
    DataModel.ErrorCodes.ErrorCode taskletServerError;

    @With
    @Nullable
    Integer reuseCount; // 0 - task was not reused, 1 - reused once (two launches with same sandbox-id) and so on

    @Nonnull
    @Override
    public JobInstance.Id getId() {
        return id;
    }

    public int getReuseCount() {
        return Objects.requireNonNullElse(reuseCount, 0);
    }

    public JobInstance incReuseCount() {
        return withReuseCount(getReuseCount() + 1);
    }

    public static class Builder {
        public Builder() {
            this.status = JobStatus.CREATED;
            this.runtime = TaskletRuntime.SANDBOX;
            this.output = new JsonObject();
        }
    }

    @SuppressWarnings("ReferenceEquality")
    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<JobInstance> {
        @With
        @Nonnull
        @Column(name = "flowLaunchId", dbType = DbType.UTF8)
        String flowLaunchId;

        @Nonnull
        @Column(name = "jobId", dbType = DbType.UTF8)
        String jobId;

        @With
        @Column(name = "number")
        int number;
    }

}
