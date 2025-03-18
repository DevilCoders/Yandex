package ru.yandex.ci.flow.engine.runtime.state.model;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.annotations.VisibleForTesting;
import com.google.gson.annotations.SerializedName;
import lombok.Value;

import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.ydb.Persisted;

/**
 * Информация из {@link ru.yandex.ci.core.launch.Launch}
 */
@Persisted
@Value
public class LaunchInfo {

    @Deprecated
    @JsonProperty(value = "version")
    @SerializedName("version")
    @Nullable
    String versionString;

    @Nullable
    @JsonProperty("ver") // version is used for unstructured version
    @SerializedName("ver")
    Version version;

    public Version getVersion() {
        if (version == null) {
            return Version.fromAsString(versionString);
        }
        return version;
    }

    @VisibleForTesting
    public static LaunchInfo of(@Nonnull String version) {
        return new LaunchInfo(version, Version.major(version));
    }

    public static LaunchInfo of(@Nonnull Version version) {
        return new LaunchInfo(null, version);
    }
}
