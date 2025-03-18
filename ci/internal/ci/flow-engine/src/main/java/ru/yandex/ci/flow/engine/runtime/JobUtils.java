package ru.yandex.ci.flow.engine.runtime;

import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;

public class JobUtils {
    private JobUtils() {

    }

    public static String getFullJobId(FlowLaunchEntity flowLaunch, JobState jobState) {
        FlowLaunchId flowLaunchId = flowLaunch.getFlowLaunchId();
        String jobId = jobState.getJobId();
        int launchNumber = jobState.getLastLaunch().getNumber();
        return getFullJobId(flowLaunchId, jobId, launchNumber);
    }

    public static String getFullJobId(FlowLaunchId flowLaunchId, String jobId, int launchNumber) {
        return flowLaunchId.asString() + ":" + jobId + ":" + launchNumber;
    }
}
