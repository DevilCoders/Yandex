package ru.yandex.ci.engine.flow;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.SandboxResponse;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.BatchStatus;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.config.registry.SandboxTaskBadgesConfig;
import ru.yandex.ci.core.db.TestCiDbUtils;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.job.JobInstanceTable;
import ru.yandex.ci.core.job.JobStatus;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.engine.SandboxTaskService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.flow.utils.UrlService;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class SandboxTaskExecutorTest {
    private static final String SANDBOX_URL = "sandbox-url";

    @Mock
    private TaskletMetadataService taskletMetadataService;
    @Mock
    private CiDb db;
    @Mock
    private JobInstanceTable jobInstanceTable;
    @Mock
    private TaskBadgeService taskBadgeService;
    @Mock
    private SandboxClient sandboxClient;

    private SandboxTaskService sandboxTaskService;
    private SandboxTaskLauncher sandboxTaskLauncher;
    private SandboxTaskExecutor sandboxTaskExecutor;

    private final long taskId = 1234;

    @BeforeEach
    public void setUp() {
        TestCiDbUtils.mockToCallRealTxMethods(db);

        when(db.jobInstance()).thenReturn(jobInstanceTable);

        var clock = CommonConfig.defaultClock();
        SandboxTaskPollerSettings pollerSettings = new SandboxTaskPollerSettings(
                clock, Duration.ofMillis(0), Duration.ofMillis(0), Duration.ofMillis(0), 1, 1, 0
        );

        sandboxTaskLauncher = new SandboxTaskLauncher(
                mock(SchemaService.class),
                db,
                mock(SecurityAccessService.class),
                mock(SandboxClientFactory.class),
                mock(UrlService.class),
                "url",
                taskBadgeService,
                new TaskletContextProcessor(mock(UrlService.class))
        );
        sandboxTaskService = new SandboxTaskService(sandboxClient, "CI", "sec-abcd");

        sandboxTaskExecutor = new SandboxTaskExecutor(
                db, sandboxTaskLauncher, taskletMetadataService, pollerSettings, taskBadgeService
        );


    }

    @SuppressWarnings("unchecked")
    @Test
    void waitTaskUpdateTaskBadges() throws Exception {
        JobInstance.Id instanceKey = JobInstance.Id.of("someTestFlowLaunchId", "testJobId", 1);

        JobInstance instance = createJobInstance(instanceKey, JobStatus.RUNNING, null);
        JobInstance finishedInstance = createJobInstance(instanceKey, JobStatus.SUCCESS, SandboxTaskStatus.SUCCESS);

        List<TaskBadge> badges = createTaskBadges(3);
        TaskBadge changedBadge = TaskBadge.of(
                "report_0", "SANDBOX", SANDBOX_URL + "/other_report", TaskBadge.TaskStatus.SUCCESSFUL
        );

        when(jobInstanceTable.get(instanceKey)).thenReturn(instance);

        when(sandboxClient.getTask(taskId)).thenReturn(
                SandboxTestData.uniqTaskOutput(SandboxTaskStatus.EXECUTING),
                SandboxTestData.uniqTaskOutput(SandboxTaskStatus.EXECUTING),
                SandboxTestData.uniqTaskOutput(SandboxTaskStatus.EXECUTING),
                SandboxTestData.uniqTaskOutput(SandboxTaskStatus.EXECUTING),
                SandboxTestData.uniqTaskOutput(SandboxTaskStatus.SUCCESS)
        );

        reset(taskBadgeService);
        when(taskBadgeService.toTaskBadges(any(), any(), any())).thenReturn(
                List.of(badges.get(0), badges.get(1)),
                List.of(badges.get(0), badges.get(1)),
                List.of(changedBadge, badges.get(1), badges.get(2)),
                List.of(changedBadge, badges.get(1), badges.get(2)),
                List.of(changedBadge, badges.get(1), badges.get(2))
        );

        sandboxTaskExecutor.waitTask(
                sandboxTaskService,
                instance,
                List.of(
                        SandboxTaskBadgesConfig.of("report_0"),
                        SandboxTaskBadgesConfig.of("report_1"),
                        SandboxTaskBadgesConfig.of("report_2")
                ),
                false,
                false
        );

        // Updates 2 times: once in updateJobInstance then next in updateJobInstanceFinal
        verify(jobInstanceTable, times(2)).save(finishedInstance);
        verify(taskBadgeService).updateTaskBadges(instance, List.of(badges.get(0), badges.get(1)));
        verify(taskBadgeService).updateTaskBadges(instance, List.of(changedBadge, badges.get(2)));
    }


    @SuppressWarnings("unchecked")
    @Test
    void updateTaskBadgesForRunningTask() throws InterruptedException {

        when(taskBadgeService.toTaskBadges(any(), any(), any())).thenCallRealMethod();

        JobInstance.Id instanceKey = JobInstance.Id.of("someTestFlowLaunchId", "testJobId", 1);

        JobInstance instance = JobInstance.builder()
                .id(instanceKey)
                .sandboxTaskId(taskId)
                .status(JobStatus.RUNNING)
                .build();

        SandboxTaskOutput.Report report = new SandboxTaskOutput.Report(
                "http://test_data_url.ru/something", "report", "Report"
        );

        when(jobInstanceTable.get(instanceKey)).thenReturn(instance);

        when(sandboxClient.getTask(taskId)).thenReturn(
                SandboxResponse.of(
                        new SandboxTaskOutput(
                                "TEST_TASK", taskId, SandboxTaskStatus.EXECUTING, Map.of(), Map.of(),
                                List.of(report)
                        ),
                        21, 42
                ),
                SandboxResponse.of(
                        new SandboxTaskOutput(
                                "TEST_TASK", taskId, SandboxTaskStatus.FAILURE, Map.of(), Map.of(),
                                List.of()
                        ),
                        21, 42
                )
        );

        sandboxTaskExecutor.waitTask(
                sandboxTaskService,
                instance,
                List.of(SandboxTaskBadgesConfig.of("report")),
                false,
                false
        );

        TaskBadge expected = TaskBadge.of(
                report.getLabel(),
                "SANDBOX",
                sandboxTaskLauncher.makeTaskUrl(taskId) + "/" + report.getLabel(),
                TaskBadge.TaskStatus.SUCCESSFUL
        );

        verify(taskBadgeService).updateTaskBadges(instance, List.of(expected));

    }

    @SuppressWarnings("unchecked")
    @Test
    void startDraftTask() throws Exception {

        when(sandboxClient.getTaskStatus(taskId)).thenReturn(
                SandboxTestData.taskStatus(SandboxTaskStatus.DRAFT),
                SandboxTestData.taskStatus(SandboxTaskStatus.SUCCESS)
        );

        when(sandboxClient.getTask(taskId)).thenReturn(SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS));
        when(sandboxClient.startTask(eq(taskId), anyString())).thenReturn(
                BatchResult.builder().status(BatchStatus.SUCCESS).build()
        );

        JobInstance.Id instanceKey = JobInstance.Id.of("someTestFlowLaunchId", "testJobId", 1);
        JobInstance instance = JobInstance.builder()
                .id(instanceKey)
                .sandboxTaskId(taskId)
                .status(JobStatus.CREATED)
                .build();
        when(jobInstanceTable.get(instanceKey)).thenReturn(instance);

        sandboxTaskExecutor.waitTask(
                sandboxTaskService,
                instance,
                List.of(),
                false,
                false
        );

        Mockito.verify(sandboxClient).startTask(eq(taskId), anyString());
    }

    @SuppressWarnings("unchecked")
    @Test
    void handleStoppedAsFailure() throws Exception {
        when(sandboxClient.getTaskStatus(taskId)).thenReturn(
                SandboxTestData.taskStatus(SandboxTaskStatus.DRAFT),
                SandboxTestData.taskStatus(SandboxTaskStatus.STOPPING),
                SandboxTestData.taskStatus(SandboxTaskStatus.STOPPED),
                SandboxTestData.taskStatus(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskStatus(SandboxTaskStatus.FINISHING),
                SandboxTestData.taskStatus(SandboxTaskStatus.SUCCESS)
        );

        when(sandboxClient.getTask(taskId)).thenReturn(
                SandboxTestData.taskOutput(SandboxTaskStatus.STOPPED),
                SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS)
        );
        when(sandboxClient.startTask(eq(taskId), anyString())).thenReturn(
                BatchResult.builder().status(BatchStatus.SUCCESS).build()
        );

        JobInstance.Id instanceKey = JobInstance.Id.of("someTestFlowLaunchId", "testJobId", 1);
        JobInstance instance = JobInstance.builder()
                .id(instanceKey)
                .sandboxTaskId(taskId)
                .status(JobStatus.CREATED)
                .build();
        when(jobInstanceTable.get(instanceKey)).thenReturn(instance);

        instance = sandboxTaskExecutor.waitTask(
                sandboxTaskService,
                instance,
                List.of(),
                false,
                false
        );

        Mockito.verify(sandboxClient).startTask(eq(taskId), anyString());
        assertThat(instance.getStatus()).isEqualTo(JobStatus.FAILED);
    }

    @SuppressWarnings("unchecked")
    @Test
    void handleStoppedAsRunning() throws Exception {
        when(sandboxClient.getTaskStatus(taskId)).thenReturn(
                SandboxTestData.taskStatus(SandboxTaskStatus.DRAFT),
                SandboxTestData.taskStatus(SandboxTaskStatus.STOPPING),
                SandboxTestData.taskStatus(SandboxTaskStatus.STOPPED),
                SandboxTestData.taskStatus(SandboxTaskStatus.EXECUTING),
                SandboxTestData.taskStatus(SandboxTaskStatus.FINISHING),
                SandboxTestData.taskStatus(SandboxTaskStatus.SUCCESS)
        );

        when(sandboxClient.getTask(taskId)).thenReturn(
                SandboxTestData.taskOutput(SandboxTaskStatus.SUCCESS)
        );
        when(sandboxClient.startTask(eq(taskId), anyString())).thenReturn(
                BatchResult.builder().status(BatchStatus.SUCCESS).build()
        );

        JobInstance.Id instanceKey = JobInstance.Id.of("someTestFlowLaunchId", "testJobId", 1);
        JobInstance instance = JobInstance.builder()
                .id(instanceKey)
                .sandboxTaskId(taskId)
                .status(JobStatus.CREATED)
                .build();
        when(jobInstanceTable.get(instanceKey)).thenReturn(instance);

        instance = sandboxTaskExecutor.waitTask(
                sandboxTaskService,
                instance,
                List.of(),
                false,
                true
        );

        Mockito.verify(sandboxClient).startTask(eq(taskId), anyString());
        assertThat(instance.getStatus()).isEqualTo(JobStatus.SUCCESS);
    }

    private JobInstance createJobInstance(JobInstance.Id instanceKey,
                                          JobStatus status,
                                          @Nullable SandboxTaskStatus sandboxStatus) {
        return JobInstance.builder()
                .id(instanceKey)
                .sandboxTaskId(taskId)
                .status(status)
                .sandboxTaskStatus(sandboxStatus)
                .build();
    }

    private List<TaskBadge> createTaskBadges(int number) {
        List<TaskBadge> badges = new ArrayList<>();
        for (int i = 0; i < number; ++i) {
            badges.add(TaskBadge.of(
                    "report_" + i, "SANDBOX", SANDBOX_URL + "/report_" + i, TaskBadge.TaskStatus.RUNNING
            ));
        }

        return badges;
    }
}
