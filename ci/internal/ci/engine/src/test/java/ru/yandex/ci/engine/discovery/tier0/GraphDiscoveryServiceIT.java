package ru.yandex.ci.engine.discovery.tier0;

import java.time.Instant;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.stubbing.Answer;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.BatchStatus;
import ru.yandex.ci.client.sandbox.api.SandboxCustomField;
import ru.yandex.ci.client.sandbox.api.SandboxTask;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority;
import ru.yandex.ci.client.sandbox.api.SandboxTaskRequirements;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.TaskExecution;
import ru.yandex.ci.client.sandbox.api.TaskSemaphoreAcquire;
import ru.yandex.ci.client.sandbox.api.TaskSemaphores;
import ru.yandex.ci.client.sandbox.api.notification.NotificationSetting;
import ru.yandex.ci.client.sandbox.api.notification.NotificationStatus;
import ru.yandex.ci.client.sandbox.api.notification.NotificationTransport;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_FAILED;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_STARTED;

@ContextConfiguration(classes = GraphDiscoveryMockConfig.class)
@SuppressWarnings("MethodName")
class GraphDiscoveryServiceIT extends YdbCiTestBase {

    @Autowired
    GraphDiscoveryService graphDiscoveryService;

    @Autowired
    OverridableClock clock;

    @Autowired
    SandboxClient sandboxClient;

    @Autowired
    AutocheckBlacklistService autocheckBlacklistService;

    @Autowired
    BazingaTaskManager bazingaTaskManager;

    @BeforeEach
    void setUp() {
        reset(sandboxClient, bazingaTaskManager);
    }

    @Test
    void scheduleGraphDiscovery() {
        graphDiscoveryService.scheduleGraphDiscovery(TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_R4);

        verifyCommitMarkedAsDiscovered(TestData.TRUNK_R4.getCommitId(), false);
        verify(bazingaTaskManager, atLeastOnce()).schedule(any(GraphDiscoveryStartTask.class));
    }

    @Test
    void scheduleGraphDiscovery_whenCommitIsBlacklisted() {
        graphDiscoveryService.scheduleGraphDiscovery(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3,
                TestData.TRUNK_R3);

        verifyCommitMarkedAsDiscovered(TestData.TRUNK_R3.getCommitId(), true);
        verifyNoInteractions(bazingaTaskManager);
    }

    @Test
    void startGraphDiscoverySandboxTask() {
        GraphDiscoveryTask linuxAndMandatoryPlatforms = GraphDiscoveryTask.graphEvaluationStarted(
                ArcRevision.of("right-revision"), ArcRevision.of("left-revision"), 10, 9, 100500L,
                Set.of(
                        GraphDiscoveryTask.Platform.LINUX, GraphDiscoveryTask.Platform.MANDATORY
                ),
                SandboxTaskStatus.DRAFT.name(),
                Instant.parse("2020-01-02T10:00:00.000Z")
        );
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(linuxAndMandatoryPlatforms));

        clock.setTime(Instant.parse("2021-01-02T10:00:00.000Z"));
        doReturn(new SandboxTaskOutput(
                "GRAPH_DISCOVERY_FAKE_TASK", 100501L,
                SandboxTaskStatus.DRAFT, Map.of(), Map.of(), List.of()
        )).when(sandboxClient).createTask(any(SandboxTask.class));

        doReturn(List.of(
                new BatchResult(100500L, null, BatchStatus.SUCCESS),
                new BatchResult(100501L, null, BatchStatus.SUCCESS)
        )).when(sandboxClient).startTasks(any(), anyString());

        doReturn(Map.of(
                100500L, SandboxTaskOutput.builder().id(100500L).status(SandboxTaskStatus.ASSIGNED).build(),
                100501L, SandboxTaskOutput.builder().id(100501L).status(SandboxTaskStatus.EXECUTING).build()
        )).when(sandboxClient).getTasks(eq(Set.of(100500L, 100501L)), any());

        graphDiscoveryService.startGraphDiscoverySandboxTask(
                ArcRevision.of("right-revision"), ArcRevision.of("left-revision"), 10, 9,
                Set.of(
                        GraphDiscoveryTask.Platform.LINUX, GraphDiscoveryTask.Platform.MANDATORY,
                        GraphDiscoveryTask.Platform.SANITIZERS
                )
        );
        verify(sandboxClient).createTask(argThat(arg -> {
            var customFields = arg.getCustomFields();
            return arg.getPriority().getClazz() == SandboxTaskPriority.PriorityClass.SERVICE &&
                    arg.getPriority().getSubclass() == SandboxTaskPriority.PrioritySubclass.HIGH &&
                    arg.getRequirements().getSemaphores().getAcquires()
                            .stream()
                            .map(TaskSemaphoreAcquire::getName)
                            .collect(Collectors.toList())
                            .equals(List.of("CI_GRAPH_DISCOVERY_SANITIZERS.distbuild-testing")) &&
                    hasCustomField(customFields, "build_profile", "pre_commit") &&
                    hasCustomField(customFields, "targets", "autocheck") &&
                    hasCustomField(customFields, "arc_url_right", "arcadia-arc:/#right-revision") &&
                    hasCustomField(customFields, "arc_url_left", "arcadia-arc:/#left-revision") &&
                    hasCustomField(customFields, "autocheck_yaml_url", "arcadia-arc:/#right-revision") &&
                    hasCustomField(customFields, "distbuild_pool", "//sas_gg/autocheck/precommits/public") &&
                    hasCustomField(customFields, "ya_token", "secretWithYaToolToken") &&
                    hasCustomField(customFields, "compress_results", true) &&
                    hasCustomField(customFields, "platforms", List.of(
                            ChangesDetectorSandboxTaskBuilder.platform(GraphDiscoveryTask.Platform.SANITIZERS)
                    ));
        }));
        verify(sandboxClient).startTasks(eq(Set.of(100500L, 100501L)), anyString());

        db.currentOrReadOnly(() -> {
            assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(100500))
                    .get()
                    .usingRecursiveComparison()
                    .isEqualTo(
                            GraphDiscoveryTask.builder()
                                    .id(new GraphDiscoveryTask.Id(
                                            "right-revision",
                                            new LinkedHashSet<>(List.of(
                                                    GraphDiscoveryTask.Platform.LINUX,
                                                    GraphDiscoveryTask.Platform.MANDATORY
                                            ))
                                    ))
                                    .leftCommitId("left-revision")
                                    .rightSvnRevision(10)
                                    .leftSvnRevision(9)
                                    .sandboxTaskId(100500L)
                                    .sandboxStatus("ASSIGNED")
                                    .status(GRAPH_EVALUATION_STARTED)
                                    .graphEvaluationStartedAt(Instant.parse("2020-01-02T10:00:00.000Z"))
                                    .attemptNumber(1)
                                    .failedSandboxTaskIds(Set.of())
                                    .build()
                    );
            assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(100501))
                    .get()
                    .usingRecursiveComparison()
                    .isEqualTo(
                            GraphDiscoveryTask.builder()
                                    .id(new GraphDiscoveryTask.Id(
                                            "right-revision", Set.of(GraphDiscoveryTask.Platform.SANITIZERS)
                                    ))
                                    .leftCommitId("left-revision")
                                    .rightSvnRevision(10)
                                    .leftSvnRevision(9)
                                    .sandboxTaskId(100501L)
                                    .sandboxStatus("EXECUTING")
                                    .status(GRAPH_EVALUATION_STARTED)
                                    .graphEvaluationStartedAt(Instant.parse("2021-01-02T10:00:00.000Z"))
                                    .attemptNumber(1)
                                    .failedSandboxTaskIds(Set.of())
                                    .build()
                    );
        });
    }

    @Test
    void updateStateOfRunningSandboxTasksWhenNoRunningTasks() {
        graphDiscoveryService.updateStateOfRunningSandboxTasks();
        verifyNoInteractions(sandboxClient);
    }

    @Test
    void updateStateOfRunningSandboxTasksWhenSandboxTaskIsStillExecuting() {
        var task = GraphDiscoveryTask.graphEvaluationStarted(
                ArcRevision.of("right-revision"), ArcRevision.of("left-revision"), 10, 9, 100500,
                Set.of(GraphDiscoveryTask.Platform.LINUX),
                SandboxTaskStatus.EXECUTING.name(),
                Instant.parse("2020-01-02T10:00:00.000Z")
        );
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(task));

        doAnswer(sandboxAnswer(Map.of(
                100500L,
                SandboxTaskOutput.builder()
                        .id(100500L)
                        .status(SandboxTaskStatus.EXECUTING)
                        .build()
        ))).when(sandboxClient).getTasks(any(), any(), any());

        graphDiscoveryService.updateStateOfRunningSandboxTasks();

        db.currentOrReadOnly(() ->
                assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(100500))
                        .get().isEqualTo(task)
        );
    }

    @Test
    void updateStateOfRunningSandboxTasksWhenSandboxTaskFinished() {
        var task = GraphDiscoveryTask.graphEvaluationStarted(
                ArcRevision.of("right-revision"), ArcRevision.of("left-revision"), 10, 9, 100500,
                Set.of(GraphDiscoveryTask.Platform.LINUX),
                SandboxTaskStatus.EXECUTING.name(),
                Instant.parse("2020-01-02T10:00:00.000Z")
        );
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(task));

        doAnswer(sandboxAnswer(Map.of(
                100500L,
                SandboxTaskOutput.builder()
                        .id(100500L)
                        .status(SandboxTaskStatus.SUCCESS)
                        .execution(TaskExecution.builder()
                                .finished(Instant.parse("2020-01-02T10:00:00.000Z"))
                                .build()
                        )
                        .build()
        ))).when(sandboxClient).getTasks(any(), any(), any());

        clock.setTime(Instant.parse("2021-01-02T10:00:00.000Z"));
        graphDiscoveryService.updateStateOfRunningSandboxTasks();

        db.currentOrReadOnly(() ->
                assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(100500))
                        .get().isEqualTo(
                                task.toBuilder()
                                        .status(GraphDiscoveryTask.Status.GRAPH_EVALUATION_SUCCESS)
                                        .sandboxStatus("SUCCESS")
                                        .graphEvaluationFinishedAt(Instant.parse("2020-01-02T10:00:00.000Z"))
                                        .nextAttemptAfter(Instant.parse("2021-01-02T10:00:30Z"))
                                        .build()
                        )
        );
    }

    @Test
    void updateStateOfRunningSandboxTasksWhenSandboxTaskFailed() {
        var task = GraphDiscoveryTask.graphEvaluationStarted(
                ArcRevision.of("right-revision"), ArcRevision.of("left-revision"), 10, 9, 100500,
                Set.of(GraphDiscoveryTask.Platform.LINUX),
                SandboxTaskStatus.EXECUTING.name(),
                Instant.parse("2020-01-02T10:00:00.000Z")
        );
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(task));

        doAnswer(sandboxAnswer(Map.of(
                100500L,
                SandboxTaskOutput.builder()
                        .id(100500L)
                        .status(SandboxTaskStatus.FAILURE)
                        .execution(TaskExecution.builder()
                                .finished(Instant.parse("2020-01-02T10:00:00.000Z"))
                                .build()
                        )
                        .build()
        ))).when(sandboxClient).getTasks(any(), any(), any());

        clock.setTime(Instant.parse("2021-01-02T10:00:00.000Z"));
        graphDiscoveryService.updateStateOfRunningSandboxTasks();

        db.currentOrReadOnly(() ->
                assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(100500))
                        .get().isEqualTo(
                                task.toBuilder()
                                        .status(GRAPH_EVALUATION_FAILED)
                                        .sandboxStatus("FAILURE")
                                        .nextAttemptAfter(Instant.parse("2021-01-02T10:00:30Z"))
                                        .build()
                        )
        );
    }

    @Test
    void updateStateOfFailedSandboxTasksWhenNoFailedTasks() {
        graphDiscoveryService.updateStateOfFailedSandboxTasks();
        verifyNoInteractions(sandboxClient);
    }

    @Test
    void updateStateOfFailedSandboxTasksWhenSandboxTaskRunning() {
        var task = graphDiscoveryTask(100500, SandboxTaskStatus.EXCEPTION, GRAPH_EVALUATION_FAILED);
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(task));

        doAnswer(sandboxAnswer(Map.of(
                100500L,
                SandboxTaskOutput.builder()
                        .id(100500L)
                        .status(SandboxTaskStatus.EXECUTING)
                        .build()
        ))).when(sandboxClient).getTasks(any(), any(), any());

        graphDiscoveryService.updateStateOfFailedSandboxTasks();

        db.currentOrReadOnly(() ->
                assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(100500))
                        .get().isEqualTo(
                                task.toBuilder()
                                        .status(GRAPH_EVALUATION_STARTED)
                                        .sandboxStatus(SandboxTaskStatus.EXECUTING.name())
                                        .build()
                        )
        );
    }

    @Test
    void updateStateOfFailedSandboxTasksWhenSandboxTaskFailed() {
        var task = graphDiscoveryTask(100500, SandboxTaskStatus.EXCEPTION, GRAPH_EVALUATION_FAILED);
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(task));

        doAnswer(sandboxAnswer(Map.of(
                100500L,
                SandboxTaskOutput.builder()
                        .id(100500L)
                        .status(SandboxTaskStatus.FAILURE)
                        .execution(TaskExecution.builder()
                                .finished(Instant.parse("2020-01-02T10:00:00.000Z"))
                                .build()
                        )
                        .build()
        ))).when(sandboxClient).getTasks(any(), any(), any());

        clock.setTime(Instant.parse("2021-01-02T10:00:00.000Z"));
        graphDiscoveryService.updateStateOfFailedSandboxTasks();

        db.currentOrReadOnly(() ->
                assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(100500))
                        .get().isEqualTo(
                                task.toBuilder()
                                        .status(GRAPH_EVALUATION_FAILED)
                                        .build()
                        )
        );
    }

    @Test
    void restartGraphDiscoverySandboxTask() {
        var taskWithFailureStatus = graphDiscoveryTask("right-revision", 100_500L, SandboxTaskStatus.FAILURE,
                GRAPH_EVALUATION_FAILED);
        var taskWithTimeoutStatus = graphDiscoveryTask("right-revision-r2", 100_501L, SandboxTaskStatus.TIMEOUT,
                GRAPH_EVALUATION_FAILED);

        db.currentOrTx(() -> {
            db.graphDiscoveryTaskTable().save(taskWithFailureStatus);
            db.graphDiscoveryTaskTable().save(taskWithTimeoutStatus);
        });

        clock.setTime(Instant.parse("2021-01-02T10:00:00.000Z"));

        // mock sandbox responses for 100_501L task
        doReturn(List.of(
                new BatchResult(100_501L, null, BatchStatus.SUCCESS)
        )).when(sandboxClient).startTasks(eq(Set.of(100_501L)), anyString());
        doReturn(Map.of(
                100_501L, SandboxTaskOutput.builder().id(100_501L).status(SandboxTaskStatus.EXECUTING).build()
        )).when(sandboxClient).getTasks(eq(Set.of(100_501L)), any());

        // mock sandbox responses for 200_000L task
        doReturn(new SandboxTaskOutput(
                "GRAPH_DISCOVERY_FAKE_TASK", 200_000L,
                SandboxTaskStatus.DRAFT, Map.of(), Map.of(), List.of()
        )).when(sandboxClient).createTask(any(SandboxTask.class));
        doReturn(List.of(
                new BatchResult(200_000L, null, BatchStatus.SUCCESS)
        )).when(sandboxClient).startTasks(eq(Set.of(200_000L)), anyString());
        doReturn(Map.of(
                200_000L, SandboxTaskOutput.builder().id(200_000L).status(SandboxTaskStatus.ASSIGNED).build()
        )).when(sandboxClient).getTasks(eq(Set.of(200_000L)), any());

        // restartGraphDiscoverySandboxTask
        graphDiscoveryService.restartGraphDiscoverySandboxTask(List.of(
                taskWithFailureStatus,
                taskWithTimeoutStatus
        ));

        var createTaskArgumentCaptor = ArgumentCaptor.forClass(SandboxTask.class);
        verify(sandboxClient).createTask(createTaskArgumentCaptor.capture());
        assertThat(createTaskArgumentCaptor.getValue())
                .usingRecursiveComparison()
                .isEqualTo(sandboxTask());

        verify(sandboxClient).startTasks(eq(Set.of(100_501L)), anyString());
        verify(sandboxClient).startTasks(eq(Set.of(200_000L)), anyString());

        db.currentOrReadOnly(() -> {
            assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(100_500L)).isEmpty();
            assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(100_501L)).get()
                    .isEqualTo(taskWithTimeoutStatus.toBuilder()
                            .status(GraphDiscoveryTask.Status.GRAPH_EVALUATION_STARTED)
                            .sandboxStatus("EXECUTING")
                            .attemptNumber(2)
                            .build()
                    );
            assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(200_000L)).get()
                    .isEqualTo(taskWithFailureStatus.toBuilder()
                            .sandboxTaskId(200_000L)
                            .failedSandboxTaskId(100_500L)
                            .status(GraphDiscoveryTask.Status.GRAPH_EVALUATION_STARTED)
                            .sandboxStatus("ASSIGNED")
                            .attemptNumber(2)
                            .build()
                    );
        });
    }

    private static GraphDiscoveryTask graphDiscoveryTask(
            long sandboxTaskId,
            SandboxTaskStatus sandboxTaskStatus,
            GraphDiscoveryTask.Status taskStatus
    ) {
        return graphDiscoveryTask("right-revision", sandboxTaskId, sandboxTaskStatus, taskStatus);
    }

    private static GraphDiscoveryTask graphDiscoveryTask(
            String commitId,
            long sandboxTaskId,
            SandboxTaskStatus sandboxTaskStatus,
            GraphDiscoveryTask.Status taskStatus
    ) {
        return GraphDiscoveryTask.graphEvaluationStarted(
                ArcRevision.of(commitId), ArcRevision.of("left-revision"), 10, 9, sandboxTaskId,
                Set.of(GraphDiscoveryTask.Platform.LINUX),
                sandboxTaskStatus.name(),
                Instant.parse("2020-01-02T10:00:00.000Z")
        ).withStatus(taskStatus);
    }

    private static SandboxTask sandboxTask() {
        return SandboxTask.builder()
                .type("GRAPH_DISCOVERY_FAKE_TASK")
                .owner("sandboxTaskOwner")
                .priority(new SandboxTaskPriority(
                        SandboxTaskPriority.PriorityClass.SERVICE,
                        SandboxTaskPriority.PrioritySubclass.HIGH
                ))
                .requirements(new SandboxTaskRequirements()
                        .setSemaphores(new TaskSemaphores()
                                .setAcquires(List.of(
                                        new TaskSemaphoreAcquire()
                                                .setName("CI_GRAPH_DISCOVERY_LINUX_AND_MANDATORY.distbuild-testing")
                                                .setWeight(1L)
                                ))
                        )
                )
                .customFields(List.of(
                        new SandboxCustomField("build_profile", "pre_commit"),
                        new SandboxCustomField("targets", "autocheck"),
                        new SandboxCustomField("arc_url_right", "arcadia-arc:/#right-revision"),
                        new SandboxCustomField("arc_url_left", "arcadia-arc:/#left-revision"),
                        new SandboxCustomField("autocheck_yaml_url", "arcadia-arc:/#right-revision"),
                        new SandboxCustomField("platforms", List.of("linux")),
                        new SandboxCustomField("distbuild_pool", "//sas_gg/autocheck/precommits/public"),
                        new SandboxCustomField("ya_token", "secretWithYaToolToken"),
                        new SandboxCustomField("compress_results", true)
                ))
                .tags(List.of("CI", "GRAPH_DISCOVERY", "LINUX"))
                .description("right: 10 (right-revision), left: 9 (left-revision), [LINUX]")
                .notifications(List.of(
                        new NotificationSetting(
                                NotificationTransport.EMAIL,
                                List.of(
                                        NotificationStatus.EXCEPTION,
                                        NotificationStatus.TIMEOUT
                                ),
                                // v-korovin is author of Sandbox task CHANGES_DETECTOR
                                List.of("v-korovin")
                        )
                ))
                .build();
    }


    private static boolean hasCustomField(List<SandboxCustomField> fields, String name, Object value) {
        return fields.stream().anyMatch(it -> customFieldEqualTo(it, name, value));
    }

    private static boolean customFieldEqualTo(SandboxCustomField field, String name, Object value) {
        return name.equals(field.getName()) && field.getValue().equals(value);
    }

    @SuppressWarnings({"unchecked", "rawtypes"})
    private static Answer<Void> sandboxAnswer(Map<Long, SandboxTaskOutput> sandboxTaskOutputMap) {
        return invocation -> {
            ((Consumer) invocation.getArgument(2)).accept(sandboxTaskOutputMap);
            return null;
        };
    }

    private void verifyCommitMarkedAsDiscovered(String commitId, boolean discovered) {
        db.currentOrReadOnly(() -> {
            assertThat(
                    db.commitDiscoveryProgress().find(commitId)
                            .map(CommitDiscoveryProgress::isGraphDiscoveryFinished)
                            .orElse(false)
            ).isEqualTo(discovered);
        });
    }
}
