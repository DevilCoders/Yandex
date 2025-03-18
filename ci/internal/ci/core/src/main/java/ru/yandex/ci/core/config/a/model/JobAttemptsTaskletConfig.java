package ru.yandex.ci.core.config.a.model;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.util.Overridable;
import ru.yandex.ci.util.Overrider;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.tasklet.api.v2.DataModel;

@Persisted
@Value
@Builder(toBuilder = true)
@JsonDeserialize(builder = JobAttemptsTaskletConfig.Builder.class)
public class JobAttemptsTaskletConfig implements Overridable<JobAttemptsTaskletConfig> {

    @Singular
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    @JsonProperty("exclude-server-errors")
    List<DataModel.ErrorCodes.ErrorCode> excludeServerErrors;

    @Override
    public JobAttemptsTaskletConfig override(JobAttemptsTaskletConfig override) {
        var builder = this.toBuilder();
        var overrider = new Overrider<>(this, override);
        overrider.field(builder::excludeServerErrors, JobAttemptsTaskletConfig::getExcludeServerErrors);
        return builder.build();
    }
}
