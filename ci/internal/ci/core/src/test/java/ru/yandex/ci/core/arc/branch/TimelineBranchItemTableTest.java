package ru.yandex.ci.core.arc.branch;

import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.common.ydb.YdbExecutor;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainTables;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.core.timeline.BranchState;
import ru.yandex.ci.core.timeline.BranchVcsInfo;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.core.timeline.TimelineBranchItemByUpdateDate.Offset;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

class TimelineBranchItemTableTest extends CommonYdbTestBase {
    private static final CiProcessId RELEASE_ONE_1 = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release_1");
    private static final CiProcessId RELEASE_TWO_2 = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release_2");

    @Autowired
    private YdbExecutor executor;

    @Test
    void findByUpdateDate() {
        db.currentOrTx(() -> db.timelineBranchItems().save(
                item(RELEASE_ONE_1, "branch-01", 10),
                item(RELEASE_ONE_1, "branch-02", 11),
                item(RELEASE_TWO_2, "branch-03", 13),
                item(RELEASE_ONE_1, "branch-04", 15),
                item(RELEASE_TWO_2, "branch-05", 18),
                item(RELEASE_ONE_1, "branch-06", 20),
                item(RELEASE_ONE_1, "branch-07", 22),
                item(RELEASE_TWO_2, "branch-08", 23),
                item(RELEASE_TWO_2, "branch-09", 23),
                item(RELEASE_ONE_1, "branch-10", 24),
                item(RELEASE_TWO_2, "branch-11", 28),
                item(RELEASE_ONE_1, "branch-12", 29)
        ));

        assertThat(db.currentOrTx(() -> db.timelineBranchItems().findByUpdateDate(RELEASE_ONE_1, null, 3)))
                .containsExactly(
                        item(RELEASE_ONE_1, "branch-12", 29),
                        item(RELEASE_ONE_1, "branch-10", 24),
                        item(RELEASE_ONE_1, "branch-07", 22)
                );

        Offset offset = Offset.of(item(RELEASE_ONE_1, "branch-07", 22));

        assertThat(db.currentOrTx(() -> db.timelineBranchItems().findByUpdateDate(RELEASE_ONE_1, offset, 2)))
                .containsExactly(
                        item(RELEASE_ONE_1, "branch-06", 20),
                        item(RELEASE_ONE_1, "branch-04", 15)
                );

    }

    @Test
    void findLastUpdatedInProcesses() {
        db.currentOrTx(() -> db.timelineBranchItems().save(
                item(RELEASE_ONE_1, "branch-01", 10),
                item(RELEASE_ONE_1, "branch-02", 11),
                item(RELEASE_TWO_2, "branch-03", 13),
                item(RELEASE_ONE_1, "branch-04", 15),
                item(RELEASE_TWO_2, "branch-05", 18),
                item(RELEASE_ONE_1, "branch-06", 20),
                item(RELEASE_ONE_1, "branch-07", 22),
                item(RELEASE_TWO_2, "branch-08", 23),
                item(RELEASE_TWO_2, "branch-09", 23),
                item(RELEASE_ONE_1, "branch-10", 24),
                item(RELEASE_TWO_2, "branch-11", 28),
                item(RELEASE_ONE_1, "branch-12", 29)
        ));

        List<TimelineBranchItem> top =
                db.currentOrTx(() -> db.timelineBranchItems().findLastUpdatedInProcesses(
                        List.of(RELEASE_ONE_1, RELEASE_TWO_2), 3
                ));

        assertThat(top)
                .isEqualTo(List.of(
                        item(RELEASE_ONE_1, "branch-12", 29),
                        item(RELEASE_ONE_1, "branch-10", 24),
                        item(RELEASE_ONE_1, "branch-07", 22),
                        item(RELEASE_TWO_2, "branch-11", 28),
                        item(RELEASE_TWO_2, "branch-08", 23),
                        item(RELEASE_TWO_2, "branch-09", 23)
                ));
    }


    @Test
    void findLastUpdatedInProcesses_shouldNotThrowResultTruncatedException() {
        int groupLimit = 3;
        var processIds = new ArrayList<CiProcessId>();
        var expectedItemsMap = new HashMap<CiProcessId, List<TimelineBranchItem>>();

        db.currentOrTx(() -> {
            IntStream.range(0, 335).forEach(processIdSuffix -> {
                var processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release_" + processIdSuffix);
                processIds.add(processId);

                for (int branchNumber = 9; branchNumber >= 5; branchNumber--) {
                    var branch = "branch-%d-%d".formatted(processIdSuffix, branchNumber);
                    var item = item(processId, branch, 10 + branchNumber);
                    db.timelineBranchItems().save(item);

                    var expectedItems = expectedItemsMap.computeIfAbsent(processId, k -> new ArrayList<>());
                    if (expectedItems.size() < groupLimit) {
                        expectedItems.add(item);
                    }
                }
            });
        });

        assertThat(processIds.size() * groupLimit).isGreaterThan(YdbUtils.RESULT_ROW_LIMIT);

        var top = db.currentOrTx(() -> db.timelineBranchItems().findLastUpdatedInProcesses(processIds, groupLimit));
        assertThat(top)
                .hasSizeGreaterThan(YdbUtils.RESULT_ROW_LIMIT)
                .containsExactlyInAnyOrderElementsOf(
                        processIds.stream()
                                .flatMap(processId -> expectedItemsMap.get(processId).stream())
                                .toList()
                );
    }

    @Test
    void findByUpdateDate_afterUpdate() {
        db.currentOrTx(() -> db.timelineBranchItems().save(
                item(RELEASE_ONE_1, "branch-01", 10),
                item(RELEASE_TWO_2, "branch-03", 13),
                item(RELEASE_ONE_1, "branch-04", 15),
                item(RELEASE_TWO_2, "branch-05", 18)
        ));

        assertThat(db.currentOrTx(() -> db.timelineBranchItems().findByUpdateDate(RELEASE_TWO_2, null, 50)))
                .containsExactly(
                        item(RELEASE_TWO_2, "branch-05", 18),
                        item(RELEASE_TWO_2, "branch-03", 13)
                );

        db.currentOrTx(() -> db.timelineBranchItems().save(item(RELEASE_TWO_2, "branch-03", 97)));

        assertThat(db.currentOrTx(() -> db.timelineBranchItems().findByUpdateDate(RELEASE_TWO_2, null, 50)))
                .containsExactly(
                        item(RELEASE_TWO_2, "branch-03", 97),
                        item(RELEASE_TWO_2, "branch-05", 18)
                );
    }

    @Test
    void findByName() {
        var items = List.of(
                item(RELEASE_ONE_1, "branch-01"),
                item(RELEASE_TWO_2, "branch-02"),
                item(RELEASE_ONE_1, "branch-04"),
                item(RELEASE_TWO_2, "branch-05"),
                item(RELEASE_TWO_2, "branch-04")
        );
        db.currentOrTx(() ->
                db.timelineBranchItems().save(items));

        ArcBranch branch = branch("branch-04");
        List<TimelineBranchItem> load = db.currentOrTx(() -> db.timelineBranchItems().findByName(branch));

        assertThat(load)
                .extracting(TimelineBranchItem::getProcessId)
                .containsExactlyInAnyOrder(RELEASE_ONE_1, RELEASE_TWO_2);

        assertThat(load)
                .extracting(TimelineBranchItem::getArcBranch)
                .containsOnly(branch);
    }

    @Test
    void findByNames() {
        var items = List.of(
                item(RELEASE_ONE_1, "branch-01", 10),
                item(RELEASE_TWO_2, "branch-03", 13),
                item(RELEASE_ONE_1, "branch-04", 15),
                item(RELEASE_TWO_2, "branch-05", 18)
        );
        db.currentOrTx(() ->
                db.timelineBranchItems().save(items));

        var allBranchNames = items.stream()
                .map(TimelineBranchItem::getArcBranch)
                .collect(Collectors.toList());

        assertThat(allBranchNames).hasSize(4);

        assertThat(db.currentOrTx(() -> db.timelineBranchItems().findIdsByNames(RELEASE_ONE_1, allBranchNames)))
                .describedAs("should return only branches for related process")
                .extracting(TimelineBranchItem.Id::getBranch)
                .containsExactlyInAnyOrder(
                        "releases/ci/branch-01", "releases/ci/branch-04"
                );
    }

    @Test
    void backwardCompatibility() {
        TestUtils.forTable(db, CiMainTables::timelineBranchItems)
                .upsertValues(executor, "ydb-data/branch-item.json");
        var items = db.currentOrReadOnly(() -> db.timelineBranchItems().findAll());
        assertThat(items).isNotEmpty();

        assertThat(items)
                .extracting(TimelineBranchItem::getState)
                .extracting(BranchState::getCancelledBaseLaunches)
                .doesNotContainNull();

        assertThat(items)
                .extracting(TimelineBranchItem::getState)
                .extracting(BranchState::getCancelledBranchLaunches)
                .doesNotContainNull();
    }


    private TimelineBranchItem item(CiProcessId processId, String branch) {
        return item(processId, branch, 0);
    }

    private TimelineBranchItem item(CiProcessId processId, String branch, int time) {
        return TimelineBranchItem.builder()
                .idOf(processId, branch(branch))
                .vcsInfo(BranchVcsInfo.builder()
                        .updatedDate(Instant.ofEpochSecond(time))
                        .build())
                .version(Version.majorMinor("13", "42"))
                .build();
    }

    private static ArcBranch branch(String branch) {
        return ArcBranch.ofBranchName("releases/ci/" + branch);
    }

}
