package ru.yandex.ci.common.bazinga.monitoring;

import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.assertj.core.api.Assertions;
import org.joda.time.Instant;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.stubbing.Answer;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.bolts.collection.ListF;
import ru.yandex.bolts.collection.Option;
import ru.yandex.commune.bazinga.BazingaControllerApp;
import ru.yandex.commune.bazinga.impl.FullJobId;
import ru.yandex.commune.bazinga.impl.JobId;
import ru.yandex.commune.bazinga.impl.JobInfoValue;
import ru.yandex.commune.bazinga.impl.JobStatus;
import ru.yandex.commune.bazinga.impl.OnetimeJob;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.impl.controller.BazingaController;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskRegistry;
import ru.yandex.commune.bazinga.ydb.storage.YdbBazingaStorage;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class OnetimeJobRetriesTest {
    private YdbBazingaStorage bazingaStorage;
    private OnetimeJobRetries sut;
    private BazingaController bazingaController;
    private WorkerTaskRegistry workerTaskRegistry;

    private static final TaskId TASK_ID_1 = new TaskId("taskId1");
    private static final TaskId TASK_ID_2 = new TaskId("taskId2");

    @BeforeEach
    public void setUp() {
        bazingaStorage = mock(YdbBazingaStorage.class);
        workerTaskRegistry = mock(WorkerTaskRegistry.class);

        when(workerTaskRegistry.getOnetimeTasks()).thenReturn(Cf.list(
                new FakeOneTimeTask(TASK_ID_1),
                new FakeOneTimeTask(TASK_ID_2)
        ));

        BazingaControllerApp bazingaControllerApp = mock(BazingaControllerApp.class);
        sut = new OnetimeJobRetries(bazingaControllerApp, bazingaStorage, workerTaskRegistry, null, 42);
        bazingaController = mock(BazingaController.class);
        when(bazingaControllerApp.getBazingaController()).thenReturn(bazingaController);
    }

    @Test
    public void oneFailingJobInTask1() {
        run(Cf.list(jobType1(43, JobStatus.RUNNING)), Cf.list(jobType2(42, JobStatus.FAILED)));
        Assertions.assertThat(sut.getJobsWithRetryExceededTotal()).isEqualTo(1);
        Assertions.assertThat(sut.getJobsWithRetryExceededByTaskType()).isEqualTo(Map.of(
                TASK_ID_1, 1,
                TASK_ID_2, 0
        ));
    }

    @Test
    public void oneFailingJobInTask2() {
        run(Cf.list(jobType1(42, JobStatus.READY)), Cf.list(jobType2(43, JobStatus.READY)));
        Assertions.assertThat(sut.getJobsWithRetryExceededTotal()).isEqualTo(1);
        Assertions.assertThat(sut.getJobsWithRetryExceededByTaskType()).isEqualTo(Map.of(
                TASK_ID_1, 0,
                TASK_ID_2, 1
        ));
    }

    @Test
    public void oneCompletedJobWithTooManyRetries() {
        run(Cf.list(jobType1(6, JobStatus.COMPLETED)), Cf.list());
        Assertions.assertThat(sut.getJobsWithRetryExceededTotal()).isEqualTo(0);

        Assertions.assertThat(sut.getJobsWithRetryExceededByTaskType()).isEqualTo(Map.of(
                TASK_ID_1, 0,
                TASK_ID_2, 0
        ));
    }

    private void run(ListF<OnetimeJob> taskId1Jobs, ListF<OnetimeJob> taskId2Jobs) {
        when(bazingaController.isMaster()).thenReturn(true);
        mockFindLatestOnetimeJobs(TASK_ID_1, taskId1Jobs);
        mockFindLatestOnetimeJobs(TASK_ID_2, taskId2Jobs);
        sut.run();
    }

    private void mockFindLatestOnetimeJobs(TaskId taskId, List<OnetimeJob> jobs) {
        mockJobCount(taskId, jobs);
    }

    private void mockJobCount(TaskId taskId, List<OnetimeJob> jobs) {
        for (JobStatus status : JobStatus.values()) {
            when(bazingaStorage.findOnetimeJobCount(eq(taskId), eq(status), any(), any()))
                    .thenAnswer((Answer<Integer>) invocation -> {
                        Option<Integer> attempts = invocation.getArgument(3);
                        var count = count(jobs, status, attempts.orElseThrow());
                        return count;
                    });
        }
    }

    private static int count(List<OnetimeJob> jobs, JobStatus jobStatus, int attempts) {
        return (int) jobs.stream()
                .filter(job -> job.getAttempt().orElse(0) > attempts)
                .map(OnetimeJob::getValue)
                .map(JobInfoValue::getStatus)
                .filter(status -> status == jobStatus)
                .count();
    }

    private static OnetimeJob jobType1(int attempts, JobStatus status) {
        return job(TASK_ID_1, attempts, status, Instant.now());
    }

    private static OnetimeJob jobType2(int attempts, JobStatus status) {
        return job(TASK_ID_2, attempts, status, Instant.now());
    }

    private static OnetimeJob job(TaskId taskId, int attempts, JobStatus status, Instant creationDate) {
        return new OnetimeJob(
                new FullJobId(taskId, new JobId(UUID.randomUUID())),
                Option.of(creationDate),
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
                Option.of(attempts),
                null,
                null,
                0,
                null
        );
    }

    private static class FakeOneTimeTask extends TestOneTimeTask {
        private final TaskId taskId;

        private FakeOneTimeTask(TaskId taskId) {
            this.taskId = taskId;
        }

        @Override
        public TaskId id() {
            return taskId;
        }
    }
}
