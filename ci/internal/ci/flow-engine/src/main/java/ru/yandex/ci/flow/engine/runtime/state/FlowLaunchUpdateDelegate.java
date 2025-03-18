package ru.yandex.ci.flow.engine.runtime.state;

import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;

public interface FlowLaunchUpdateDelegate {
    NoOpUpdateDelegate NO_OP = new NoOpUpdateDelegate();

    void flowLaunchUpdated(FlowLaunchEntity flowLaunch);

    final class NoOpUpdateDelegate implements FlowLaunchUpdateDelegate {

        @Override
        public void flowLaunchUpdated(FlowLaunchEntity flowLaunch) {
            //
        }
    }
}
