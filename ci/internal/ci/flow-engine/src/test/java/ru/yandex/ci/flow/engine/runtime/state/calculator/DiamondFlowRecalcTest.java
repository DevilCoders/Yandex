package ru.yandex.ci.flow.engine.runtime.state.calculator;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.test_data.diamond.DiamondFlow;

public class DiamondFlowRecalcTest extends FlowStateCalculatorTest {

    @Override
    protected Flow getFlow() throws AYamlValidationException {
        return DiamondFlow.flow();
    }

    @Test
    public void triggerLastJobManually() {
        runToTheEnd();

        // Триггернём последнюю джобу ещё раз и убедимся, что её состояние не посчиталось дважды
        recalc(new TriggerEvent(DiamondFlow.END_JOB, USERNAME, false));

        Assertions.assertEquals(1, getTriggeredJobs().size());
    }

    @Test
    public void triggerFirstJobManually() {
        runToTheEnd();

        recalc(new TriggerEvent(DiamondFlow.START_JOB, USERNAME, false));

        Assertions.assertEquals(1, getTriggeredJobs().size());
        Assertions.assertTrue(getFlowLaunch().getJobState(DiamondFlow.TOP_JOB).isOutdated());
        Assertions.assertTrue(getFlowLaunch().getJobState(DiamondFlow.BOTTOM_JOB).isOutdated());
        Assertions.assertTrue(getFlowLaunch().getJobState(DiamondFlow.END_JOB).isOutdated());
    }

    private void runToTheEnd() {
        recalc(null);
        Assertions.assertEquals(1, getTriggeredJobs().size());
        getTriggeredJobs().clear();

        recalc(new JobRunningEvent(DiamondFlow.START_JOB, 1, DummyTmsTaskIdFactory.create()));
        recalc(new JobExecutorSucceededEvent(DiamondFlow.START_JOB, 1));
        recalc(new SubscribersSucceededEvent(DiamondFlow.START_JOB, 1));
        recalc(new JobSucceededEvent(DiamondFlow.START_JOB, 1));

        Assertions.assertEquals(2, getTriggeredJobs().size());
        getTriggeredJobs().clear();

        recalc(new JobRunningEvent(DiamondFlow.TOP_JOB, 1, DummyTmsTaskIdFactory.create()));
        recalc(new JobRunningEvent(DiamondFlow.BOTTOM_JOB, 1, DummyTmsTaskIdFactory.create()));
        recalc(new JobExecutorSucceededEvent(DiamondFlow.TOP_JOB, 1));
        recalc(new JobExecutorSucceededEvent(DiamondFlow.BOTTOM_JOB, 1));
        recalc(new SubscribersSucceededEvent(DiamondFlow.TOP_JOB, 1));
        recalc(new SubscribersSucceededEvent(DiamondFlow.BOTTOM_JOB, 1));
        recalc(new JobSucceededEvent(DiamondFlow.TOP_JOB, 1));
        recalc(new JobSucceededEvent(DiamondFlow.BOTTOM_JOB, 1));

        Assertions.assertEquals(1, getTriggeredJobs().size());
        getTriggeredJobs().clear();

        recalc(new JobRunningEvent(DiamondFlow.END_JOB, 1, DummyTmsTaskIdFactory.create()));
        recalc(new JobExecutorSucceededEvent(DiamondFlow.END_JOB, 1));
        recalc(new SubscribersSucceededEvent(DiamondFlow.END_JOB, 1));
        recalc(new JobSucceededEvent(DiamondFlow.END_JOB, 1));
    }

}
