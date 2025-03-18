package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import org.junit.jupiter.api.BeforeEach;

import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.runtime.events.JobEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.ScheduleCommand;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.test.TestFlowId;

/**
 * Абстрактный класс для тестирования модуля пересчёта флоу.
 */
public abstract class FlowStateCalculatorTest extends FlowStateCalculatorTestBase {
    private FlowLaunchEntity flowLaunch;
    private List<ScheduleCommand> triggeredJobs;

    /**
     * Flow, который нужно выполнить в рамках этого теста
     *
     * @return flow для регистрации
     */
    protected abstract Flow getFlow() throws AYamlValidationException;

    void recalc(JobEvent event) {
        triggeredJobs.addAll(
                flowStateCalculator.recalc(flowLaunch, event).getPendingCommands().stream()
                        .filter(ScheduleCommand.class::isInstance)
                        .map(ScheduleCommand.class::cast)
                        .collect(Collectors.toList())
        );
    }

    List<ScheduleCommand> getTriggeredJobs() {
        return triggeredJobs;
    }

    FlowLaunchEntity getFlowLaunch() {
        return flowLaunch;
    }

    @BeforeEach
    public void setUp() throws Exception {
        var flow = getFlow();
        var flowId = flowTester.register(flow, TestFlowId.TEST_PATH);

        flowLaunch = flowLaunchFactory.create(FlowTestUtils.launchParametersBuilder(flowId, flow));
        triggeredJobs = new ArrayList<>();
    }
}
