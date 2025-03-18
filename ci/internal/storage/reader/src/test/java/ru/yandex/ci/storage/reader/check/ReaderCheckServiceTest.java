package ru.yandex.ci.storage.reader.check;

import java.time.Instant;
import java.util.List;
import java.util.Set;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationStatistics;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.ExpectedTask;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.reader.StorageReaderYdbTestBase;
import ru.yandex.ci.storage.reader.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.message.shard.ShardOutStreamStatistics;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriter;
import ru.yandex.ci.storage.reader.other.MetricAggregationService;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

public class ReaderCheckServiceTest extends StorageReaderYdbTestBase {
    private static final Instant TIMESTAMP = Instant.ofEpochSecond(1602601048L);

    private ReaderCheckService service;

    @Autowired
    private ShardInMessageWriter shardInMessageWriter;

    private CheckFinalizationService finalizationService;

    @BeforeEach
    public void setup() {
        var readerStatistics = new ReaderStatistics(
                mock(MainStreamStatistics.class),
                mock(ShardOutStreamStatistics.class),
                mock(EventsStreamStatistics.class),
                meterRegistry
        );

        var requirementsService = mock(RequirementsService.class);
        var analysisService = new CheckAnalysisService(List.of());
        this.service = new ReaderCheckService(
                requirementsService, readerCache, readerStatistics, db, badgeEventsProducer,
                mock(MetricAggregationService.class)
        );

        this.finalizationService = new CheckFinalizationService(
                readerCache, readerStatistics, db,
                List.of(),
                shardInMessageWriter,
                mock(BazingaTaskManager.class),
                analysisService,
                service
        );
    }

    @Test
    void finishesPartition() {
        var check = register(1L, "base", "merge");
        var iteration = registerIteration(check.getId(), IterationType.FAST, 1);
        var task = registerTask(CheckStatus.CREATED, iteration.getId(), "task-1", 3, false);
        var taskTwo = registerTask(CheckStatus.CREATED, iteration.getId(), "task-2", 1, false);

        this.db.currentOrTx(
                () -> this.db.checkIterations().save(
                        iteration.toBuilder()
                                .expectedTasks(
                                        Set.of(
                                                new ExpectedTask("task-1", false),
                                                new ExpectedTask("task-2", false)
                                        )
                                )
                                .registeredExpectedTasks(
                                        Set.of(
                                                new ExpectedTask("task-1", false),
                                                new ExpectedTask("task-2", false)
                                        )
                                )
                                .numberOfTasks(2)
                                .build()
                )
        );

        var taskId = task.getId();
        readerCache.modifyWithDbTx(cache -> finalizationService.processPartitionFinishedInTx(cache, taskId, 0));

        this.db.readOnly().run(
                () -> assertThat(this.db.checkTasks().get(taskId).getStatus()).isEqualTo(CheckStatus.CREATED)
        );

        readerCache.modifyWithDbTx(cache -> finalizationService.processPartitionFinishedInTx(cache, taskId, 2));
        this.db.readOnly().run(
                () -> assertThat(this.db.checkTasks().get(taskId).getStatus()).isEqualTo(CheckStatus.CREATED)
        );
        readerCache.modifyWithDbTx(cache -> finalizationService.processPartitionFinishedInTx(cache, taskId, 1));
        this.db.readOnly().run(
                () -> assertThat(this.db.checkTasks().get(taskId).getStatus())
                        .isEqualTo(CheckStatus.COMPLETED_SUCCESS)
        );

        this.db.readOnly().run(
                () -> {
                    var entity = this.db.checkIterations().get(iteration.getId());
                    assertThat(entity.getStatus()).isEqualTo(CheckStatus.RUNNING);
                }
        );

        this.db.readOnly().run(
                () -> assertThat(this.db.checkTasks().get(taskId).getStatus()).isEqualTo(CheckStatus.COMPLETED_SUCCESS)
        );

        readerCache.modifyWithDbTx(cache -> finalizationService.processPartitionFinishedInTx(cache, taskTwo.getId(),
                0));

        this.db.readOnly().run(
                () -> assertThat(this.db.checkTasks().get(taskTwo.getId()).getStatus())
                        .isEqualTo(CheckStatus.COMPLETED_SUCCESS)
        );

        this.db.readOnly().run(
                () -> {
                    var entity = this.db.checkIterations().get(iteration.getId());
                    assertThat(entity.getStatus()).isEqualTo(CheckStatus.COMPLETING);
                    assertThat(entity.getInfo().getProgress()).isEqualTo(99);
                }
        );

        finalizationService.finishIteration(iteration.getId());

        this.db.readOnly().run(
                () -> {
                    var entity = this.db.checkIterations().get(iteration.getId());
                    assertThat(entity.getStatus()).isEqualTo(CheckStatus.COMPLETED_SUCCESS);
                    assertThat(entity.getInfo().getProgress()).isEqualTo(100);
                }
        );

        this.db.readOnly()
                .run(
                        () -> assertThat(this.db.checks().get(check.getId()).getStatus())
                                .isEqualTo(CheckStatus.COMPLETED_SUCCESS)
                );
    }

    @Test
    void waitingForAllTasksToRegister() {
        var check = register(1L, "base", "merge");
        var iteration = registerIteration(check.getId(), IterationType.FAST, 1);
        var task = registerTask(CheckStatus.CREATED, iteration.getId(), "task-1", 1, false);

        this.db.currentOrTx(
                () -> this.db.checkIterations().save(
                        iteration.toBuilder()
                                .expectedTasks(
                                        Set.of(
                                                new ExpectedTask("task-1", false),
                                                new ExpectedTask("task-2", false)
                                        )
                                )
                                .registeredExpectedTasks(
                                        Set.of(
                                                new ExpectedTask("task-1", false)
                                        )
                                )
                                .numberOfTasks(1)
                                .build()
                )
        );

        var taskId = task.getId();
        readerCache.modifyWithDbTx(cache -> finalizationService.processPartitionFinishedInTx(cache, taskId, 0));
        this.db.readOnly().run(
                () -> assertThat(this.db.checkTasks().get(taskId).getStatus())
                        .isEqualTo(CheckStatus.COMPLETED_SUCCESS)
        );

        this.db.readOnly().run(
                () -> {
                    var entity = this.db.checkIterations().get(iteration.getId());
                    assertThat(entity.getStatus()).isEqualTo(CheckStatus.RUNNING);
                }
        );
    }

    @Test
    void notFinishedWhenHasOtherIteration() {
        var check = register(1L, "base", "merge");
        var metaIteration = registerIteration(check.getId(), IterationType.FAST, 0);
        var iteration = registerIteration(check.getId(), IterationType.FAST, 1);
        var fullIteration = registerIteration(check.getId(), IterationType.FULL, 1);
        var iteration2 = registerIteration(check.getId(), IterationType.FAST, 2);

        finalizationService.finishIteration(fullIteration.getId());

        this.db.readOnly().run(
                () -> {
                    var entity = this.db.checkIterations().get(fullIteration.getId());
                    assertThat(entity.getStatus()).isEqualTo(CheckStatus.COMPLETED_SUCCESS);

                    assertThat(this.db.checkIterations().get(metaIteration.getId()).getStatus())
                            .isEqualTo(CheckStatus.CREATED);

                    assertThat(this.db.checks().get(check.getId()).getStatus())
                            .isEqualTo(CheckStatus.RUNNING);
                }
        );

        // Test wrong order of iteration complete, rare case.
        finalizationService.finishIteration(iteration2.getId());

        this.db.readOnly()
                .run(
                        () -> {
                            var entity = this.db.checkIterations().get(iteration2.getId());
                            assertThat(entity.getStatus()).isEqualTo(CheckStatus.COMPLETED_SUCCESS);

                            assertThat(this.db.checkIterations().get(metaIteration.getId()).getStatus())
                                    .isEqualTo(CheckStatus.CREATED);

                            assertThat(this.db.checks().get(check.getId()).getStatus())
                                    .isEqualTo(CheckStatus.RUNNING);
                        }
                );

        finalizationService.finishIteration(iteration.getId());

        this.db.readOnly().run(
                () -> {
                    assertThat(this.db.checkIterations().get(iteration.getId()).getStatus())
                            .isEqualTo(CheckStatus.COMPLETED_SUCCESS);

                    assertThat(this.db.checkIterations().get(metaIteration.getId()).getStatus())
                            .isEqualTo(CheckStatus.COMPLETED_SUCCESS);

                    assertThat(this.db.checks().get(check.getId()).getStatus())
                            .isEqualTo(CheckStatus.COMPLETED_SUCCESS);
                }
        );
    }

    @Test
    void usesMetaIterationStatusOnFinish() {
        var check = register(1L, "base", "merge");
        var metaIteration = registerIteration(check.getId(), IterationType.FAST, 0);
        var iteration = registerIteration(check.getId(), IterationType.FAST, 1);
        var iteration2 = registerIteration(check.getId(), IterationType.FAST, 2);
        var fullIteration = registerIteration(check.getId(), IterationType.FULL, 1);

        this.db.currentOrTx(() -> {
            this.db.checkIterations().save(metaIteration.withStatus(CheckStatus.COMPLETED_SUCCESS));
            this.db.checkIterations().save(iteration.withStatus(CheckStatus.COMPLETED_FAILED));
            this.db.checkIterations().save(iteration2.withStatus(CheckStatus.COMPLETED_SUCCESS));
        });

        finalizationService.finishIteration(fullIteration.getId());

        this.db.readOnly().run(
                () -> {
                    var entity = this.db.checkIterations().get(fullIteration.getId());
                    assertThat(entity.getStatus()).isEqualTo(CheckStatus.COMPLETED_SUCCESS);

                    assertThat(this.db.checks().get(check.getId()).getStatus())
                            .isEqualTo(CheckStatus.COMPLETED_SUCCESS);
                }
        );
    }

    @Test
    void failsWhenOtherIterationFailed() {
        var check = register(1L, "base", "merge");
        var iteration = registerIteration(check.getId(), IterationType.FAST, 1);
        var fullIteration = registerIteration(check.getId(), IterationType.FULL, 1);

        this.db.currentOrTx(() -> {
            this.db.checkIterations().save(fullIteration.withStatus(CheckStatus.COMPLETED_FAILED));
        });

        finalizationService.finishIteration(iteration.getId());

        this.db.readOnly().run(
                () -> {
                    assertThat(this.db.checks().get(check.getId()).getStatus())
                            .isEqualTo(CheckStatus.COMPLETED_FAILED);
                }
        );
    }

    @Test
    void cancelsCheckByTimeout() {
        var check = register(1L, "base", "merge");
        var iteration = this.registerIteration(check.getId(), IterationType.FAST, 1);
        registerTask(CheckStatus.CANCELLING_BY_TIMEOUT, iteration.getId(), "task-1", 2, false);

        this.db.currentOrTx(() -> this.db.checks().save(check.withStatus(CheckStatus.CANCELLING_BY_TIMEOUT)));
        this.db.currentOrTx(
                () -> this.db.checkIterations().save(iteration.withStatus(CheckStatus.CANCELLING_BY_TIMEOUT))
        );

        var checkId = new CheckTaskEntity.Id(iteration.getId(), "task-1");
        finalizationService.processIterationCancelled(iteration.getId());

        this.db.readOnly().run(
                () -> assertThat(this.db.checkTasks().get(checkId).getStatus())
                        .isEqualTo(CheckStatus.CANCELLED_BY_TIMEOUT)
        );
        this.db.readOnly()
                .run(
                        () -> assertThat(this.db.checkIterations().get(iteration.getId()).getStatus())
                                .isEqualTo(CheckStatus.CANCELLED_BY_TIMEOUT)
                );
        this.db.readOnly()
                .run(
                        () -> assertThat(this.db.checks().get(check.getId()).getStatus())
                                .isEqualTo(CheckStatus.CANCELLED_BY_TIMEOUT)
                );
    }

    @Test
    void pessimizeIteration() {
        var check = register(1L, "base", "merge");
        var iteration = registerIteration(check.getId(), IterationType.FAST, 1);

        service.processPessimize(iteration.getId(), "Pessimize reason 1");
        iteration = readerCache.iterations().getFreshOrThrow(
                CheckIterationEntity.Id.of(check.getId(), IterationType.FAST, 1)
        );
        assertThat(iteration.getInfo().isPessimized()).isTrue();
        assertThat(iteration.getInfo().getPessimizationInfo()).isEqualTo("Pessimize reason 1");

        service.processPessimize(iteration.getId(), "Pessimize reason 2");
        iteration = readerCache.iterations().getFreshOrThrow(
                CheckIterationEntity.Id.of(check.getId(), IterationType.FAST, 1)
        );

        // Save only first pessimization Info
        assertThat(iteration.getInfo().isPessimized()).isTrue();
        assertThat(iteration.getInfo().getPessimizationInfo()).isEqualTo("Pessimize reason 1");
    }

    private CheckIterationEntity registerIteration(CheckEntity.Id id, IterationType iterationType, int number) {
        return this.db.currentOrTx(() -> this.db.checkIterations()
                .save(
                        CheckIterationEntity.builder()
                                .id(CheckIterationEntity.Id.of(id, iterationType, number))
                                .statistics(IterationStatistics.EMPTY)
                                .created(Instant.now())
                                .status(CheckStatus.CREATED)
                                .info(IterationInfo.EMPTY)
                                .build()
                )
        );
    }

    private CheckEntity register(long leftRevisionNumber, String leftRevision, String rightRevision) {
        return register(leftRevisionNumber, leftRevision, rightRevision, Set.of());
    }

    private CheckEntity register(long leftRevisionNumber, String leftRevision, String rightRevision, Set<String> tags) {
        return this.db.currentOrTx(() -> this.db.checks().save(
                CheckEntity.builder()
                        .id(CheckEntity.Id.of(1L))
                        .left(new StorageRevision("trunk", leftRevision, leftRevisionNumber, Instant.EPOCH))
                        .right(new StorageRevision("trunk", rightRevision, leftRevisionNumber, Instant.EPOCH))
                        .tags(tags)
                        .type(CheckType.TRUNK_POST_COMMIT)
                        .created(TIMESTAMP)
                        .status(CheckStatus.RUNNING)
                        .build()
        ));
    }

    private CheckTaskEntity registerTask(
            CheckStatus status, CheckIterationEntity.Id id, String taskId, int numberOfPartitions, boolean isRight
    ) {
        return this.db.currentOrTx(() -> this.db.checkTasks().save(CheckTaskEntity.builder()
                .id(new CheckTaskEntity.Id(id, taskId))
                .numberOfPartitions(numberOfPartitions)
                .right(isRight)
                .status(status)
                .jobName(taskId)
                .completedPartitions(Set.of())
                .build()));
    }
}
