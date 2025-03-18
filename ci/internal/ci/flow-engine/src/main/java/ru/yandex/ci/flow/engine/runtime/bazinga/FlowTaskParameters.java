package ru.yandex.ci.flow.engine.runtime.bazinga;

import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@BenderBindAllFields
public class FlowTaskParameters {
    private final FlowLaunchId flowLaunchId;
    private final String jobId;
    private final int jobLaunchNumber;

    public FlowTaskParameters(FlowLaunchId flowLaunchId, String jobId, int jobLaunchNumber) {
        this.flowLaunchId = flowLaunchId;
        this.jobId = jobId;
        this.jobLaunchNumber = jobLaunchNumber;
    }

    public static FlowTaskParameters create(FullJobLaunchId jobLaunchId) {
        return new FlowTaskParameters(
                jobLaunchId.getFlowLaunchId(), jobLaunchId.getJobId(), jobLaunchId.getJobLaunchNumber()
        );
    }

    public FlowLaunchId getFlowLaunchId() {
        return flowLaunchId;
    }

    public String getJobId() {
        return jobId;
    }

    public int getJobLaunchNumber() {
        return jobLaunchNumber;
    }

    @Override
    public String toString() {
        return "FlowTaskParameters{" +
                "flowLaunchId='" + flowLaunchId + '\'' +
                ", jobId='" + jobId + '\'' +
                ", jobLaunchNumber=" + jobLaunchNumber +
                '}';
    }

    public FullJobLaunchId toFullJobLaunchId() {
        return new FullJobLaunchId(flowLaunchId, jobId, jobLaunchNumber);
    }
}
