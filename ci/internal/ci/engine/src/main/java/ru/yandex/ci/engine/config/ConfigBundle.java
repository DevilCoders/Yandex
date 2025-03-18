package ru.yandex.ci.engine.config;

import java.nio.file.Path;
import java.util.NavigableMap;
import java.util.Optional;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.EqualsAndHashCode;
import lombok.ToString;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.lang.NonNullApi;

@ToString
@EqualsAndHashCode
@NonNullApi
public class ConfigBundle {
    private final ConfigEntity configEntity;
    @Nullable
    private final AYamlConfig aYamlConfig;
    private final NavigableMap<TaskId, TaskConfig> taskConfigs;

    public ConfigBundle(@Nonnull ConfigEntity configEntity,
                        @Nullable AYamlConfig aYamlConfig,
                        @Nonnull NavigableMap<TaskId, TaskConfig> taskConfigs) {
        if (configEntity.getStatus().isValidCiConfig()) {
            Preconditions.checkArgument(aYamlConfig != null, "aYamlConfig cannot be null if configuration is valid");
        }
        this.configEntity = configEntity;
        this.aYamlConfig = aYamlConfig;
        this.taskConfigs = taskConfigs;
    }

    public ConfigEntity getConfigEntity() {
        return configEntity;
    }

    public Optional<AYamlConfig> getOptionalAYamlConfig() {
        return Optional.ofNullable(aYamlConfig);
    }

    public AYamlConfig getValidAYamlConfig() {
        Preconditions.checkState(configEntity.getStatus().isValidCiConfig(),
                "Config is invalid at revision %s",
                configEntity.getRevision());
        Preconditions.checkState(aYamlConfig != null,
                "aYamlConfig cannot be null if configuration is valid");
        return aYamlConfig;
    }

    public NavigableMap<TaskId, TaskConfig> getTaskConfigs() {
        return taskConfigs;
    }

    public Path getConfigPath() {
        return configEntity.getConfigPath();
    }

    public OrderedArcRevision getRevision() {
        return configEntity.getRevision();
    }

    public ConfigStatus getStatus() {
        return configEntity.getStatus();
    }

    public boolean isReadyForLaunch() {
        return getStatus() == ConfigStatus.READY;
    }

    // TODO: Rewrite - introduce 'validConfig' then implement all required access methods
    public ReleaseConfig getValidReleaseConfigOrThrow(CiProcessId processId) {
        return getRelease(processId).orElseThrow(() ->
                new IllegalArgumentException("Release not found " + processId.getSubId()));
    }

    public Optional<ReleaseConfig> getRelease(CiProcessId processId) {
        Preconditions.checkState(processId.getType() == CiProcessId.Type.RELEASE,
                "process is not a release: %s", processId.getType());
        Preconditions.checkState(configEntity.getConfigPath().equals(processId.getPath()),
                "requested process for config %s, but bundle for %s",
                processId.getPath(), configEntity.getConfigPath());

        var cfg = getValidAYamlConfig();

        var releaseId = processId.getSubId();
        return cfg.getCi().findRelease(releaseId);
    }

    // Post-process configBundle to make sure it's compatible with provided processId
    public ConfigBundle withVirtualProcessId(VirtualCiProcessId processId, String service) {
        var virtualType = processId.getVirtualType();
        if (virtualType == null) {
            return this;
        }

        var cfg = getValidAYamlConfig();

        var copyAction = cfg.getCi()
                .getAction(virtualType.getCiProcessId().getSubId())
                .withId(processId.getCiProcessId().getSubId());

        var newCi = cfg.getCi().toBuilder()
                .action(copyAction)
                .build();
        var newCfg = cfg.withService(service).withCi(newCi);
        return new ConfigBundle(configEntity, newCfg, taskConfigs);
    }
}
