package ru.yandex.ci.core.config.a.model;

import java.time.Duration;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.util.Overridable;
import ru.yandex.ci.util.Overrider;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Persisted
@Value
@Builder
@JsonDeserialize(builder = RuntimeConfig.Builder.class)
public class RuntimeConfig implements Overridable<RuntimeConfig> {

    @With
    @Nonnull
    @JsonProperty
    RuntimeSandboxConfig sandbox;

    @With
    @JsonProperty("get-output-on-fail")
    Boolean getOutputOnFail;

    public static RuntimeConfig of(RuntimeSandboxConfig sandbox) {
        return builder().sandbox(sandbox).build();
    }

    public static RuntimeConfig ofKillTimeout(Duration killTimeout) {
        return RuntimeConfig.builder()
                .sandbox(RuntimeSandboxConfig.builder()
                        .killTimeout(killTimeout)
                        .build())
                .build();
    }

    public static RuntimeConfig ofSandboxOwner(String owner) {
        return RuntimeConfig.of(RuntimeSandboxConfig.ofOwner(owner));
    }

    @Override
    public RuntimeConfig override(RuntimeConfig override) {
        var copy = RuntimeConfig.builder();
        var overrider = new Overrider<>(this, override);
        overrider.fieldDeep(copy::sandbox, RuntimeConfig::getSandbox);
        overrider.field(copy::getOutputOnFail, RuntimeConfig::getGetOutputOnFail);
        return copy.build();
    }

    public static class Builder {

        public Builder() {
            sandbox(RuntimeSandboxConfig.builder().build());
        }

        // Multiple constructors with single property cannot be discriminated
        public Builder(
                @Nullable @JsonProperty("sandbox") RuntimeSandboxConfig sandbox,
                @JsonProperty("sandbox-owner") String sandboxOwner,
                @JsonProperty("get-output-on-fail") Boolean getOutputOnFail) {
            if (sandbox != null) {
                sandbox(sandbox);
            } else {
                sandbox(RuntimeSandboxConfig.ofOwner(sandboxOwner));
            }
            getOutputOnFail(getOutputOnFail);
        }
    }
}
