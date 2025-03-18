package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.Collections;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.runtime.events.JobStateChangedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobTaskStateChangeEvent;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class JobStateChangesTest extends FlowStateCalculatorTest {
    private static final String JOB_ID = "jobId1";

    @Override
    protected Flow getFlow() {
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(DummyJob.ID, JOB_ID);

        return builder.build();
    }

    @Test
    public void globalStatusChanged() {
        recalc(null);

        recalc(new JobStateChangedEvent(JOB_ID, 1, "New Status", .1f,
                Collections.emptyList()
        ));

        JobLaunch lastLaunch = getFlowLaunch().getJobState(JOB_ID).getLastLaunch();
        assertEquals("New Status", lastLaunch.getStatusText());
    }

    @Test
    public void totalProgressChanges() {
        recalc(null);

        recalc(new JobStateChangedEvent(JOB_ID, 1, "New Status", .1f,
                Collections.emptyList()));

        JobLaunch lastLaunch = getFlowLaunch().getJobState(JOB_ID).getLastLaunch();
        assertEquals(.1f, lastLaunch.getTotalProgress(), .0f);
    }

    @Test
    public void taskAdded() {
        recalc(null);

        TaskBadge taskBadge = TaskBadge.of("tsum", "TsumUI", "http://localhost", TaskBadge.TaskStatus.RUNNING);

        recalc(new JobStateChangedEvent(JOB_ID, 1, "New Status", .1f,
                Collections.singletonList(taskBadge)));

        JobLaunch lastLaunch = getFlowLaunch().getJobState(JOB_ID).getLastLaunch();
        assertEquals(lastLaunch.getTaskStates().get(0), taskBadge);
    }

    @Test
    public void taskRemoved() {
        recalc(null);

        TaskBadge taskBadge = TaskBadge.of("tsum", "TsumUI", "http://localhost", TaskBadge.TaskStatus.RUNNING);

        recalc(new JobStateChangedEvent(JOB_ID, 1, "New Status", .1f,
                Collections.singletonList(taskBadge)));

        JobLaunch lastLaunch = getFlowLaunch().getJobState(JOB_ID).getLastLaunch();
        assertEquals(lastLaunch.getTaskStates().get(0), taskBadge);

        recalc(new JobStateChangedEvent(JOB_ID, 1, "New Status", .1f,
                Collections.emptyList()));

        assertTrue(lastLaunch.getTaskStates().isEmpty());
    }

    @Test
    public void taskUpdated() {
        recalc(null);

        TaskBadge taskletState = TaskBadge.of("tasklet", "Tasklet", "http://localhost", TaskBadge.TaskStatus.RUNNING);
        TaskBadge deployState = TaskBadge.of("deploy", "Deploy", "http://localhost", TaskBadge.TaskStatus.RUNNING);

        recalc(new JobTaskStateChangeEvent(JOB_ID, 1, taskletState));
        recalc(new JobTaskStateChangeEvent(JOB_ID, 1, deployState));

        JobLaunch lastLaunch = getFlowLaunch().getJobState(JOB_ID).getLastLaunch();
        assertEquals(lastLaunch.getTaskStates().get(0), taskletState);
        assertEquals(lastLaunch.getTaskStates().get(1), deployState);

        TaskBadge updatedDeployState = deployState.withStatus(TaskBadge.TaskStatus.SUCCESSFUL);
        recalc(new JobTaskStateChangeEvent(JOB_ID, 1, updatedDeployState));

        lastLaunch = getFlowLaunch().getJobState(JOB_ID).getLastLaunch();
        assertEquals(lastLaunch.getTaskStates().get(0), taskletState);
        assertEquals(lastLaunch.getTaskStates().get(1), updatedDeployState);
    }

}
