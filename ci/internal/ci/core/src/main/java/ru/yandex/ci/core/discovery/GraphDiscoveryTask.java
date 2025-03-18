package ru.yandex.ci.core.discovery;

import java.io.UncheckedIOException;
import java.time.Instant;
import java.util.LinkedHashSet;
import java.util.Objects;
import java.util.Set;
import java.util.TreeSet;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.ydb.Persisted;

@Value
@Builder(toBuilder = true)
@AllArgsConstructor
@Table(name = "main/GraphDiscoveryTask")
@GlobalIndex(name = GraphDiscoveryTask.IDX_SANDBOX_TASK_ID, fields = "sandboxTaskId")
@GlobalIndex(name = GraphDiscoveryTask.IDX_STATUS_AND_SANDBOX_TASK_ID, fields = {"status", "sandboxTaskId"})
public class GraphDiscoveryTask implements Entity<GraphDiscoveryTask> {

    public static final String IDX_SANDBOX_TASK_ID = "IDX_SANDBOX_TASK_ID";
    public static final String IDX_STATUS_AND_SANDBOX_TASK_ID = "IDX_STATUS_AND_SANDBOX_TASK_ID";

    GraphDiscoveryTask.Id id;

    @Column
    long leftSvnRevision;

    @Column
    long rightSvnRevision;

    @Column(dbType = DbType.UTF8)
    @Nullable
    String leftCommitId;

    @Column
    long sandboxTaskId;

    @Column(dbType = DbType.UTF8)
    String sandboxStatus;

    @Column
    @Nullable
    Long sandboxResultResourceId;

    @Column
    @Nullable
    Long sandboxResourceSize;

    @With
    @Column(dbType = DbType.STRING)
    Status status;

    @Column(dbType = DbType.TIMESTAMP)
    Instant graphEvaluationStartedAt;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant graphEvaluationFinishedAt;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant discoveryStartedAt;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant discoveryFinishedAt;

    @Nullable
    // null for old values
    Integer attemptNumber;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant nextAttemptAfter;

    @Singular
    @Nullable   // null for old values
    Set<Long> failedSandboxTaskIds;


    @Override
    public GraphDiscoveryTask.Id getId() {
        return id;
    }

    public int getAttemptNumber() {
        return attemptNumber != null ? attemptNumber : 1;
    }

    @Nullable
    public Instant getNextAttemptAfter() {
        return nextAttemptAfter;
    }

    public Set<Long> getFailedSandboxTaskIds() {
        return Objects.requireNonNullElse(failedSandboxTaskIds, Set.of());
    }

    public ArcRevision getRightRevision() {
        return ArcRevision.of(id.getCommitId());
    }

    public ArcRevision getLeftRevision() {
        return ArcRevision.of(leftCommitId);
    }

    public static GraphDiscoveryTask graphEvaluationStarted(
            ArcRevision rightRevision,
            ArcRevision leftRevision,
            long rightSvnRevision,
            long leftSvnRevision,
            long sandboxTaskId,
            Set<Platform> platforms,
            String sandboxStatus,
            Instant graphEvaluationStartedAt
    ) {
        return GraphDiscoveryTask.builder()
                .id(new Id(rightRevision.getCommitId(), platforms))
                .leftCommitId(leftRevision.getCommitId())
                .rightSvnRevision(rightSvnRevision)
                .leftSvnRevision(leftSvnRevision)
                .sandboxTaskId(sandboxTaskId)
                .sandboxStatus(sandboxStatus)
                .status(Status.GRAPH_EVALUATION_STARTED)
                .graphEvaluationStartedAt(graphEvaluationStartedAt)
                .attemptNumber(1)
                .build();
    }


    @Value
    public static class Id implements Entity.Id<GraphDiscoveryTask> {

        private static final ObjectMapper OBJECT_MAPPER = new ObjectMapper();

        @Column(name = "commitId", dbType = DbType.UTF8)
        String commitId;
        @Column(name = "platformsJson", dbType = DbType.UTF8)
        String platformsJson;

        transient Set<Platform> platforms;

        private Id(String commitId, String platformsJson) {
            // we need this constructor for fetching rows from database
            this.commitId = commitId;
            this.platformsJson = platformsJson;
            this.platforms = platformsFromJson(platformsJson);
        }

        public Id(String commitId, Set<Platform> platforms) {
            var sortedPlatforms = new TreeSet<>(platforms);
            this.commitId = commitId;
            this.platformsJson = platformsToJson(sortedPlatforms);
            this.platforms = sortedPlatforms;
        }

        private static String platformsToJson(TreeSet<Platform> platforms) {
            try {
                return OBJECT_MAPPER.writeValueAsString(platforms);
            } catch (JsonProcessingException e) {
                throw new UncheckedIOException(e);
            }
        }

        private static LinkedHashSet<Platform> platformsFromJson(String platforms) {
            try {
                return OBJECT_MAPPER.readValue(platforms, new SetStringsTypeReference());
            } catch (JsonProcessingException e) {
                throw new UncheckedIOException(e);
            }
        }
    }


    @Persisted
    public enum Status {
        GRAPH_EVALUATION_STARTED,
        GRAPH_EVALUATION_FAILED,
        GRAPH_EVALUATION_SUCCESS,

        GRAPH_RESULTS_PROCESSING,
        GRAPH_RESULTS_PROCESSED,
    }

    @Persisted
    public enum Platform {
        LINUX,
        MANDATORY,
        SANITIZERS,
        GCC_MSVC_MUSL,
        IOS_ANDROID_CYGWIN,
    }

    private static class SetStringsTypeReference extends TypeReference<LinkedHashSet<Platform>> {
    }

}
