package ru.yandex.ci.core.config;

import java.nio.file.Path;
import java.util.List;
import java.util.NavigableMap;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.Builder;
import lombok.Getter;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.registry.TaskId;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@Table(name = "main/ConfigHistory")
public class ConfigEntity implements Entity<ConfigEntity> {

    ConfigEntity.Id id;

    @Getter(AccessLevel.NONE)
    @Column(dbType = DbType.UTF8)
    String commitId;

    @Column
    long pullRequestId;

    @Column(dbType = DbType.UTF8)
    ConfigStatus status;

    List<ConfigProblem> problems;

    @Column(name = "taskRevisions2")
    @With
    NavigableMap<TaskId, ArcRevision> taskRevisions;

    @Column(dbType = DbType.JSON, flatten = false)
    ConfigSecurityState securityState;

    @Column(dbType = DbType.JSON, flatten = false)
    ConfigCreationInfo creationInfo;

    @With
    @Nullable
    @Column(dbType = DbType.JSON, flatten = false)
    ConfigPermissions permissions;

    @Nullable
    @Column(dbType = DbType.STRING)
    String username;

    public ConfigEntity(
            Id id,
            String commitId,
            Long pullRequestId,
            ConfigStatus status,
            List<ConfigProblem> problems,
            @Nullable NavigableMap<TaskId, ArcRevision> taskRevisions,
            ConfigSecurityState securityState,
            ConfigCreationInfo creationInfo,
            @Nullable ConfigPermissions permissions,
            @Nullable String username) {
        this.id = id;
        this.commitId = commitId;
        this.pullRequestId = Objects.requireNonNullElse(pullRequestId, 0L);
        this.status = status;
        this.problems = problems;
        this.taskRevisions = taskRevisions;
        this.securityState = securityState;
        this.creationInfo = creationInfo;
        this.permissions = permissions;
        this.username = username;
        validateStatus();
    }

    public ConfigEntity(
            Path configPath,
            OrderedArcRevision revision,
            ConfigStatus status,
            List<ConfigProblem> problems,
            NavigableMap<TaskId, ArcRevision> taskRevisions,
            ConfigSecurityState securityState,
            ConfigCreationInfo creationInfo,
            @Nullable ConfigPermissions permissions,
            @Nullable String username) {
        this.id = Id.of(configPath, revision);
        this.commitId = revision.getCommitId();
        this.pullRequestId = revision.getPullRequestId();
        this.status = status;
        this.problems = problems;
        this.taskRevisions = taskRevisions;
        this.securityState = securityState;
        this.creationInfo = creationInfo;
        this.permissions = permissions;
        this.username = username;
        validateStatus();
    }

    @Override
    public ConfigEntity.Id getId() {
        return id;
    }

    private void validateStatus() {
        switch (status) {
            case READY -> {
                Preconditions.checkState(ConfigProblem.isValid(problems));
                Preconditions.checkState(securityState.isValid());
            }
            case SECURITY_PROBLEM -> {
                Preconditions.checkState(ConfigProblem.isValid(problems));
                Preconditions.checkState(!securityState.isValid());
            }
            case INVALID -> {
                Preconditions.checkState(!ConfigProblem.isValid(problems));
                Preconditions.checkState(!securityState.isValid());
            }
            case NOT_CI -> {
                Preconditions.checkState(problems.isEmpty());
                Preconditions.checkState(taskRevisions.isEmpty());
                Preconditions.checkState(!securityState.isValid());
            }
            default -> throw new IllegalStateException("Unsupported state: " + status);
        }
    }

    public OrderedArcRevision getRevision() {
        return OrderedArcRevision.fromHash(commitId, id.branch, id.commitNumber, pullRequestId);
    }

    public Optional<OrderedArcRevision> getPreviousValidRevision() {
        return creationInfo.getPreviousValidRevision();
    }

    public Path getConfigPath() {
        return Path.of(id.configPath);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<ConfigEntity> {
        @Column(name = "configPath", dbType = DbType.UTF8)
        String configPath;
        @Column(name = "branch", dbType = DbType.UTF8)
        String branch;
        @Column(name = "commitNumber")
        long commitNumber;

        public static Id of(Path configPath, OrderedArcRevision revision) {
            return of(configPath.toString(), revision.getBranch().asString(), revision.getNumber());
        }
    }


}
