package ru.yandex.ci.tms.monitoring.bazinga;


import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.bolts.collection.ListF;
import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.common.bazinga.monitoring.OnetimeJobFailurePercent;
import ru.yandex.commune.bazinga.BazingaControllerApp;
import ru.yandex.commune.bazinga.impl.JobInfoValue;
import ru.yandex.commune.bazinga.impl.JobStatus;
import ru.yandex.commune.bazinga.impl.OnetimeJob;
import ru.yandex.commune.bazinga.impl.OnetimeTaskState;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.impl.controller.BazingaController;
import ru.yandex.commune.bazinga.ydb.storage.YdbBazingaStorage;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class OnetimeJobFailurePercentTest extends CommonTestBase {
    private YdbBazingaStorage bazingaStorage;
    private OnetimeJobFailurePercent sut;
    private BazingaController bazingaController;

    private static final TaskId TASK_ID_1 = new TaskId("taskId1");
    private static final TaskId TASK_ID_2 = new TaskId("taskId2");

    @BeforeEach
    public void setUp() {
        bazingaStorage = mock(YdbBazingaStorage.class);
        BazingaControllerApp bazingaControllerApp = mock(BazingaControllerApp.class);
        sut = new OnetimeJobFailurePercent(bazingaControllerApp, bazingaStorage, null);
        bazingaController = mock(BazingaController.class);
        when(bazingaControllerApp.getBazingaController()).thenReturn(bazingaController);
    }

    @Test
    public void tooManyFailed() {
        run(
                Cf.list(job(JobStatus.RUNNING)),
                Cf.list(job(JobStatus.FAILED), job(JobStatus.COMPLETED))
        );
        sut.run();
        assertThat(this.sut.getFailureCount()).isEqualTo(1L);
    }

    @Test
    public void tooManyExpired() {
        run(
                Cf.list(job(JobStatus.RUNNING)),
                Cf.list(job(JobStatus.EXPIRED), job(JobStatus.COMPLETED))
        );
        sut.run();
        assertThat(this.sut.getFailureCount()).isEqualTo(1L);
    }

    @Test
    public void notTooManyFailed() {
        run(
                Cf.list(job(JobStatus.READY), job(JobStatus.RUNNING)),
                Cf.list(job(JobStatus.FAILED), job(JobStatus.COMPLETED))
        );
        sut.run();
        assertThat(this.sut.getFailureCount()).isEqualTo(1L);
    }

    @Test
    public void notTooManyFailedAndExpired() {
        run(
                Cf.list(job(JobStatus.READY), job(JobStatus.EXPIRED)),
                Cf.list(job(JobStatus.FAILED), job(JobStatus.COMPLETED))
        );
        sut.run();
        assertThat(this.sut.getFailureCount()).isEqualTo(2L);
    }

    private void run(ListF<OnetimeJob> taskId1Jobs, ListF<OnetimeJob> taskId2Jobs) {
        when(bazingaController.isMaster()).thenReturn(true);
        when(bazingaStorage.findOnetimeTaskStates())
                .thenReturn(Cf.list(TASK_ID_1, TASK_ID_2).map(OnetimeTaskState::initial));

        mockJobCount(TASK_ID_1, taskId1Jobs);
        mockJobCount(TASK_ID_2, taskId2Jobs);

        sut.run();
    }

    private void mockJobCount(TaskId taskId, List<OnetimeJob> jobs) {
        for (JobStatus status : JobStatus.values()) {
            var count = count(jobs, status);
            when(bazingaStorage.findOnetimeJobCount(eq(taskId), eq(status), any(), any()))
                    .thenReturn(count);
        }
    }

    private static int count(List<OnetimeJob> jobs, JobStatus jobStatus) {
        return (int) jobs.stream()
                .map(OnetimeJob::getValue)
                .map(JobInfoValue::getStatus)
                .filter(status -> status == jobStatus)
                .count();
    }

    private static OnetimeJob job(JobStatus status) {
        return new OnetimeJob(
                null,
                null,
                null,
                null,
                new JobInfoValue(
                        status,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null,
                        null
                ),
                null,
                null,
                null,
                0,
                null
        );
    }
}
