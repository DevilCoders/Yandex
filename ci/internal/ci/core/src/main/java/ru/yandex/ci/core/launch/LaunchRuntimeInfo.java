package ru.yandex.ci.core.launch;

import java.time.Duration;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.Value;

import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.a.model.RuntimeSandboxConfig;
import ru.yandex.ci.core.config.a.model.SandboxNotificationConfig;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority;
import ru.yandex.ci.core.sandbox.NotificationSettingEntity;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("deprecation")
@Persisted
@AllArgsConstructor
@Value
@Builder(toBuilder = true)
public class LaunchRuntimeInfo {

    // Could be overwritten with `delegateConfig` solution
    @Nullable
    YavToken.Id yavTokenUid; // Could be null with DELAY state

    // Could be overwritten with `delegatedConfig`, null means we should use owner from `runtimeConfig`
    @Nullable
    String sandboxOwner;

    @Nullable
    RuntimeConfig runtimeConfig; // Could be null for old records

    @Nullable
    RequirementsConfig requirementsConfig; // Could be null


    // Only for backward compatibility

    @Getter(AccessLevel.NONE)
    @Nullable
    List<NotificationSettingEntity> sandboxNotificationSettings;

    @Getter(AccessLevel.NONE)
    @Nullable
    Duration killTimeout;

    @Getter(AccessLevel.NONE)
    @Nullable
    List<String> tags;

    @Getter(AccessLevel.NONE)
    @Nullable
    List<String> hints;

    @Getter(AccessLevel.NONE)
    @Nullable
    TaskPriority priority;

    public static LaunchRuntimeInfo ofRuntimeSandboxOwner(@Nonnull String sandboxOwner) {
        return of(null, null, RuntimeConfig.ofSandboxOwner(sandboxOwner), null);
    }

    public static LaunchRuntimeInfo ofRuntimeSandboxOwner(
            @Nonnull YavToken.Id yavTokenUid,
            @Nonnull String sandboxOwner
    ) {
        return of(yavTokenUid, null, RuntimeConfig.ofSandboxOwner(sandboxOwner), null);
    }

    public static LaunchRuntimeInfo of(
            @Nullable YavToken.Id yavTokenUid,
            @Nullable String sandboxOwner,
            @Nonnull RuntimeConfig runtimeConfig,
            @Nullable RequirementsConfig requirementsConfig
    ) {
        return new LaunchRuntimeInfo(
                yavTokenUid,
                sandboxOwner,
                runtimeConfig,
                requirementsConfig,
                List.of(),
                null,
                List.of(),
                List.of(),
                null
        );
    }

    public YavToken.Id getYavTokenUidOrThrow() {
        Preconditions.checkState(yavTokenUid != null,
                "YavTokenUid not found in runtime info");
        return yavTokenUid;
    }

    public RuntimeConfig getRuntimeConfig() {
        if (runtimeConfig != null) {
            return runtimeConfig;
        } else {
            List<SandboxNotificationConfig> notifications = sandboxNotificationSettings == null
                    ? List.of()
                    :
                    sandboxNotificationSettings.stream()
                            .map(NotificationSettingEntity::toConfig)
                            .toList();
            return RuntimeConfig.builder()
                    .sandbox(
                            RuntimeSandboxConfig.builder()
                                    .owner(sandboxOwner)
                                    .notifications(notifications)
                                    .killTimeout(killTimeout)
                                    .tags(Objects.requireNonNullElse(tags, List.of()))
                                    .hints(Objects.requireNonNullElse(hints, List.of()))
                                    .priority(priority)
                                    .build()
                    )
                    .build();
        }
    }
}
