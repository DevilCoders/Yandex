package ru.yandex.ci.flow.engine.runtime;

import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;

public interface JobScheduler {
    void scheduleFlow(String project,
                      LaunchId launchId,
                      FullJobLaunchId fullJobLaunchId);

    void scheduleStageRecalc(String project, FlowLaunchId flowLaunchId);

}
