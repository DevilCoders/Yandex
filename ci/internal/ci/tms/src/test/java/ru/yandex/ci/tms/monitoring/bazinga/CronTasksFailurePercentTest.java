package ru.yandex.ci.tms.monitoring.bazinga;

import java.util.UUID;

import javax.annotation.Nullable;

import org.joda.time.Duration;
import org.joda.time.Instant;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Answers;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.bolts.collection.ListF;
import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.common.bazinga.monitoring.CronTasksFailurePercent;
import ru.yandex.commune.bazinga.BazingaControllerApp;
import ru.yandex.commune.bazinga.impl.CronJob;
import ru.yandex.commune.bazinga.impl.CronTaskState;
import ru.yandex.commune.bazinga.impl.JobId;
import ru.yandex.commune.bazinga.impl.JobInfoValue;
import ru.yandex.commune.bazinga.impl.JobStatus;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.impl.controller.BazingaController;
import ru.yandex.commune.bazinga.impl.controller.ControllerCronTask;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;
import ru.yandex.commune.bazinga.impl.worker.BazingaHostPort;
import ru.yandex.commune.bazinga.scheduler.CronTaskInfo;
import ru.yandex.commune.bazinga.scheduler.schedule.SchedulePeriodic;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class CronTasksFailurePercentTest extends CommonTestBase {
    private BazingaStorage bazingaStorage;
    private CronTasksFailurePercent sut;
    private BazingaController bazingaController;

    private static final TaskId TASK_ID_1 = new TaskId("taskId1");
    private static final TaskId TASK_ID_2 = new TaskId("taskId2");
    private static final TaskId TASK_ID_3 = new TaskId("taskId3");
    private BazingaControllerApp bazingaControllerApp;

    @BeforeEach
    public void setUp() {
        bazingaStorage = mock(BazingaStorage.class);
        bazingaControllerApp = mock(BazingaControllerApp.class, Answers.RETURNS_DEEP_STUBS);

        sut = new CronTasksFailurePercent(bazingaControllerApp, bazingaStorage, 1, null);
        bazingaController = mock(BazingaController.class);

        when(bazingaControllerApp.getBazingaController()).thenReturn(bazingaController);
    }

    @Test
    public void noFailingCronJobs() {
        run(
                Cf.list(job(JobStatus.COMPLETED, null)),
                Cf.list(job(JobStatus.RUNNING, null)),
                Cf.list(job(JobStatus.FAILED, "bla-bla InterruptedException bla-bla"))
        );

        assertThat(this.sut.getFailurePercent()).isEqualTo(0f);
    }

    @Test
    public void oneFailingCronTaskTooLongCrit() {
        run(
                Cf.list(job(JobStatus.FAILED, null), job(JobStatus.FAILED, null)),
                Cf.list(),
                Cf.list()
        );

        assertThat(this.sut.getFailurePercent()).isEqualTo(33.333332f);
    }

    private void run(
            ListF<CronJob> taskId1Jobs,
            ListF<CronJob> taskId2Jobs,
            ListF<CronJob> taskId3Jobs
    ) {
        when(bazingaController.isMaster()).thenReturn(true);
        when(bazingaControllerApp.getTaskRegistry().getCronTasks())
                .thenReturn(
                        Cf.list(TASK_ID_1, TASK_ID_2, TASK_ID_3)
                                .map(CronTasksFailurePercentTest::createControllerCronTask)
                );
        when(bazingaStorage.findCronTaskStates())
                .thenReturn(
                        Cf.list(TASK_ID_1, TASK_ID_2, TASK_ID_3).map(CronTasksFailurePercentTest::createCronTaskState)
                );
        when(bazingaStorage.findLatestCronJobs(eq(TASK_ID_1), any())).thenReturn(taskId1Jobs);
        when(bazingaStorage.findLatestCronJobs(eq(TASK_ID_2), any())).thenReturn(taskId2Jobs);
        when(bazingaStorage.findLatestCronJobs(eq(TASK_ID_3), any())).thenReturn(taskId3Jobs);
        sut.run();
    }

    private static CronTaskState createCronTaskState(TaskId taskId) {
        return new CronTaskState(
                new JobId(UUID.randomUUID()),
                Option.empty(),
                taskId,
                JobStatus.COMPLETED,
                new BazingaHostPort("localhost", 8080),
                Instant.now(),
                Option.empty(),
                Option.empty(),
                Option.empty(),
                Option.empty(),
                Option.empty(),
                Option.empty(),
                Option.empty(),
                Option.empty()
        );
    }

    private static ControllerCronTask createControllerCronTask(TaskId taskId) {
        return new ControllerCronTask(
                new CronTaskInfo(taskId, new SchedulePeriodic(Duration.standardMinutes(2)), null, null, null, 0, null),
                createCronTaskState(taskId), null, null
        );
    }

    private static CronJob job(JobStatus status, @Nullable String exceptionMessage) {
        return new CronJob(
                null,
                new JobInfoValue(
                        status,
                        null,
                        null,
                        null,
                        Option.ofNullable(exceptionMessage),
                        null,
                        null,
                        null
                )
        );
    }
}
