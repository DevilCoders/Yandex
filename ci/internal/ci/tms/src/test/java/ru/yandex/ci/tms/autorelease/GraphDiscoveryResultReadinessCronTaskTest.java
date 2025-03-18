package ru.yandex.ci.tms.autorelease;

import java.time.Duration;
import java.time.Instant;
import java.util.Set;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryMockConfig;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryRestartTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryResultProcessorTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_FAILED;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_SUCCESS;

@ContextConfiguration(classes = GraphDiscoveryMockConfig.class)
class GraphDiscoveryResultReadinessCronTaskTest extends CommonYdbTestBase {

    @Autowired
    private BazingaTaskManager bazingaTaskManager;
    @Autowired
    private GraphDiscoveryService graphDiscoveryService;
    @Autowired
    AutocheckBlacklistService autocheckBlacklistService;

    private GraphDiscoveryResultReadinessCronTask graphDiscoveryResultReadinessCronTask;
    private OverridableClock clock;

    @BeforeEach
    public void setUp() {
        clock = new OverridableClock();
        graphDiscoveryResultReadinessCronTask = new GraphDiscoveryResultReadinessCronTask(
                Duration.ofSeconds(10),
                Duration.ofMinutes(1),
                db,
                graphDiscoveryService,
                bazingaTaskManager,
                clock,
                null
        );
    }


    @Test
    void doExecute_whenGraphIsNotEvaluatedYet() {
        graphDiscoveryResultReadinessCronTask.doExecute();

        verifyNoInteractions(bazingaTaskManager);
        verifyNoGraphDiscoveryTaskCreated();
    }

    @Test
    void doExecute_whenGraphEvaluated() {
        var taskWithReadyGraph = GraphDiscoveryTask.graphEvaluationStarted(
                        ArcRevision.of("right-revision"), ArcRevision.of("left-revision"), 10, 9, 100500,
                        Set.of(
                                GraphDiscoveryTask.Platform.LINUX, GraphDiscoveryTask.Platform.MANDATORY
                        ),
                        SandboxTaskStatus.EXECUTING.name(),
                        Instant.parse("2020-01-02T10:00:00.000Z")
                ).toBuilder()
                .status(GRAPH_EVALUATION_SUCCESS)
                .graphEvaluationFinishedAt(Instant.parse("2021-01-02T10:00:00.000Z"))
                .sandboxResultResourceId(200500L)
                .build();
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(taskWithReadyGraph));

        clock.setTime(Instant.parse("2022-01-02T10:00:00.000Z"));

        graphDiscoveryResultReadinessCronTask.doExecute();

        verify(bazingaTaskManager).schedule(argThat(
                it -> {
                    if (it instanceof GraphDiscoveryResultProcessorTask) {
                        var params = (GraphDiscoveryResultProcessorTask.Params) it.getParameters();
                        return params.getSandboxTaskId() == 100500;
                    }
                    return false;
                }
        ));

        verifyGraphDiscoveryTaskCreated(
                taskWithReadyGraph.toBuilder()
                        .status(GraphDiscoveryTask.Status.GRAPH_RESULTS_PROCESSING)
                        .discoveryStartedAt(Instant.parse("2022-01-02T10:00:00.000Z"))
                        .build()
        );
    }

    @Test
    void doExecute_whenFailedSandboxTaskIsReadyForRestart() {
        var taskWithReadyGraph = GraphDiscoveryTask.graphEvaluationStarted(
                        ArcRevision.of("right-revision"), ArcRevision.of("left-revision"), 10, 9, 100500,
                        Set.of(
                                GraphDiscoveryTask.Platform.LINUX, GraphDiscoveryTask.Platform.MANDATORY
                        ),
                        SandboxTaskStatus.EXCEPTION.name(),
                        Instant.parse("2020-01-02T10:00:00.000Z")
                ).toBuilder()
                .status(GRAPH_EVALUATION_FAILED)
                .sandboxResultResourceId(200500L)
                .build();
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(taskWithReadyGraph));

        clock.setTime(Instant.parse("2022-01-02T10:00:00.000Z"));

        graphDiscoveryResultReadinessCronTask.doExecute();

        verify(bazingaTaskManager).schedule(argThat(
                it -> {
                    if (it instanceof GraphDiscoveryRestartTask) {
                        var params = (GraphDiscoveryRestartTask.Params) it.getParameters();
                        return params.getSandboxTaskId() == 100500;
                    }
                    return false;
                }
        ));

        verifyGraphDiscoveryTaskCreated(
                taskWithReadyGraph.toBuilder()
                        .status(GraphDiscoveryTask.Status.GRAPH_EVALUATION_STARTED)
                        .nextAttemptAfter(Instant.parse("2022-01-02T10:00:00Z"))
                        .build()
        );
    }

    private void verifyNoGraphDiscoveryTaskCreated() {
        db.currentOrReadOnly(() ->
                assertThat(db.graphDiscoveryTaskTable().findAll()).isEmpty()
        );
    }

    private void verifyGraphDiscoveryTaskCreated(GraphDiscoveryTask task) {
        db.currentOrReadOnly(() ->
                assertThat(db.graphDiscoveryTaskTable().get(task.getId()))
                        .isEqualTo(task)
        );
    }

}
