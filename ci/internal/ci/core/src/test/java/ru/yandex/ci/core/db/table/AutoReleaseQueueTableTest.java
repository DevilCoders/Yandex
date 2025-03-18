package ru.yandex.ci.core.db.table;

import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.model.AutoReleaseQueueItem;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;

public class AutoReleaseQueueTableTest extends CommonYdbTestBase {

    private static final AutoReleaseQueueItem.State WAITING_CONDITIONS = AutoReleaseQueueItem.State.WAITING_CONDITIONS;

    @Test
    void findByState() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        OrderedArcRevision rev0 = TestData.TRUNK_R1;
        OrderedArcRevision rev2 = TestData.TRUNK_R2;

        var rev0WaitingPreviousCommits = AutoReleaseQueueItem.of(rev0, processId,
                AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS);
        var rev2WaitingConditions = AutoReleaseQueueItem.of(rev2, processId, WAITING_CONDITIONS);
        db.currentOrTx(() -> {
            db.autoReleaseQueue().save(rev0WaitingPreviousCommits);
            db.autoReleaseQueue().save(rev2WaitingConditions);
        });
        db.currentOrReadOnly(() -> {
            assertThat(
                    db.autoReleaseQueue().findByState(AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS)
            ).isEqualTo(List.of(rev0WaitingPreviousCommits));
            assertThat(
                    db.autoReleaseQueue().findByState(WAITING_CONDITIONS)
            ).isEqualTo(List.of(rev2WaitingConditions));
        });
    }

    @Test
    void findByState_testPagination() {
        Set<AutoReleaseQueueItem> autoReleaseQueueItems = db.currentOrTx(() ->
                IntStream.range(1, 2001)
                        .mapToObj(i -> AutoReleaseQueueItem.of(
                                TestData.TRUNK_R1,
                                CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release" + "_" + i),
                                WAITING_CONDITIONS
                        ))
                        .map(db.autoReleaseQueue()::save)
                        .collect(Collectors.toSet())
        );
        db.currentOrReadOnly(() -> {
            assertThat(
                    Set.copyOf(db.autoReleaseQueue().findByState(WAITING_CONDITIONS))
            ).isEqualTo(autoReleaseQueueItems);
        });
    }

    @Test
    void findByProcessIdAndState_testPagination() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        Set<AutoReleaseQueueItem> autoReleaseQueueItems = db.currentOrTx(() ->
                IntStream.range(1, 2001)
                        .mapToObj(i -> AutoReleaseQueueItem.of(
                                OrderedArcRevision.fromHash(
                                        "hash" + i, ArcBranch.trunk(), i, i
                                ),
                                processId,
                                WAITING_CONDITIONS
                        ))
                        .map(db.autoReleaseQueue()::save)
                        .collect(Collectors.toSet())
        );
        db.currentOrReadOnly(() -> {
            assertThat(
                    Set.copyOf(db.autoReleaseQueue().findByProcessIdAndState(processId, WAITING_CONDITIONS))
            ).isEqualTo(autoReleaseQueueItems);
        });
    }

    @Test
    void findByProcessIdAndState() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        OrderedArcRevision rev0 = TestData.TRUNK_R1;
        OrderedArcRevision rev2 = TestData.TRUNK_R2;

        var rev0WaitingPreviousCommits = AutoReleaseQueueItem.of(rev0, processId,
                AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS);
        var rev2WaitingConditions = AutoReleaseQueueItem.of(rev2, processId, WAITING_CONDITIONS);

        db.currentOrTx(() -> {
            db.autoReleaseQueue().save(rev0WaitingPreviousCommits);
            db.autoReleaseQueue().save(rev2WaitingConditions);
        });

        db.currentOrTx(() -> {
            assertThat(
                    db.autoReleaseQueue().findByProcessIdAndState(
                            processId, AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS
                    )
            ).isEqualTo(List.of(rev0WaitingPreviousCommits));
            assertThat(
                    db.autoReleaseQueue().findByProcessIdAndState(
                            processId, WAITING_CONDITIONS
                    )
            ).isEqualTo(List.of(rev2WaitingConditions));
        });

        // check that table AutoReleaseQueueItem.ByProcessAndStateId is updated
        var updatedRelease = rev2WaitingConditions.withState(AutoReleaseQueueItem.State.CHECKING_FREE_STAGE);
        db.currentOrTx(() -> db.autoReleaseQueue().save(updatedRelease));
        db.currentOrTx(() ->
                assertThat(
                        db.autoReleaseQueue().findByProcessIdAndState(
                                processId, AutoReleaseQueueItem.State.CHECKING_FREE_STAGE
                        )
                ).isEqualTo(List.of(updatedRelease))
        );
    }

    @Test
    public void saveOrUpdate_shouldUpdateProjections() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        OrderedArcRevision rev0 = TestData.TRUNK_R1;

        var item = AutoReleaseQueueItem.of(rev0, processId, AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS);
        db.currentOrTx(() -> db.autoReleaseQueue().save(item));
        db.currentOrTx(() ->
                assertThat(
                        db.autoReleaseQueue().findByState(AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS)
                ).containsExactly(item)
        );

        var updatedItem = item.withState(WAITING_CONDITIONS);
        db.currentOrTx(() -> db.autoReleaseQueue().save(updatedItem));
        db.currentOrTx(() -> {
            assertThat(
                    db.autoReleaseQueue().findByState(AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS)
            ).isEmpty();
            assertThat(
                    db.autoReleaseQueue().findByState(WAITING_CONDITIONS)
            ).containsExactly(updatedItem);
        });
    }

}
