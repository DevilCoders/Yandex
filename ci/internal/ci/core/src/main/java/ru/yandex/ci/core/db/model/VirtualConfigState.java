package ru.yandex.ci.core.db.model;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Instant;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Optional;

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
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;

/**
 * Unlike ConfigState here we store virtual configurations, without code in repository
 */
@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@Table(name = "main/VirtualConfigState")
@GlobalIndex(name = VirtualConfigState.IDX_PROJECT, fields = {"project"})
@JsonDeserialize(using = GsonJacksonDeserializer.class)
@JsonSerialize(using = GsonJacksonSerializer.class)
public class VirtualConfigState implements Entity<VirtualConfigState> {
    public static final String IDX_PROJECT = "IDX_PROJECT";

    // warning! @Column should be defined inside Id class, don't copy-paste this
    @Column(name = "configPath", dbType = DbType.STRING)
    VirtualConfigState.Id id;

    @Column(dbType = DbType.UTF8)
    @Nullable
    String project; // ABC slug

    @Column(dbType = DbType.UTF8)
    String title;

    @Column(dbType = DbType.UTF8)
    ConfigState.Status status;

    @Singular
    @JsonSetter(nulls = Nulls.AS_EMPTY)
    @Column(name = "flows")
    List<ActionConfigState> actions;

    @Column(dbType = DbType.TIMESTAMP)
    @With
    Instant created;

    @Column(dbType = DbType.TIMESTAMP)
    Instant updated;

    @Column(dbType = DbType.UTF8)
    VirtualType virtualType;

    @Override
    public Id getId() {
        return id;
    }

    public Path getConfigPath() {
        return id.getConfigPath();
    }

    public Optional<ActionConfigState> findFlow(String flowId) {
        return actions.stream()
                .filter(f -> flowId.equals(f.getFlowId()))
                .findFirst();
    }

    public ActionConfigState getFlow(String flowId) {
        return findFlow(flowId).orElseThrow(() ->
                new NoSuchElementException("Action or flow %s not found".formatted(flowId)));
    }

    public ConfigState toConfigState() {
        return ConfigState.builder()
                .id(ConfigState.Id.of(id.configPath))
                .project(project)
                .title(title)
                .status(status)
                .actions(actions)
                .created(created)
                .updated(updated)
                .build();
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<VirtualConfigState> {
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
            this.id = VirtualConfigState.Id.of(configPath);
            return this;
        }
    }
}
