package ru.yandex.ci.core.config.a.model;

import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.Setter;
import lombok.ToString;
import lombok.Value;
import lombok.experimental.NonFinal;

import ru.yandex.ci.core.config.a.model.auto.AutoReleaseConfig;
import ru.yandex.ci.util.jackson.parse.HasParseInfo;
import ru.yandex.ci.util.jackson.parse.ParseInfo;

@Value
@Builder
@JsonDeserialize(builder = ReleaseBranchesConfig.Builder.class)
public class ReleaseBranchesConfig implements HasParseInfo {
    public static final ReleaseBranchesConfig EMPTY = ReleaseBranchesConfig.builder()
            .enabled(false)
            .pattern("releases/ci/examples/${version}")
            .build();

    @JsonProperty
    @lombok.Builder.Default
    boolean enabled = true;

    @JsonProperty
    String pattern;

    @JsonProperty("forbid-trunk-releases")
    boolean forbidTrunkReleases;

    @JsonProperty("auto-create")
    boolean autoCreate;

    @Nullable
    @JsonProperty
    AutoReleaseConfig auto;

    @JsonProperty("independent-stages")
    boolean independentStages;

    @Nullable
    @JsonProperty("default-config-source")
    DefaultConfigSource defaultConfigSource;

    @ToString.Exclude
    @EqualsAndHashCode.Exclude
    @Getter(onMethod_ = @Override)
    @Setter(onMethod_ = @Override)
    @NonFinal
    @JsonIgnore
    transient ParseInfo parseInfo;

    public enum DefaultConfigSource {
        @JsonProperty("trunk")
        TRUNK,

        @JsonProperty("branch")
        BRANCH;

        public boolean isTrunk() {
            return this == TRUNK;
        }
    }

    public AutoReleaseConfig getAuto() {
        return Objects.requireNonNullElse(auto, AutoReleaseConfig.EMPTY);
    }

    public DefaultConfigSource getDefaultConfigSource() {
        return Objects.requireNonNullElse(defaultConfigSource, DefaultConfigSource.TRUNK);
    }
}
