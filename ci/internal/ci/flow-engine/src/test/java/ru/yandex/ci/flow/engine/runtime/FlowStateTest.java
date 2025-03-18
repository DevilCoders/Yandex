package ru.yandex.ci.flow.engine.runtime;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchState;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchStatistics;
import ru.yandex.ci.flow.engine.runtime.test_data.common.FailingJob;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

public class FlowStateTest extends FlowEngineTestBase {
    private static final String FIRST_JOB_ID = "FIRST_JOB";
    private static final String STAGE_GROUP_ID = "test-stages";

    private final StageGroup stageGroup = new StageGroup("first");

    @Test
    public void updateStateStagedFlow() {
        // arrange
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(FailingJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStage("first"));

        Flow flow = builder.build();


        FlowLaunchId launchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);


        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        assertThat(flowLaunch.getState()).isEqualTo(LaunchState.FAILURE);
    }

    @Test
    public void updateStateInRegularFlow() {
        // arrange
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(FailingJob.ID, FIRST_JOB_ID);

        Flow flow = builder.build();


        FlowLaunchId launchId = flowTester.runFlowToCompletion(flow);


        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        assertThat(flowLaunch.getState()).isEqualTo(LaunchState.FAILURE);
    }

    @Test
    void fromJson() {
        LaunchStatistics statistics = TestUtils.parseJson("failed-statistics.json", LaunchStatistics.class);
        LaunchState state = LaunchState.fromStatistics(statistics);
        assertThat(state).isEqualTo(LaunchState.FAILURE);
    }

}
