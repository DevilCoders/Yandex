package ru.yandex.ci.engine.timeline;

import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import com.google.common.collect.Iterables;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.launch.LaunchCancelTask;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doReturn;

@Slf4j
public class ConcurrentTimelineTest extends EngineTestBase {
    private static final CiProcessId PROCESS_ID = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;

    @Autowired
    private LaunchCancelTask launchCancelTask;

    @BeforeEach
    void setUp() {
        mockValidationSuccessful();
        doReturn(TestData.RELEASE_BRANCH_2.asString())
                .when(branchNameGenerator).generateName(any(), any(), anyInt());

        discoveryToR7();
        delegateToken(PROCESS_ID.getPath());
    }

    @AfterEach
    public void tearDown() {
        ((ArcServiceStub) arcService).resetAndInitTestData();
    }

    @Test
    void cancelConcurrent() throws Exception {
        int releasesToCancel = 5;
        var revisions = IntStream.range(2, releasesToCancel + 3)
                .mapToObj(TestData::trunkRevision)
                .collect(Collectors.toList());

        var firstRevision = revisions.get(0);
        var lastRevision = Iterables.getLast(revisions);
        var revisionsToCancel = StreamEx.of(revisions)
                .without(firstRevision, lastRevision)
                .toImmutableList();

        Launch firstLaunch = launchReleaseAt(firstRevision);
        List<Launch> launchesToCancel = StreamEx.of(revisionsToCancel)
                .map(this::launchReleaseAt)
                .toImmutableList();

        AtomicReference<Launch> lastLaunch = new AtomicReference<>();

        var executorService = Executors.newFixedThreadPool(revisionsToCancel.size() + 1);

        var callables = launchesToCancel.stream()
                .map(launch -> (Callable<Void>) () -> {
                    cancelLaunch(launch);
                    return null;
                })
                .collect(Collectors.toList());

        callables.add(() -> {
            var launch = launchReleaseAt(lastRevision);
            log.info("Created launch at {}: {}", lastRevision, launch.getId().getLaunchNumber());
            lastLaunch.set(launch);
            return null;
        });

        var futures = executorService.invokeAll(callables);
        executorService.shutdown();

        for (Future<Void> future : futures) {
            future.get(555, TimeUnit.SECONDS);
        }

        assertThat(StreamEx.of(launchesToCancel)
                .map(ConcurrentTimelineTest.this::getLaunch)
                .map(Launch::getStatus).toList())
                .containsOnly(LaunchState.Status.CANCELED);

        var lastLaunchUpdated = getLaunch(lastLaunch.get());

        var cancelledNumbers = launchesToCancel.stream()
                .map(l -> l.getLaunchId().getNumber())
                .collect(Collectors.toSet());

        // все предыдущие релизы отменились и передали по цепочке свои коммиты наверх до первого
        assertThat(lastLaunchUpdated.getStatus().isTerminal()).isFalse();

        assertThat(lastLaunchUpdated.getVcsInfo().getPreviousRevision())
                .isEqualTo(firstLaunch.getVcsInfo().getRevision());

        assertThat(lastLaunchUpdated.getCancelledReleases()).isEqualTo(cancelledNumbers).isNotEmpty();
    }

    private void cancelLaunch(Launch launchToCancel) {
        log.info("Cancelling {}", launchToCancel.getId().getLaunchNumber());
        launchService.cancel(launchToCancel.getLaunchId(), TestData.CI_USER, "");

        var task = new LaunchCancelTask(launchCancelTask);
        task.setParameters(new LaunchCancelTask.Params(launchToCancel.getLaunchId()));
        try {
            task.execute(null);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        Launch cancelled = getLaunch(launchToCancel);

        assertThat(cancelled).isNotNull();
        assertThat(cancelled.getStatus()).isEqualTo(LaunchState.Status.CANCELED);
    }

    private Launch getLaunch(Launch launch) {
        return db.currentOrTx(() -> db.launches().get(launch.getLaunchId()));
    }

    private Launch launchReleaseAt(OrderedArcRevision revision) {
        var launch = launchService.startRelease(PROCESS_ID, revision, revision.getBranch(), TestData.CI_USER, null,
                false, false, null, true, null, null, null);
        log.info("Create launch at {}: {}", revision, launch.getId().getLaunchNumber());
        return launch;
    }

}
