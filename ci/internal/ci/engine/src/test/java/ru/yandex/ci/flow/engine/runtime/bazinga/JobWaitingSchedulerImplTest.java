package ru.yandex.ci.flow.engine.runtime.bazinga;

import java.time.Instant;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;

import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.commune.bazinga.scheduler.OnetimeTask;
import ru.yandex.commune.bazinga.test.BazingaTaskManagerStub;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

class JobWaitingSchedulerImplTest {

    private BazingaTaskManagerStub bazingaTaskManager;
    private OverridableClock clock;

    @BeforeEach
    void setUp() {
        clock = new OverridableClock();
        bazingaTaskManager = spy(new BazingaTaskManagerStub());
    }

    @Test
    void retry() {
        clock.setTime(Instant.parse("2015-01-01T10:00:00.000Z"));

        var jobWaitingScheduler = new JobWaitingSchedulerImpl(bazingaTaskManager, clock);

        jobWaitingScheduler.retry(
                new JobScheduleTask(
                        new JobScheduleTaskParameters(
                                FlowLaunchId.of(LaunchId.of(TestData.SIMPLE_FLOW_PROCESS_ID, 1)),
                                "job-id", 1, 2
                        )
                ),
                Instant.parse("2099-01-01T10:00:00.000Z")
        );

        var taskArgumentCaptor = ArgumentCaptor.forClass(JobScheduleTask.class);
        verify(bazingaTaskManager).schedule(
                taskArgumentCaptor.capture(), any(),
                eq(org.joda.time.Instant.parse("2099-01-01T10:00:00.000Z")),
                anyInt(), eq(true), any(), any(), any()
        );

        assertThat(taskArgumentCaptor.getValue())
                .extracting(OnetimeTask::getParameters)
                .hasFieldOrPropertyWithValue(
                        "flowLaunchId", FlowLaunchId.of(LaunchId.of(TestData.SIMPLE_FLOW_PROCESS_ID, 1))
                )
                .hasFieldOrPropertyWithValue("jobId", "job-id")
                .hasFieldOrPropertyWithValue("jobLaunchNumber", 1)
                .hasFieldOrPropertyWithValue("schedulerLaunchNumber", 3);
    }
}
