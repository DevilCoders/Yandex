package ru.yandex.ci.core.discovery;

import java.time.Instant;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.LongStream;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.arc.ArcRevision;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_STARTED;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_SUCCESS;

class GraphDiscoveryTaskTableTest extends CommonYdbTestBase {

    @Test
    void findBySandboxTaskId() {
        int targetSandboxTaskId = 100500;
        db.currentOrTx(() -> assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(targetSandboxTaskId))
                .isEmpty());

        var expectedTask = GraphDiscoveryTask.graphEvaluationStarted(
                ArcRevision.of("right-revision"), ArcRevision.of("left-revision"), 10, 9, targetSandboxTaskId,
                Set.of(
                        GraphDiscoveryTask.Platform.LINUX, GraphDiscoveryTask.Platform.MANDATORY
                ),
                SandboxTaskStatus.EXECUTING.name(),
                Instant.parse("2020-01-02T10:00:00.000Z")
        );
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(expectedTask));
        db.currentOrTx(() -> assertThat(db.graphDiscoveryTaskTable().findBySandboxTaskId(targetSandboxTaskId))
                .get().isEqualTo(expectedTask));
    }

    @Test
    void findByArcRevision() {
        ArcRevision targetArcRevision = ArcRevision.of("right-revision");
        db.currentOrTx(() -> assertThat(db.graphDiscoveryTaskTable().findByArcRevision(targetArcRevision))
                .isEmpty());

        var expectedTask = GraphDiscoveryTask.graphEvaluationStarted(
                targetArcRevision, ArcRevision.of("left-revision"), 10, 9, 100500,
                Set.of(
                        GraphDiscoveryTask.Platform.LINUX, GraphDiscoveryTask.Platform.MANDATORY
                ),
                SandboxTaskStatus.EXECUTING.name(),
                Instant.parse("2020-01-02T10:00:00.000Z")
        );
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(expectedTask));
        db.currentOrTx(() -> assertThat(db.graphDiscoveryTaskTable().findByArcRevision(targetArcRevision))
                .contains(expectedTask));
        var expectedTask2 = GraphDiscoveryTask.graphEvaluationStarted(
                targetArcRevision, ArcRevision.of("left-revision"), 10, 9, 100501,
                Set.of(GraphDiscoveryTask.Platform.SANITIZERS),
                SandboxTaskStatus.EXECUTING.name(),
                Instant.parse("2020-01-02T10:00:00.000Z")
        );
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(expectedTask2));
        db.currentOrTx(() -> assertThat(db.graphDiscoveryTaskTable().findByArcRevision(targetArcRevision))
                .contains(expectedTask, expectedTask2));
    }

    @Test
    void findByStatus() {
        ArcRevision targetArcRevision = ArcRevision.of("right-revision");
        db.currentOrTx(() -> assertThat(
                db.graphDiscoveryTaskTable()
                        .findByStatusAndSandboxTaskIdGreaterThen(GRAPH_EVALUATION_STARTED, -1, 1000)
        ).isEmpty());

        var expectedTask = GraphDiscoveryTask.graphEvaluationStarted(
                targetArcRevision, ArcRevision.of("left-revision"), 10, 9, 100500,
                Set.of(
                        GraphDiscoveryTask.Platform.LINUX, GraphDiscoveryTask.Platform.MANDATORY
                ),
                SandboxTaskStatus.EXECUTING.name(),
                Instant.parse("2020-01-02T10:00:00.000Z")
        );
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(expectedTask));
        db.currentOrTx(() -> assertThat(
                db.graphDiscoveryTaskTable()
                        .findByStatusAndSandboxTaskIdGreaterThen(GRAPH_EVALUATION_STARTED, -1, 1000)
        ).contains(expectedTask));

        GraphDiscoveryTask updatedTask = expectedTask.toBuilder()
                .status(GRAPH_EVALUATION_SUCCESS)
                .graphEvaluationFinishedAt(Instant.parse("2021-01-02T10:00:00.000Z"))
                .build();
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(updatedTask));
        db.currentOrTx(() -> assertThat(
                db.graphDiscoveryTaskTable()
                        .findByStatusAndSandboxTaskIdGreaterThen(GRAPH_EVALUATION_STARTED, -1, 1000)
        ).isEmpty());
        db.currentOrTx(() -> assertThat(
                db.graphDiscoveryTaskTable()
                        .findByStatusAndSandboxTaskIdGreaterThen(GRAPH_EVALUATION_SUCCESS, -1, 1000)
        ).contains(updatedTask));
    }

    @Test
    void findByStatusAndSandboxTaskIdGreaterThen() {
        db.currentOrTx(() ->
                LongStream.range(1, 10).forEach(sandboxTaskId ->
                        db.graphDiscoveryTaskTable().save(
                                GraphDiscoveryTask.graphEvaluationStarted(
                                        ArcRevision.of("right-revision" + sandboxTaskId),
                                        ArcRevision.of("left-revision"),
                                        10, 9, sandboxTaskId,
                                        Set.of(GraphDiscoveryTask.Platform.LINUX),
                                        SandboxTaskStatus.EXECUTING.name(),
                                        Instant.parse("2020-01-02T10:00:00.000Z")
                                )
                        )
                ));

        db.currentOrTx(() -> {
            assertThat(
                    db.graphDiscoveryTaskTable()
                            .findByStatusAndSandboxTaskIdGreaterThen(GRAPH_EVALUATION_STARTED, -1, 1000)
            ).extracting(GraphDiscoveryTask::getSandboxTaskId)
                    .containsExactly(1L, 2L, 3L, 4L, 5L, 6L, 7L, 8L, 9L);
            assertThat(
                    db.graphDiscoveryTaskTable()
                            .findByStatusAndSandboxTaskIdGreaterThen(GRAPH_EVALUATION_STARTED, 7, 1)
            ).extracting(GraphDiscoveryTask::getSandboxTaskId)
                    .containsExactly(8L);
            assertThat(
                    db.graphDiscoveryTaskTable()
                            .findByStatusAndSandboxTaskIdGreaterThen(GRAPH_EVALUATION_STARTED, 1000, 1000)
            ).isEmpty();
        });
    }

    @Test
    void testPlatformJsonOrderInIdField() {
        db.currentOrTx(() -> {
            db.graphDiscoveryTaskTable().save(
                    GraphDiscoveryTask.graphEvaluationStarted(
                            ArcRevision.of("commit-hash-1"), ArcRevision.of("left-revision"), 10, 9, 100500,
                            new LinkedHashSet<>(List.of(
                                    GraphDiscoveryTask.Platform.LINUX,
                                    GraphDiscoveryTask.Platform.MANDATORY
                            )),
                            SandboxTaskStatus.EXECUTING.name(),
                            Instant.parse("2020-01-02T10:00:00.000Z")
                    )
            );
            db.graphDiscoveryTaskTable().save(
                    GraphDiscoveryTask.graphEvaluationStarted(
                            ArcRevision.of("commit-hash-1"), ArcRevision.of("left-revision"), 10, 9, 100500,
                            new LinkedHashSet<>(List.of(
                                    GraphDiscoveryTask.Platform.MANDATORY,
                                    GraphDiscoveryTask.Platform.LINUX
                            )),
                            SandboxTaskStatus.EXECUTING.name(),
                            Instant.parse("2020-01-02T10:00:00.000Z")
                    )
            );
        });

        db.currentOrTx(() ->
                assertThat(db.graphDiscoveryTaskTable().findAll()).hasSize(1)
        );
    }
}
