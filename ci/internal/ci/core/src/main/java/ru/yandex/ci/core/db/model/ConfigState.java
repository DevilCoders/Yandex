package ru.yandex.ci.core.db.model;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Instant;
import java.util.Collection;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Objects;
import java.util.Optional;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonSetter;
import com.fasterxml.jackson.annotation.Nulls;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@Table(name = "main/ConfigState")
@GlobalIndex(name = ConfigState.IDX_PROJECT_STATUS, fields = {"project", "status"})
@JsonDeserialize(using = GsonJacksonDeserializer.class)
@JsonSerialize(using = GsonJacksonSerializer.class)
public class ConfigState implements Entity<ConfigState> {
    public static final String IDX_PROJECT_STATUS = "IDX_PROJECT_STATUS";

    // warning! @Column should be defined inside Id class, don't copy-paste this
    @Column(name = "configPath", dbType = DbType.STRING)
    ConfigState.Id id;

    @Column(dbType = DbType.UTF8)
    @Nullable
    String project; // ABC slug

    @Column(dbType = DbType.UTF8)
    String title;

    @Column(flatten = false)
    OrderedArcRevision lastRevision;

    @Column(dbType = DbType.UTF8)
    Status status;

    @Column(flatten = false)
    @Nullable
    OrderedArcRevision lastValidRevision;

    @Singular
    @JsonSetter(nulls = Nulls.AS_EMPTY)
    List<ReleaseConfigState> releases;

    @Singular
    @JsonSetter(nulls = Nulls.AS_EMPTY)
    @Column(name = "flows")
    List<ActionConfigState> actions;

    @Column(dbType = DbType.TIMESTAMP)
    @With
    Instant created;

    @Column(dbType = DbType.TIMESTAMP)
    Instant updated;

    @Override
    public Id getId() {
        return id;
    }

    public Path getConfigPath() {
        return id.getConfigPath();
    }

    public long getVersion() {
        return lastRevision.getNumber();
    }

    public List<ReleaseConfigState> getReleases() {
        return Objects.requireNonNullElse(releases, List.of());
    }

    public Optional<ReleaseConfigState> findRelease(String releaseId) {
        return getReleases().stream()
                .filter(r -> releaseId.equals(r.getReleaseId()))
                .findFirst();
    }

    public ReleaseConfigState getRelease(String releaseId) {
        return findRelease(releaseId).orElseThrow(() ->
                new NoSuchElementException("Release %s not found".formatted(releaseId)));
    }

    public List<ActionConfigState> getActions() {
        return Objects.requireNonNullElse(actions, List.of());
    }

    public Optional<ActionConfigState> findAction(String flowId) {
        return getActions().stream()
                .filter(f -> flowId.equals(f.getFlowId()))
                .findFirst();
    }

    public ActionConfigState getAction(String flowId) {
        return findAction(flowId).orElseThrow(() ->
                new NoSuchElementException("Action or flow %s not found".formatted(flowId)));
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<ConfigState> {
        @Column(dbType = DbType.STRING)
        String configPath;

        public static Id of(Path configPath) {
            return of(configPath.toString());
        }

        public Path getConfigPath() {
            return Paths.get(configPath);
        }
    }

    public static class Builder {
        public Builder configPath(Path configPath) {
            this.id = ConfigState.Id.of(configPath);
            return this;
        }
    }

    @Persisted
    public enum Status {
        OK(false),
        INVALID(false),
        INVALID_PREVIOUS_VALID(false),

        // Deleted config
        DELETED(true),

        // Special state, registered for new a.yaml in PR
        DRAFT(true),

        // Arcanum a.yaml (does not have CI block but still probably a valid a.yaml)
        NOT_CI(true);

        private static final List<Status> HIDDEN = Stream.of(Status.values())
                .filter(Status::isHidden)
                .toList();

        private final boolean hidden;

        Status(boolean hidden) {
            this.hidden = hidden;
        }

        public boolean isHidden() {
            return hidden;
        }

        // 'hidden' status means this config could not be used in CI
        // It's a basic exclusion filter whenever you need to list configs to use (even if not delegated yet)
        public static Collection<Status> hidden() {
            return HIDDEN;
        }
    }
}
