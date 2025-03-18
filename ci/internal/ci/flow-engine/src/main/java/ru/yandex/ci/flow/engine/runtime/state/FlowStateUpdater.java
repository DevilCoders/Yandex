package ru.yandex.ci.flow.engine.runtime.state;

import java.util.List;

import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.FlowEvent;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;

interface FlowStateUpdater {
    FlowLaunchEntity activateLaunch(FlowLaunchEntity flowLaunch);

    FlowLaunchEntity recalc(FlowLaunchId flowLaunchId, LaunchId launchId, FlowEvent event);

    void disableLaunchGracefully(FlowLaunchId flowLaunchId, LaunchId launchId, boolean ignoreUninterruptableStage);

    void disableJobsInLaunchGracefully(
            FlowLaunchId flowLaunchId,
            LaunchId launchId,
            List<String> jobIds,
            boolean ignoreUninterruptableStage,
            boolean killJobs
    );

    void cleanupFlowLaunch(FlowLaunchId flowLaunchId, LaunchId launchId);
}
