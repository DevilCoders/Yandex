package ru.yandex.ci.storage.api.check;

import java.time.Instant;
import java.util.List;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.IntStream;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.mockito.Mockito;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.storage.api.ApiTestBase;
import ru.yandex.ci.storage.api.cache.StorageApiCache;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.check.CreateIterationParams;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGeneratorEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class ApiCheckServiceTest extends ApiTestBase {
    private static final Instant TIMESTAMP = Instant.ofEpochSecond(1602601048L);

    private ApiCheckService service;

    @BeforeEach
    public void setup() {
        var arcService = Mockito.mock(ArcService.class);
        var storageEventsProducer = mock(StorageEventsProducer.class);

        for (var i = 1; i < 5; ++i) {
            when(arcService.getCommit(ArcRevision.of("r" + i)))
                    .thenReturn(ArcCommit.builder().id(ArcCommit.Id.of("r" + i)).svnRevision(i).build());
        }

        var requirementsService = mock(RequirementsService.class);
        this.service = new ApiCheckService(
                new OverridableClock(), requirementsService, db, arcService, "test", apiCache, storageEventsProducer, 1,
                mock(BazingaTaskManager.class), ShardingSettings.DEFAULT
        );
        this.db.currentOrTx(
                () -> this.db.checkIds().save(List.of(
                        new CheckIdGeneratorEntity(new CheckIdGeneratorEntity.Id(1), 1L),
                        new CheckIdGeneratorEntity(new CheckIdGeneratorEntity.Id(2), 2L),
                        new CheckIdGeneratorEntity(new CheckIdGeneratorEntity.Id(3), 3L),
                        new CheckIdGeneratorEntity(new CheckIdGeneratorEntity.Id(4), 4L)
                )));
    }

    @ParameterizedTest
    @MethodSource("checkRunLargeTestsAfterDiscovery")
    void registersCheckAndRunAfterDiscovery(CheckType checkType, boolean expectRunLargeTestsAfterDiscovery) {
        var check = register(1L, "r1", "r2", Set.of(), checkType);
        assertThat(check.getRunLargeTestsAfterDiscovery()).isEqualTo(expectRunLargeTestsAfterDiscovery);
    }

    @Test
    void registersCheckOnceWithSameTag() {
        var check = register(1L, "r1", "r2");
        var check2 = register(1L, "r1", "r2");
        var check3 = register(2L, "r1", "r2", Set.of("tag"));

        assertThat(check2.getId()).isEqualTo(check.getId());
        assertThat(check3.getId()).isNotEqualTo(check.getId());
    }

    @Test
    void registersCheckIteration() {
        var check = register(1L, "r1", "r2");
        var iteration = this.apiCache.modifyWithDbTxAndGet(
                cache -> service.registerIterationInTx(
                        cache,
                        CheckIterationEntity.Id.of(check.getId(), IterationType.FAST, 0),
                        CreateIterationParams.builder().build()
                )
        );

        assertThat(iteration).isNotNull();
        assertThat(iteration.getId().getNumber()).isEqualTo(1);

        var nextIteration = this.apiCache.modifyWithDbTxAndGet(
                cache -> service.registerIterationInTx(
                        cache,
                        CheckIterationEntity.Id.of(check.getId(), IterationType.FAST, 0),
                        CreateIterationParams.builder().build()
                )
        );
        assertThat(nextIteration).isNotNull();
        assertThat(nextIteration.getId().getNumber()).isEqualTo(2);

        var nextFullIteration = this.apiCache.modifyWithDbTxAndGet(
                cache -> service.registerIterationInTx(
                        cache, CheckIterationEntity.Id.of(check.getId(), IterationType.FULL, 1),
                        CreateIterationParams.builder().build()
                )
        );
        assertThat(nextFullIteration).isNotNull();
        assertThat(nextFullIteration.getId().getNumber()).isEqualTo(1);
    }

    @Test
    void registersSecondCheckIterationAndAggregate() {
        var check = register(1L, "r1", "r2");

        this.apiCache.modifyWithDbTxAndGet(
                cache -> service.registerIterationInTx(
                        cache, CheckIterationEntity.Id.of(check.getId(), IterationType.FAST, 1),
                        CreateIterationParams.builder().build()
                )
        );

        var iteration = this.apiCache.modifyWithDbTxAndGet(
                cache -> service.registerIterationInTx(
                        cache, CheckIterationEntity.Id.of(check.getId(), IterationType.FAST, 2),
                        CreateIterationParams.builder().build()
                )
        );

        assertThat(iteration).isNotNull();
        assertThat(iteration.getId().getNumber()).isEqualTo(2);

        var aggregateIteration = apiCache.iterations().getFresh(iteration.getId().toMetaId());
        assertThat(aggregateIteration).isPresent();
        assertThat(aggregateIteration.get().getId().getNumber()).isEqualTo(0);
    }

    @Test
    void findsByRevision() {
        register(1L, "r1", "r2");
        register(1L, "r1", "r2", Set.of("tag"));
        register(2L, "r3", "r4");
        register(2L, "r3", "r4");

        assertThat(service.findChecksByRevisions("r1", "r2", Set.of()).size()).isEqualTo(2);
        assertThat(service.findChecksByRevisions("r3", "r4", Set.of()).size()).isEqualTo(1);
    }

    @Test
    void multiThreadRegister() {
        var check = register(1L, "r1", "r2");

        var metaId = CheckIterationEntity.Id.of(check.getId(), IterationType.HEAVY, 0);
        Consumer<StorageApiCache.Modifiable> action = (StorageApiCache.Modifiable cache) -> {
            var iteration = service.registerIterationInTx(
                    cache, metaId, CreateIterationParams.builder().build()
            );
            var metaIterationId = cache.iterations().getFresh(metaId).map(CheckIterationEntity::getId).orElse(null);
            var taskIdOne = new CheckTaskEntity.Id(iteration.getId(), "1");
            var taskIdTwo = new CheckTaskEntity.Id(iteration.getId(), "2");
            var taskTheeTwo = new CheckTaskEntity.Id(iteration.getId(), "3");
            var taskFourTwo = new CheckTaskEntity.Id(iteration.getId(), "4");

            cache.checkTasks().getFresh(Set.of(taskIdOne, taskIdTwo, taskTheeTwo, taskFourTwo));

            service.registerNotExistingTaskInTx(
                    cache, CheckTaskEntity.builder()
                            .id(taskIdOne)
                            .build(),
                    iteration.getId(),
                    metaIterationId
            );

            service.registerNotExistingTaskInTx(
                    cache, CheckTaskEntity.builder()
                            .id(taskIdTwo)
                            .build(),
                    iteration.getId(),
                    metaIterationId
            );

            try {
                Thread.sleep(1);
            } catch (InterruptedException e) {
                throw new RuntimeException("Interrupted", e);
            }

            service.registerNotExistingTaskInTx(
                    cache, CheckTaskEntity.builder()
                            .id(taskTheeTwo)
                            .build(),
                    iteration.getId(),
                    metaIterationId
            );

            service.registerNotExistingTaskInTx(
                    cache, CheckTaskEntity.builder()
                            .id(taskFourTwo)
                            .build(),
                    iteration.getId(),
                    metaIterationId
            );

        };
        var threads = IntStream.range(0, 4)
                .mapToObj(i -> new Thread(() -> this.apiCache.modifyWithDbTx(action)))
                .toList();

        threads.forEach(Thread::start);
        threads.forEach(thread -> {
            try {
                thread.join();
            } catch (InterruptedException e) {
                throw new RuntimeException("Interrupted", e);
            }
        });

        var metaIteration = this.db.currentOrReadOnly(() -> this.db.checkIterations().get(metaId));

        assertThat(metaIteration.getNumberOfTasks()).isEqualTo(16);


        registersCheckIteration();
    }

    private CheckEntity register(long leftRevisionNumber, String leftRevision, String rightRevision) {
        return register(leftRevisionNumber, leftRevision, rightRevision, Set.of());
    }

    private CheckEntity register(long leftRevisionNumber, String leftRevision, String rightRevision, Set<String> tags) {
        return register(leftRevisionNumber, leftRevision, rightRevision, tags, CheckType.TRUNK_POST_COMMIT);
    }

    private CheckEntity register(
            long leftRevisionNumber,
            String leftRevision,
            String rightRevision,
            Set<String> tags,
            CheckType checkType
    ) {
        return service.register(
                CheckEntity.builder()
                        .id(CheckEntity.Id.of(0L))
                        .left(new StorageRevision("trunk", leftRevision, leftRevisionNumber, Instant.EPOCH))
                        .right(new StorageRevision("trunk", rightRevision, leftRevisionNumber, Instant.EPOCH))
                        .tags(tags)
                        .type(checkType)
                        .created(TIMESTAMP)
                        .build()
        );
    }

    static List<Arguments> checkRunLargeTestsAfterDiscovery() {
        return List.of(
                Arguments.of(CheckType.BRANCH_PRE_COMMIT, false),
                Arguments.of(CheckType.BRANCH_POST_COMMIT, false),
                Arguments.of(CheckType.TRUNK_PRE_COMMIT, false),
                Arguments.of(CheckType.TRUNK_POST_COMMIT, true) // Enabled
        );
    }
}
