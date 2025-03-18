package ru.yandex.ci.flow.engine.runtime.helpers;

import java.util.HashSet;
import java.util.Set;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.LaunchAutoReleaseDelegate;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class TestLaunchAutoReleaseDelegate implements LaunchAutoReleaseDelegate {

    private final Set<FlowLaunchId> flowsWhichUnlockedStage = new HashSet<>();

    @Override
    public void scheduleLaunchAfterFlowUnlockedStage(FlowLaunchId flowLaunchId, LaunchId launchId) {
        flowsWhichUnlockedStage.add(flowLaunchId);
    }

    @Override
    public void scheduleLaunchAfterScheduledTimeHasCome(CiProcessId processId) {
        throw new UnsupportedOperationException();
    }

    public Set<FlowLaunchId> getFlowsWhichUnlockedStage() {
        return flowsWhichUnlockedStage;
    }
}
