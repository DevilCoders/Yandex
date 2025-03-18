package ru.yandex.ci.flow.engine.runtime;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public interface LaunchAutoReleaseDelegate {

    NoOp NO_OP = new NoOp();

    void scheduleLaunchAfterFlowUnlockedStage(FlowLaunchId flowLaunchId, LaunchId launchId);

    void scheduleLaunchAfterScheduledTimeHasCome(CiProcessId processId);

    class NoOp implements LaunchAutoReleaseDelegate {
        @Override
        public void scheduleLaunchAfterFlowUnlockedStage(FlowLaunchId flowLaunchId,
                                                         LaunchId launchId) {
            // no op
        }

        @Override
        public void scheduleLaunchAfterScheduledTimeHasCome(CiProcessId processId) {
            // no op
        }
    }

}
