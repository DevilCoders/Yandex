package ru.yandex.ci.flow.engine.runtime.state.model;

import java.time.Instant;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.common.UpstreamStyle;
import ru.yandex.ci.flow.engine.definition.common.UpstreamType;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.test.TestFlowId;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class FlowLaunchEntityTest {
    @Test
    public void getDownstreamsRecursive_noDownstreams() {
        JobState job1 = jobState("1");
        assertTrue(flowLaunch(job1).getDownstreamsRecursive("1").isEmpty());
    }

    @Test
    public void getDownstreamsRecursive_twoDownstreams() {
        JobState job1 = jobState("1");
        JobState job2 = jobState("2", "1");
        JobState job3 = jobState("3", "2");

        assertThat(flowLaunch(job1, job2, job3).getDownstreamsRecursive("1"))
                .containsExactlyInAnyOrder(job2, job3);

    }

    @Test
    public void getDownstreamsRecursive_diamond() {
        JobState job1 = jobState("1");
        JobState job2 = jobState("2", "1");
        JobState job3 = jobState("3", "1");
        JobState job4 = jobState("4", "2", "3");

        assertThat(flowLaunch(job1, job2, job3, job4).getDownstreamsRecursive("1"))
                .containsExactlyInAnyOrder(job2, job3, job4);
    }


    private static FlowLaunchEntity flowLaunch(JobState... jobStates) {
        FlowLaunchRef launchRef = FlowLaunchRefImpl.create(TestFlowId.of("flow-id"));

        return FlowLaunchEntity.builder()
                .id(launchRef.getFlowLaunchId())
                .createdDate(Instant.now())
                .flowInfo(FlowTestUtils.toFlowInfo(launchRef.getFlowFullId(), null))
                .launchInfo(LaunchInfo.of("771"))
                .jobs(Stream.of(jobStates).collect(Collectors.toMap(
                        JobState::getJobId,
                        Function.identity()
                )))
                .launchId(FlowTestUtils.LAUNCH_ID)
                .vcsInfo(FlowTestUtils.VCS_INFO)
                .projectId("prj")
                .build();
    }

    private static JobState jobState(String id, String... upstreamIds) {
        Job job = mock(Job.class);
        var context = ExecutorContext.internal(InternalExecutorContext.of(DummyJob.ID));
        when(job.getExecutorContext()).thenReturn(context);
        when(job.getId()).thenReturn(id);
        return new JobState(
                job,
                Stream.of(upstreamIds)
                        .map(upstreamId -> new UpstreamLink<>(upstreamId, UpstreamType.ALL_RESOURCES,
                                UpstreamStyle.SPLINE))
                        .collect(Collectors.toSet()),
                ResourceRefContainer.empty(),
                null
        );
    }
}
