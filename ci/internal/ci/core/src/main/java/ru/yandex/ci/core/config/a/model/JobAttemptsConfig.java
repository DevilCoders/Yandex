package ru.yandex.ci.core.config.a.model;

import java.time.Duration;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.time.DurationDeserializer;
import ru.yandex.ci.util.Overridable;
import ru.yandex.ci.util.Overrider;
import ru.yandex.ci.ydb.Persisted;


@Persisted
@Value
@Builder(toBuilder = true)
@AllArgsConstructor
@JsonDeserialize(builder = JobAttemptsConfig.Builder.class)
public class JobAttemptsConfig implements Overridable<JobAttemptsConfig> {
    @JsonProperty("max")
    int maxAttempts;

    @JsonProperty
    @Nullable
    Backoff backoff;

    @Nullable
    @JsonProperty("initial-backoff")
    @JsonDeserialize(converter = DurationDeserializer.class)
    @JsonSerialize(converter = DurationSerializer.class)
    Duration initialBackoff;

    @Nullable
    @JsonProperty("max-backoff")
    @JsonDeserialize(converter = DurationDeserializer.class)
    @JsonSerialize(converter = DurationSerializer.class)
    Duration maxBackoff;

    @Nullable
    @JsonProperty("if-output")
    String ifOutput;

    @Nullable
    @JsonProperty("sandbox")
    JobAttemptsSandboxConfig sandboxConfig;

    @Nullable
    @JsonProperty("tasklet")
    JobAttemptsTaskletConfig taskletConfig;

    public Backoff getBackoff() {
        return Objects.requireNonNullElse(backoff, Backoff.CONSTANT);
    }

    public boolean needWaitSchedule() {
        return initialBackoff != null;
    }

    public static JobAttemptsConfig ofAttempts(int maxAttempts) {
        return JobAttemptsConfig.builder()
                .maxAttempts(maxAttempts)
                .build();
    }

    @Override
    public JobAttemptsConfig override(JobAttemptsConfig override) {
        var builder = this.toBuilder();
        Overrider<JobAttemptsConfig> overrider = new Overrider<>(this, override);
        overrider.field(builder::maxAttempts, JobAttemptsConfig::getMaxAttempts);
        overrider.field(builder::backoff, JobAttemptsConfig::getBackoff);
        overrider.field(builder::initialBackoff, JobAttemptsConfig::getInitialBackoff);
        overrider.field(builder::maxBackoff, JobAttemptsConfig::getMaxBackoff);
        overrider.field(builder::ifOutput, JobAttemptsConfig::getIfOutput);
        overrider.fieldDeep(builder::sandboxConfig, JobAttemptsConfig::getSandboxConfig);
        overrider.fieldDeep(builder::taskletConfig, JobAttemptsConfig::getTaskletConfig);
        return builder.build();
    }

    public static class Builder {
        public Builder() {
            //
        }

        public Builder(int maxAttempts) {
            this.maxAttempts(maxAttempts);
        }

    }
}
