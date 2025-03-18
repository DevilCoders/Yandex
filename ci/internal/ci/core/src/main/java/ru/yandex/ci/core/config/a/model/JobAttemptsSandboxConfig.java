package ru.yandex.ci.core.config.a.model;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.util.Overridable;
import ru.yandex.ci.util.Overrider;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Persisted
@Value
@Builder(toBuilder = true)
@JsonDeserialize(builder = JobAttemptsSandboxConfig.Builder.class)
public class JobAttemptsSandboxConfig implements Overridable<JobAttemptsSandboxConfig> {

    @Singular
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    @JsonProperty("exclude-statuses")
    List<SandboxTaskStatus> excludeStatuses;

    @With
    @JsonProperty("reuse-tasks")
    Boolean reuseTasks;

    @With
    @JsonProperty("use-attempts")
    Integer useAttempts;

    @Override
    public JobAttemptsSandboxConfig override(JobAttemptsSandboxConfig override) {
        var builder = this.toBuilder();
        var overrider = new Overrider<>(this, override);
        overrider.field(builder::excludeStatuses, JobAttemptsSandboxConfig::getExcludeStatuses);
        overrider.field(builder::reuseTasks, JobAttemptsSandboxConfig::getReuseTasks);
        overrider.field(builder::useAttempts, JobAttemptsSandboxConfig::getUseAttempts);
        return builder.build();
    }
}
