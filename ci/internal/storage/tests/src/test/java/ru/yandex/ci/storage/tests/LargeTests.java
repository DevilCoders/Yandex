package ru.yandex.ci.storage.tests;

import java.util.List;
import java.util.NoSuchElementException;
import java.util.function.Function;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static ru.yandex.ci.storage.core.Common.CheckTaskType.CTT_LARGE_TEST;

public class LargeTests extends StorageTestsYdbTestBase {

    @Test
    public void discoverAndStartSameTestsMultipleTimes() {
        var fullIteration = discoverAndStart(diffIds -> List.of(diffIds, diffIds, diffIds, diffIds));

        var metaIterationId = new CheckIterationEntity.Id(
                fullIteration.getId().getCheckId(),
                CheckIteration.IterationType.HEAVY.getNumber(),
                0
        );

        // Meta-iteration must not exists - all tests are registered at once then deduplicated
        assertThatThrownBy(() -> storageTester.getIteration(metaIterationId))
                .isInstanceOf(NoSuchElementException.class)
                .hasMessage("Unable to find key [[100000000000/HEAVY/0]] in table [CheckIterations]");

        var heavyFirst = storageTester.getIteration(metaIterationId.toIterationId(1));
        assertThat(heavyFirst.getNumberOfTasks()).isEqualTo(6);
    }

    @Test
    public void discoverAndStartDifferentTestsMultipleTimes() {
        var fullIteration = discoverAndStart(diffIds -> {
                    assertThat(diffIds).hasSize(3);

                    // Return some mixed combinations of tests, only unique set will be started
                    return List.of(
                            List.of(diffIds.get(0), diffIds.get(2)),
                            diffIds.subList(0, 1),
                            diffIds.subList(2, 3),
                            diffIds.subList(1, 3)
                    );
                }
        );

        // Meta-iteration is expected - there is no way we could register all tests in a single call
        var heavyMeta = storageTester.getIteration(
                new CheckIterationEntity.Id(
                        fullIteration.getId().getCheckId(),
                        CheckIteration.IterationType.HEAVY.getNumber(),
                        0
                )
        );

        assertThat(heavyMeta.getNumberOfTasks()).isEqualTo(6);
    }

    private CheckIterationEntity discoverAndStart(
            Function<List<StorageFrontApi.DiffId>, List<List<StorageFrontApi.DiffId>>> actionBuilder
    ) {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration().leftTask(CTT_LARGE_TEST).rightTask(CTT_LARGE_TEST)
        );

        var testOne = exampleLargeTestSuite(101).toBuilder()
                .setPath("1")
                .setTestStatus(Common.TestStatus.TS_DISCOVERED)
                .build();
        var testTwo = exampleLargeTestSuite(102).toBuilder()
                .setPath("2")
                .setTestStatus(Common.TestStatus.TS_DISCOVERED)
                .build();
        var testThree = exampleLargeTestSuite(103).toBuilder()
                .setPath("3")
                .setTestStatus(Common.TestStatus.TS_DISCOVERED)
                .build();

        storageTester.writeAndDeliver(
                registration, writer -> writer.toAll().results(testOne, testTwo, testThree).finish()
        );

        var searchResult = storageTester.frontApi().listLargeTestsToolchains(
                registration.getIteration().getId().getCheckId()
        );

        assertThat(searchResult.getTestsList()).hasSize(3);

        var diffIds = searchResult.getTestsList().stream()
                .map(StorageFrontApi.LargeTestResponse::getTestDiff)
                .toList();

        // Each unique test will registered only once, i.e. only 3x2 tests will be launched in total
        var threads = actionBuilder.apply(diffIds).stream()
                .map(list -> {
                    assertThat(list).isNotEmpty();
                    return (Runnable) () -> storageTester.frontApi().startLargeTests(list);
                })
                .map(Thread::new)
                .toList();

        assertThat(threads).isNotEmpty();

        threads.forEach(Thread::start);
        threads.forEach(thread -> {
            try {
                thread.join();
            } catch (InterruptedException e) {
                throw new RuntimeException("Interrupted", e);
            }
        });

        return registration.getIteration();
    }
}
