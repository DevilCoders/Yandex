package ru.yandex.ci.core.launch;

import java.nio.file.Path;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.Builder;
import lombok.Getter;
import lombok.Singular;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.common.ydb.KikimrProjectionCI;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.ydb.Persisted;

/**
 * Объект хранящий информацию о запуске.
 * В отличии от FlowLaunch является легковестным и используется в различных списках, фильтрах и сортировках
 * Граф должен уметь исполнять без знаний их этого объекта
 */
@SuppressWarnings({"BoxedPrimitiveEquality", "ReferenceEquality"})
@Value
@Builder(toBuilder = true)
@Table(name = "main/Launch")
@GlobalIndex(name = Launch.IDX_PATH_COMMIT_STATUS, fields = {"configPath", "configCommitId", "status"})
@GlobalIndex(name = Launch.IDX_PROCESS_ID_VERSION, fields = {"id.processId", "versionString"})
@GlobalIndex(name = Launch.IDX_ACTIVITY_CHANGED, fields = {"activityChanged", "id.processId", "activity"})
@GlobalIndex(name = Launch.IDX_PROJECT_PROCESS_TYPE_STATUS, fields = {"project", "processType", "status"})
@GlobalIndex(name = Launch.IDX_PROCESS_ID_STATUS, fields = {"id.processId", "status"})
public class Launch implements Entity<Launch>, KikimrProjectionCI {
    /*
    Additional sql:
    ALTER TABLE `main/Launch` SET (AUTO_PARTITIONING_MIN_PARTITIONS_COUNT=10);
    ALTER TABLE `main/Launch` SET (AUTO_PARTITIONING_BY_LOAD = ENABLED);
    */
    public static final String IDX_PATH_COMMIT_STATUS = "IDX_PATH_COMMIT_STATUS";
    public static final String IDX_PROCESS_ID_VERSION = "IDX_PROCESS_ID_VERSION";
    public static final String IDX_ACTIVITY_CHANGED = "IDX_STATUS_CHANGED";
    public static final String IDX_PROJECT_PROCESS_TYPE_STATUS = "IDX_PROJECT_PROCESS_TYPE_STATUS";
    public static final String IDX_PROCESS_ID_STATUS = "IDX_PROCESS_ID_STATUS";

    Launch.Id id;

    @Column(dbType = DbType.UTF8)
    String configPath;

    @Column(dbType = DbType.UTF8)
    String title;

    @Column(dbType = DbType.UTF8)
    Type type;

    @Column(dbType = DbType.UTF8)
    String project;

    @Column(dbType = DbType.UTF8)
    String triggeredBy;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Nonnull
    @Column(dbType = DbType.JSON, flatten = false)
    LaunchFlowInfo flowInfo;

    @Column(dbType = DbType.UTF8)
    String configCommitId;

    //LaunchState
    @With
    @Column(dbType = DbType.UTF8)
    LaunchState.Status status;

    @Column(dbType = DbType.UTF8)
    String statusText;

    @Nullable
    @Column(dbType = DbType.UTF8)
    String flowLaunchId;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant started;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant finished;

    @Nullable
    @Column(dbType = DbType.UTF8)
    String cancelledBy;

    @Nullable
    @Column(dbType = DbType.UTF8)
    String cancelledReason;

    @Nullable
    @Column(dbType = DbType.UTF8)
    String launchReason;

    @Nullable
    @Column
    Boolean displaced;

    // Какая версия вытеснила текущую?
    @Nullable
    @Column(dbType = DbType.JSON, flatten = false)
    LaunchDisplacedBy displacedBy;

    @Nullable
    @Column
    Boolean hasDisplacement;

    @Nullable
    @Column
    Boolean waitingForCleanup;

    // Список изменений настроек вытеснения (по порядку применения)
    @Singular
    @Nullable
    @Column(dbType = DbType.JSON, flatten = false)
    List<LaunchDisplacementChange> displacementChanges;

    @Column(dbType = DbType.JSON, flatten = false)
    LaunchVcsInfo vcsInfo;

    //LaunchUserData
    @Singular
    @With
    @Column
    List<String> tags;

    @Column
    @With
    boolean pinned;

    @Column
    boolean notifyPullRequest;

    /**
     * Список id отмененных релизов, которые предшествовали текущему.
     * В произвольном порядке
     */
    @Singular
    @Column(dbType = DbType.JSON, flatten = false)
    Set<Integer> cancelledReleases;

    @Singular
    @Column(dbType = DbType.JSON, flatten = false)
    Set<Integer> displacedReleases;

    // В настоящий момент используется только как строковое представление #version для индексированного поиска
    @Nullable
    @Getter(AccessLevel.PRIVATE)
    @Column(dbType = DbType.UTF8, name = "version")
    String versionString;

    @Nullable
    @Column(dbType = DbType.JSON, flatten = false, name = "ver")
    Version version;

    @Column(dbType = DbType.UTF8)
    CiProcessId.Type processType;

    @Column(dbType = DbType.UTF8)
    Activity activity;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant activityChanged;

    @Column(dbType = DbType.JSON, flatten = false)
    LaunchStatistics statistics;

    @Nullable   // for old values
    @Column(dbType = DbType.UTF8)
    String selectedBranch;

    @Override
    public Launch.Id getId() {
        return id;
    }

    @Override
    public List<Entity<?>> createProjections() {
        List<Entity<?>> projections = new ArrayList<>();

        projections.add(LaunchByProcessIdAndArcBranch.of(this));
        projections.add(LaunchByProcessIdAndPinned.of(this));
        projections.add(LaunchByProcessIdAndStatus.of(this));
        projections.addAll(LaunchByProcessIdAndTag.of(this));
        LaunchByPullRequest.of(this).ifPresent(projections::add);

        return projections;
    }

    public boolean isDisplaced() {
        return Boolean.TRUE.equals(displaced);
    }

    public boolean isWaitingForCleanup() {
        return Boolean.TRUE.equals(waitingForCleanup);
    }

    @Deprecated
    public Path getConfigPath() {
        return Path.of(configPath);
    }

    public LaunchId getLaunchId() {
        return LaunchId.of(getProcessId(), id.launchNumber);
    }

    public LaunchState getState() {
        return new LaunchState(flowLaunchId, started, finished, status, statusText);
    }

    public LaunchUserData getUserData() {
        return new LaunchUserData(tags, pinned);
    }

    public CiProcessId getProcessId() {
        return id.toProcessId();
    }

    public Set<Integer> getCancelledReleases() {
        // workaround for missed values https://github.com/google/gson/issues/1005
        return Objects.requireNonNullElse(cancelledReleases, Set.of());
    }

    public Set<Integer> getDisplacedReleases() {
        return Objects.requireNonNullElse(displacedReleases, Set.of());
    }

    public List<LaunchDisplacementChange> getDisplacementChanges() {
        return Objects.requireNonNullElse(displacementChanges, List.of());
    }

    public CiProcessId.Type getProcessType() {
        return Objects.requireNonNullElseGet(processType, () -> getProcessId().getType());
    }

    public List<String> getTags() {
        return Objects.requireNonNullElse(tags, List.of());
    }

    @Persisted
    public enum Type {
        USER,   // USER is an antonym for SYSTEM

        // Only for backward compatibility with old flows
        @Deprecated
        SYSTEM
    }

    @SuppressWarnings("ReferenceEquality")
    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<Launch> {
        @With
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "launchNumber")
        int launchNumber;

        public CiProcessId toProcessId() {
            return CiProcessId.ofString(processId);
        }
    }

    public Version getVersion() {
        if (version == null) {
            Preconditions.checkState(versionString != null,
                    "Internal error, both version and versionString are null in launch %s", id);
            return Version.fromAsString(versionString);
        }
        return version;
    }

    public static class Builder {

        public Builder launchId(LaunchId launchId) {
            var processId = launchId.getProcessId();
            return this
                    .id(Id.of(processId.asString(), launchId.getNumber()))
                    .processType(processId.getType())
                    .configPath(processId.getPath().toString());
        }

        private Builder processType(CiProcessId.Type processType) {
            // рассчитывается в launchId
            this.processType = processType;
            return this;
        }

        public Builder userData(LaunchUserData userData) {
            return this
                    .tags(userData.getTags())
                    .pinned(userData.isPinned());
        }

        public Builder flowInfo(LaunchFlowInfo flowInfo) {
            this.flowInfo = flowInfo;
            this.configCommitId = flowInfo.getConfigRevision().getCommitId();
            return this;
        }

        public Builder status(LaunchState.Status status) {
            this.status = status;
            this.activity = Activity.fromStatus(status);
            return this;
        }

        private Builder configCommitId(String configCommitId) {
            /* The method is used inside `Launch.toBuilder`.
               The method is private cause we want to keep the field in sync with `flowInfo` and
                  we want to allow a user to change this field only via `withFlowInfo` */
            this.configCommitId = configCommitId;
            return this;
        }

        // It is possible to get null when using Mockito or something similar
        public Builder version(@Nullable Version version) {
            this.version = version;
            this.versionString = version != null
                    ? version.asString()
                    : null;
            return this;
        }

        private Builder versionString(String versionString) {
            // See note for #configCommitId
            this.versionString = versionString;
            return this;
        }

        public Builder vcsInfo(LaunchVcsInfo vcsInfo) {
            this.vcsInfo = vcsInfo;
            this.selectedBranch = vcsInfo.getSelectedBranch().getBranch();
            return this;
        }

        private Builder selectedBranch(String selectedBranch) {
            // selectedBranch is set inside setVcsInfo
            this.selectedBranch = selectedBranch;
            return this;
        }
    }

}
