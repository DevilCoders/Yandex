package ru.yandex.ci.core.discovery;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.base.Stopwatch;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

@Slf4j
class DiscoveredCommitTableTest extends CommonYdbTestBase {

    private static final CiProcessId PROCESS_ID = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");

    /**
     * Проверяем весь флоу работы. Сохраняет объект и два раза его обновляем
     */
    @Test
    void testFlow() {
        OrderedArcRevision revision = OrderedArcRevision.fromHash(
                "4de6c109b3febe14afa3c359679f784b90076817", ArcBranch.ofPullRequest(42), 42, 43
        );

        Assertions.assertTrue(
                findCommit(PROCESS_ID, revision).isEmpty()
        );

        DiscoveredCommitState firstState = DiscoveredCommitState.builder()
                .dirDiscovery(true)
                .build();

        DiscoveredCommit firstActualCommit = updateOrCreate(PROCESS_ID, revision, discoveredCommit -> {
            Assertions.assertTrue(discoveredCommit.isEmpty());
            return firstState;
        });

        DiscoveredCommit firstExpectedCommit = DiscoveredCommit.of(PROCESS_ID, revision, 1, firstState);
        Assertions.assertEquals(firstExpectedCommit, firstActualCommit);
        Assertions.assertEquals(firstExpectedCommit, findCommit(PROCESS_ID, revision).orElseThrow());

        DiscoveredCommitState secondState = DiscoveredCommitState.builder()
                .dirDiscovery(true)
                .graphDiscovery(true)
                .launchIds(List.of(LaunchId.of(PROCESS_ID, 42)))
                .build();

        DiscoveredCommit secondActualCommit = updateOrCreate(PROCESS_ID, revision, state -> {
            Assertions.assertEquals(firstExpectedCommit.getState(), state.orElseThrow());
            return secondState;
        });

        DiscoveredCommit secondExpectedCommit = DiscoveredCommit.of(PROCESS_ID, revision, 2, secondState);

        Assertions.assertEquals(secondExpectedCommit, secondActualCommit);
        Assertions.assertEquals(secondExpectedCommit, findCommit(PROCESS_ID, revision).orElseThrow());
        Assertions.assertEquals(
                secondExpectedCommit, findCommit(PROCESS_ID, revision).orElseThrow()
        );

        Assertions.assertEquals(
                List.of(secondExpectedCommit),
                db.currentOrReadOnly(() ->
                        db.discoveredCommit().findCommits(PROCESS_ID, revision.getBranch(), -1, -1, -1))
        );

    }

    @Test
    void testParallel() throws Exception {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "release");
        OrderedArcRevision revision = OrderedArcRevision.fromHash(
                "a6bb27178d80cf1f66e62ba6b01ec26bcb8b1f42", ArcBranch.trunk(), 21, 22
        );

        int updatesCount = 50;

        Stopwatch stopwatch = Stopwatch.createStarted();

        ExecutorService executorService = Executors.newFixedThreadPool(updatesCount / 10);

        List<Callable<Integer>> callables = new ArrayList<>();

        for (int i = 0; i < updatesCount; i++) {
            callables.add(() -> {
                DiscoveredCommit commit = updateOrCreate(processId, revision, discoveredCommitState -> {
                    int nextLaunchNumber = discoveredCommitState.map(DiscoveredCommitState::getLastLaunchId)
                            .map(l -> l.getNumber() + 1)
                            .orElse(1);

                    return discoveredCommitState.map(DiscoveredCommitState::toBuilder)
                            .orElse(DiscoveredCommitState.builder())
                            .dirDiscovery(true)
                            .launchId(LaunchId.of(processId, nextLaunchNumber))
                            .build();
                });
                return commit.getState().getLastLaunchId().getNumber();
            });
        }

        Set<Integer> launchNumbers = new HashSet<>();
        for (Future<Integer> future : executorService.invokeAll(callables, 1, TimeUnit.MINUTES)) {
            launchNumbers.add(future.get());
        }

        log.info(
                "{} parallel test executed in {} millis",
                updatesCount,
                stopwatch.stop().elapsed(TimeUnit.MILLISECONDS)
        );
        executorService.shutdown();

        Assertions.assertEquals(updatesCount, launchNumbers.size());
        for (int i = 1; i <= updatesCount; i++) {
            Assertions.assertTrue(launchNumbers.contains(i));
        }

        Assertions.assertEquals(
                DiscoveredCommit.of(
                        processId,
                        revision,
                        updatesCount,
                        DiscoveredCommitState.builder()
                                .dirDiscovery(true)
                                .launchIds(
                                        launchNumbers.stream()
                                                .sorted()
                                                .map(number -> new LaunchId(processId, number))
                                                .collect(Collectors.toList())
                                )
                                .build()
                ),
                findCommit(processId, revision).orElseThrow()
        );

    }

    @Test
    void testCompatibility_readValueViaMainDb_afterInsertionViaDao() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        OrderedArcRevision revision = OrderedArcRevision.fromHash(
                "4de6c109b3febe14afa3c359679f784b90076817", ArcBranch.ofPullRequest(42), 42, 43
        );

        DiscoveredCommitState firstState = DiscoveredCommitState.builder()
                .dirDiscovery(true)
                .configChange(new ConfigChange(ConfigChangeType.NONE))
                .graphDiscovery(false)
                .launchIds(List.of(
                        LaunchId.of(processId, 1),
                        LaunchId.of(processId, 2)
                ))
                .build();

        DiscoveredCommit firstActualCommit = updateOrCreate(
                processId, revision,
                discoveredCommit -> {
                    Assertions.assertTrue(discoveredCommit.isEmpty());
                    return firstState;
                }
        );

        assertThat(findCommit(processId, revision).orElseThrow())
                .isEqualTo(firstActualCommit);
    }

    @Test
    void testCompatibility_readValueViaDao_afterInsertionViaMainDb() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        OrderedArcRevision revision = OrderedArcRevision.fromHash(
                "4de6c109b3febe14afa3c359679f784b90076817", ArcBranch.ofPullRequest(42), 42, 43
        );

        DiscoveredCommitState firstState = DiscoveredCommitState.builder()
                .dirDiscovery(true)
                .configChange(new ConfigChange(ConfigChangeType.NONE))
                .graphDiscovery(false)
                .launchIds(List.of(
                        LaunchId.of(processId, 1),
                        LaunchId.of(processId, 2)
                ))
                .build();

        DiscoveredCommit firstActualCommit = updateOrCreate(processId, revision, discoveredCommit -> {
            Assertions.assertTrue(discoveredCommit.isEmpty());
            return firstState;
        });

        assertThat(findCommit(processId, revision).orElseThrow())
                .isEqualTo(firstActualCommit);
    }

    @Nested
    class Count {
        @BeforeEach
        public void setUp() {
            db.currentOrTx(() -> {
                for (int i = 10; i <= 20; i++) {
                    updateOrCreate(PROCESS_ID, revision(i), new ConfigChange(ConfigChangeType.NONE));
                }
            });
        }

        @Test
        void toIsExclusive() {
            assertThat(count(PROCESS_ID, ArcBranch.trunk(), 20, 10))
                    .isEqualTo(10);
        }

        @Test
        void wrongOrder() {
            assertThatThrownBy(() -> count(PROCESS_ID, ArcBranch.trunk(), 11, 15))
                    .isInstanceOf(IllegalArgumentException.class)
                    .hasMessage("lteCommitNumber (11) must be more or equal than gtCommitNumber (15)");
        }

        @Test
        void onlyFrom() {
            assertThat(count(PROCESS_ID, ArcBranch.trunk(), 11, 0)).isEqualTo(2);
        }

        @Test
        void zeroIsNoRestriction() {
            assertThat(count(PROCESS_ID, ArcBranch.trunk(), 0, 0)).isEqualTo(11);
        }
    }

    private OrderedArcRevision revision(int number) {
        return OrderedArcRevision.fromHash(
                Integer.toHexString(number), ArcBranch.trunk(), number, 0
        );
    }

    private int count(CiProcessId processId, ArcBranch branch, long fromCommitNumber, long toCommitNumber) {
        return db.currentOrReadOnly(() ->
                db.discoveredCommit().count(processId, branch, fromCommitNumber, toCommitNumber));
    }

    private Optional<DiscoveredCommit> findCommit(CiProcessId ciProcessId, OrderedArcRevision revision) {
        return db.currentOrReadOnly(() ->
                db.discoveredCommit().findCommit(ciProcessId, revision));
    }

    private DiscoveredCommit updateOrCreate(CiProcessId processId, OrderedArcRevision revision,
                                            ConfigChange configChange) {
        return db.currentOrTx(() ->
                db.discoveredCommit().updateOrCreate(processId, revision, configChange, DiscoveryType.DIR));
    }

    private DiscoveredCommit updateOrCreate(CiProcessId processId, OrderedArcRevision revision,
                                            Function<Optional<DiscoveredCommitState>, DiscoveredCommitState> function) {
        return db.currentOrTx(() ->
                db.discoveredCommit().updateOrCreate(processId, revision, function));
    }
}
