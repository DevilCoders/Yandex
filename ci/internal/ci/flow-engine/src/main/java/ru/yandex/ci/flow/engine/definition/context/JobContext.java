package ru.yandex.ci.flow.engine.definition.context;

import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;

public interface JobContext {
    /**
     * Returns new flow launch context
     *
     * @return flow launch context
     */
    FlowLaunchContext createFlowLaunchContext();

    /**
     * Gets current flow launch.
     *
     * @return flow launch.
     */
    FlowLaunchEntity getFlowLaunch();

    /**
     * Gets job's state.
     *
     * @return job's state.
     */
    JobState getJobState();

    default FullJobLaunchId getFullJobLaunchId() {
        return new FullJobLaunchId(getFlowLaunch().getFlowLaunchId(), getJobId(), getLaunchNumber());
    }

    /**
     * Gets job's id.
     *
     * @return job's id.
     */
    default String getJobId() {
        return getJobState().getJobId();
    }

    /**
     * Gets job's full id.
     *
     * @return job's full id.
     */
    String getFullJobId();

    /**
     * Gets current launch number.
     *
     * @return launch number.
     */
    int getLaunchNumber();

    /**
     * Gets progress context.
     *
     * @return progress const.
     */
    JobProgressContext progress();

    /**
     * Gets actions context.
     *
     * @return actions context.
     */
    JobActionsContext actions();

    /**
     * Gets resources context.
     *
     * @return resources context.
     */
    JobResourcesContext resources();
}
