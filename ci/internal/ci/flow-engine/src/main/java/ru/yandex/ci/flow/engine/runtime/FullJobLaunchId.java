package ru.yandex.ci.flow.engine.runtime;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Preconditions;
import lombok.Value;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.ydb.Persisted;

@Value
@Persisted
public class FullJobLaunchId implements BaseTemporalWorkflow.Id {
    @Nonnull
    String flowLaunchId;
    @Nonnull
    String jobId;
    int jobLaunchNumber;

    @JsonCreator
    public FullJobLaunchId(@JsonProperty("flowLaunchId") @Nonnull String flowLaunchId,
                           @JsonProperty("jobId") @Nonnull String jobId,
                           @JsonProperty("jobLaunchNumber") int jobLaunchNumber) {
        Preconditions.checkArgument(jobLaunchNumber > 0);
        this.flowLaunchId = flowLaunchId;
        this.jobId = jobId;
        this.jobLaunchNumber = jobLaunchNumber;
    }

    public FullJobLaunchId(@Nonnull FlowLaunchId flowLaunchId, @Nonnull String jobId, int jobLaunchNumber) {
        this(flowLaunchId.asString(), jobId, jobLaunchNumber);
    }

    public FlowLaunchId getFlowLaunchId() {
        return FlowLaunchId.of(flowLaunchId);
    }

    public static FullJobLaunchId of(JobInstance.Id id) {
        return new FullJobLaunchId(id.getFlowLaunchId(), id.getJobId(), id.getNumber());
    }

    @Override
    public String getTemporalWorkflowId() {
        return flowLaunchId + "-" + jobId + "-" + jobLaunchNumber;
    }
}
