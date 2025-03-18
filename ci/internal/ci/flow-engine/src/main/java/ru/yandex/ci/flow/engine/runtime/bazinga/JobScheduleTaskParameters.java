package ru.yandex.ci.flow.engine.runtime.bazinga;

import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@BenderBindAllFields
public class JobScheduleTaskParameters {
    private final FlowLaunchId flowLaunchId;
    private final String jobId;
    private final int jobLaunchNumber;
    private final int schedulerLaunchNumber;

    public JobScheduleTaskParameters(FlowLaunchId flowLaunchId, String jobId,
                                     int jobLaunchNumber, int schedulerLaunchNumber) {
        this.flowLaunchId = flowLaunchId;
        this.jobId = jobId;
        this.jobLaunchNumber = jobLaunchNumber;
        this.schedulerLaunchNumber = schedulerLaunchNumber;
    }

    public static JobScheduleTaskParameters retry(JobScheduleTaskParameters taskParameters) {
        return new JobScheduleTaskParameters(
                taskParameters.getFlowLaunchId(),
                taskParameters.getJobId(),
                taskParameters.getJobLaunchNumber(),
                taskParameters.getSchedulerLaunchNumber() + 1
        );
    }

    public static JobScheduleTaskParameters create(FullJobLaunchId jobLaunchId) {
        return new JobScheduleTaskParameters(
                jobLaunchId.getFlowLaunchId(), jobLaunchId.getJobId(), jobLaunchId.getJobLaunchNumber(), 0
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

    public int getSchedulerLaunchNumber() {
        return schedulerLaunchNumber;
    }

    @Override
    public String toString() {
        return "JobScheduleTaskParameters{" +
                "flowLaunchId='" + flowLaunchId + '\'' +
                ", jobId='" + jobId + '\'' +
                ", jobLaunchNumber=" + jobLaunchNumber +
                ", schedulerLaunchNumber=" + schedulerLaunchNumber +
                '}';
    }
}
