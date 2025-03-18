package ru.yandex.ci.core.config.a.model;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Value;

@Value
@Builder
@JsonDeserialize(builder = AutocheckConfig.Builder.class)
public class AutocheckConfig {

    @JsonProperty("fast-targets")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> fastTargets;

    @JsonProperty("fast-mode")
    FastMode fastMode;

    @JsonProperty("strong")
    StrongModeConfig strongMode;

    @JsonProperty("large-sandbox-owner")
    String largeSandboxOwner;

    @JsonProperty("large-autostart")
    List<LargeAutostartConfig> largeAutostart;

    @JsonProperty("native-sandbox-owner")
    String nativeSandboxOwner;

    @JsonProperty("native-builds")
    List<NativeBuildConfig> nativeBuilds;

    public List<LargeAutostartConfig> getLargeAutostart() {
        return Objects.requireNonNullElse(largeAutostart, List.of());
    }

    @JsonIgnore
    public List<NativeBuildConfig> getNativeBuilds() {
        return Objects.requireNonNullElse(nativeBuilds, List.of());
    }

    public enum FastMode {
        @JsonProperty("auto")
        AUTO,

        @Deprecated // Is not supported anymore
        @JsonProperty("sequential")
        SEQUENTIAL
    }

    public static class Builder {
        {
            fastMode = FastMode.AUTO;
        }

        @JsonProperty("large-autostart")
        @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
        public Builder largeAutostart(List<Object> largeAutostart) {
            this.largeAutostart = largeAutostart.stream()
                    .map(r -> {
                        if (r instanceof Map) {
                            return LargeAutostartConfig.fromMap((Map<?, ?>) r);
                        } else if (r instanceof LargeAutostartConfig config) {
                            return config;
                        } else if (r instanceof String string) {
                            return new LargeAutostartConfig(string);
                        } else {
                            throw new IllegalStateException("Unable to parse as Large autostart config: " + r);
                        }
                    }).toList();
            return this;
        }


        @JsonProperty("native-builds")
        @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
        public Builder nativeBuilds(LinkedHashMap<String, List<String>> nativeBuildsMap) {
            this.nativeBuilds = nativeBuildsMap.entrySet().stream()
                    .filter(b -> !b.getValue().isEmpty())
                    .map(b -> new NativeBuildConfig(b.getKey(), b.getValue()))
                    .toList();
            return this;
        }

        public Builder nativeBuilds(List<NativeBuildConfig> nativeBuilds) {
            this.nativeBuilds = nativeBuilds;
            return this;
        }

    }
}
