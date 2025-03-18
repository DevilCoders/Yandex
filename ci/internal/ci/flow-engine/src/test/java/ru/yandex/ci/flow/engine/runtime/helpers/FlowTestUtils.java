package ru.yandex.ci.flow.engine.runtime.helpers;

import javax.annotation.Nullable;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.FlowLaunchParameters;
import ru.yandex.ci.flow.engine.runtime.state.LaunchParameters;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
import ru.yandex.ci.flow.engine.runtime.test_data.simple.SimpleFlow;

public class FlowTestUtils {

    public static final CiProcessId PROCESS_ID = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "my-release");

    public static final LaunchId LAUNCH_ID = new LaunchId(PROCESS_ID, 421);

    public static final LaunchId ACTION_LAUNCH_ID
            = new LaunchId(CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "flow-1"), 12345);

    public static final LaunchFlowInfo SIMPLE_FLOW_INFO = toFlowInfo(SimpleFlow.FLOW_ID, null);
    public static final LaunchInfo SIMPLE_LAUNCH_INFO = LaunchInfo.of("421");

    public static final LaunchVcsInfo VCS_INFO = LaunchVcsInfo.builder()
            .revision(OrderedArcRevision.fromHash("b2", "trunk", 42, 0))
            .previousRevision(OrderedArcRevision.fromHash("b1", "trunk", 21, 0))
            .commitCount(7)
            .commit(TestData.TRUNK_COMMIT_2)
            .build();

    private FlowTestUtils() {
    }

    public static LaunchFlowInfo toFlowInfo(FlowFullId flowId, @Nullable String stageGroupId) {
        return LaunchFlowInfo.builder()
                .configRevision(TestData.TRUNK_R1)
                .flowId(flowId)
                .stageGroupId(stageGroupId)
                .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                .build();
    }

    public static LaunchParameters prepareLaunchParametersBuilder(FlowFullId flowId, Flow flow) {
        LaunchFlowInfo flowInfo = FlowTestUtils.toFlowInfo(flowId, null);
        return LaunchParameters.builder()
                .launchId(new LaunchId(CiProcessId.ofFlow(flowInfo.getFlowId()), 421))
                .launchInfo(LaunchInfo.of("421"))
                .flowInfo(flowInfo)
                .vcsInfo(FlowTestUtils.VCS_INFO)
                .triggeredBy(TestData.CI_USER)
                .flow(flow)
                .projectId("prj")
                .build();
    }

    public static FlowLaunchParameters launchParametersBuilder(FlowFullId flowId, Flow flow) {
        LaunchFlowInfo flowInfo = FlowTestUtils.toFlowInfo(flowId, null);
        LaunchId launchId = new LaunchId(CiProcessId.ofFlow(flowInfo.getFlowId()), 421);
        return FlowLaunchParameters.builder()
                .flowLaunchId(FlowLaunchId.of(launchId))
                .launchParameters(LaunchParameters.builder()
                        .launchId(launchId)
                        .launchInfo(LaunchInfo.of("421"))
                        .flowInfo(flowInfo)
                        .vcsInfo(FlowTestUtils.VCS_INFO)
                        .projectId("prj")
                        .triggeredBy(TestData.CI_USER)
                        .flow(flow)
                        .build())
                .build();
    }

}
