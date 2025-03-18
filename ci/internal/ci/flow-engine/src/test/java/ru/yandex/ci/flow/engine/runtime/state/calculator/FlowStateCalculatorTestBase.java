package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.concurrent.atomic.AtomicInteger;

import javax.annotation.Nullable;

import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FlowProvider;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobScheduler;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobWaitingScheduler;
import ru.yandex.ci.flow.engine.runtime.state.FlowLaunchFactory;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.LaunchParameters;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;

public abstract class FlowStateCalculatorTestBase extends FlowEngineTestBase {
    public static final String USERNAME = TestData.USER42;

    @Autowired
    protected FlowProvider flowProvider;

    @Autowired
    protected FlowStateCalculator flowStateCalculator;

    @Autowired
    protected FlowLaunchFactory flowLaunchFactory;

    @Autowired
    protected TestJobScheduler testJobScheduler;

    @Autowired
    protected TestJobWaitingScheduler testJobWaitingScheduler;

    @Autowired
    protected FlowStateService flowStateService;

    private final AtomicInteger launchNumberSeq = new AtomicInteger(0);

    protected FlowLaunchId activateLaunch(FlowFullId flowId) {
        return this.activateLaunch(flowId, null);
    }

    protected FlowLaunchId activateLaunch(FlowFullId flowId, @Nullable String stageGroupId) {

        LaunchFlowInfo flowInfo = FlowTestUtils.toFlowInfo(flowId, stageGroupId);
        try {
            var launchNumber = launchNumberSeq.incrementAndGet();
            return flowStateService
                    .activateLaunch(
                            LaunchParameters.builder()
                                    .flowInfo(flowInfo)
                                    .vcsInfo(FlowTestUtils.VCS_INFO)
                                    .launchId(new LaunchId(CiProcessId.ofFlow(flowInfo.getFlowId()), launchNumber))
                                    .launchInfo(LaunchInfo.of("" + launchNumber))
                                    .projectId("prj")
                                    .flow(flowProvider.get(flowInfo.getFlowId()))
                                    .triggeredBy(TestData.USER42)
                                    .build()
                    ).getFlowLaunchId();
        } catch (AYamlValidationException e) {
            throw new RuntimeException(e);
        }
    }
}
