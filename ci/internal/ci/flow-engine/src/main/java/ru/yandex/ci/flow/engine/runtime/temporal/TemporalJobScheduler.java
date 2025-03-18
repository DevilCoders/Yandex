package ru.yandex.ci.flow.engine.runtime.temporal;

import lombok.AllArgsConstructor;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.JobScheduler;

@AllArgsConstructor
public class TemporalJobScheduler implements JobScheduler {

    private final TemporalService temporalService;

    @Override
    public void scheduleFlow(String project, LaunchId launchId, FullJobLaunchId fullJobLaunchId) {
        temporalService.startInTx(FlowJobWorkflow.class, wf -> wf::run, fullJobLaunchId);
    }

    @Override
    public void scheduleStageRecalc(String project, FlowLaunchId flowLaunchId) {
        throw new UnsupportedOperationException("Not supported yet");
    }
}
