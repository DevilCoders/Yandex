package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.Set;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobForceSuccessEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

import static org.assertj.core.api.Assertions.assertThat;

public class JobWithConditionRunTest extends FlowStateCalculatorTest {

    private static final String JOB_ID_1 = "J1";
    private static final String JOB_ID_2 = "J2";
    private static final String JOB_ID_3 = "J3";
    private static final String JOB_ID_4 = "J4";

    @Override
    protected Flow getFlow() {
        var builder = FlowBuilder.create();

        var j1 = builder.withJob(DummyJob.ID, JOB_ID_1)
                .withManualTrigger();

        var j2 = builder.withJob(DummyJob.ID, JOB_ID_2)
                .withUpstreams(j1);

        var j3 = builder.withJob(DummyJob.ID, JOB_ID_3)
                .withUpstreams(j1);

        builder.withJob(DummyJob.ID, JOB_ID_4)
                .withUpstreams(j2, j3);

        return builder.build();
    }

    @Test
    public void triggerJob1() {
        recalc(null);
        recalc(new TriggerEvent(JOB_ID_1, USERNAME, false));

        assertThat(getTriggeredJobs().size()).isEqualTo(1);
        assertThat(getTriggeredJobs().get(0).getJobLaunchId().getJobId()).isEqualTo(JOB_ID_1);

        var j1State = getFlowLaunch().getJobs().get(JOB_ID_1);

        assertThat(j1State.getLastStatusChangeType()).isEqualTo(StatusChangeType.QUEUED);
    }

    @Test
    public void triggerJob23() {
        recalc(null);
        recalc(new TriggerEvent(JOB_ID_1, USERNAME, false));

        getTriggeredJobs().clear();
        recalc(new JobRunningEvent(JOB_ID_1, 1, DummyTmsTaskIdFactory.create()));
        recalc(new JobExecutorSucceededEvent(JOB_ID_1, 1));
        recalc(new SubscribersSucceededEvent(JOB_ID_1, 1));
        recalc(new JobSucceededEvent(JOB_ID_1, 1));

        assertThat(getFlowLaunch().getJobState(JOB_ID_1).isReadyToRun()).isTrue();
        assertThat(getTriggeredJobs().size()).isEqualTo(2);

        assertThat(getTriggeredJobs().stream()
                .map(j -> j.getJobLaunchId().getJobId())
                .collect(Collectors.toSet())).isEqualTo(Set.of(JOB_ID_3, JOB_ID_2));

        assertThat(getFlowLaunch().getJobState(JOB_ID_2).getLastStatusChangeType())
                .isEqualTo(StatusChangeType.QUEUED);
        assertThat(getFlowLaunch().getJobState(JOB_ID_3).getLastStatusChangeType())
                .isEqualTo(StatusChangeType.QUEUED);

        getTriggeredJobs().clear();

        // JOB_ID_2 - как бы исполняется
        // JOB_ID_3 - как бы не исполняется

        recalc(new JobRunningEvent(JOB_ID_2, 1, DummyTmsTaskIdFactory.create()));
        recalc(new JobForceSuccessEvent(JOB_ID_3, 1, DummyTmsTaskIdFactory.create()));

        assertThat(getTriggeredJobs().size()).isEqualTo(0);

        recalc(new JobExecutorSucceededEvent(JOB_ID_2, 1));
        recalc(new SubscribersSucceededEvent(JOB_ID_2, 1));
        recalc(new JobSucceededEvent(JOB_ID_2, 1));
        assertThat(getFlowLaunch().getJobState(JOB_ID_2).isReadyToRun()).isTrue();

        // Ждем JOB_ID_3
        assertThat(getTriggeredJobs().size()).isEqualTo(0);

        assertThat(getFlowLaunch().getJobState(JOB_ID_2).getLastStatusChangeType())
                .isEqualTo(StatusChangeType.SUCCESSFUL);
        assertThat(getFlowLaunch().getJobState(JOB_ID_3).getLastStatusChangeType())
                .isEqualTo(StatusChangeType.FORCED_EXECUTOR_SUCCEEDED);

        recalc(new SubscribersSucceededEvent(JOB_ID_3, 1));
        recalc(new JobSucceededEvent(JOB_ID_3, 1));
        assertThat(getFlowLaunch().getJobState(JOB_ID_3).isReadyToRun()).isTrue();

        // Последний JOB
        assertThat(getTriggeredJobs().size()).isEqualTo(1);

        assertThat(getFlowLaunch().getJobState(JOB_ID_4).getLastStatusChangeType())
                .isEqualTo(StatusChangeType.QUEUED);

        getTriggeredJobs().clear();

        recalc(new JobRunningEvent(JOB_ID_4, 1, DummyTmsTaskIdFactory.create()));
        recalc(new JobExecutorSucceededEvent(JOB_ID_4, 1));
        recalc(new SubscribersSucceededEvent(JOB_ID_4, 1));
        recalc(new JobSucceededEvent(JOB_ID_4, 1));

        assertThat(getTriggeredJobs().size()).isEqualTo(0);
    }
}
