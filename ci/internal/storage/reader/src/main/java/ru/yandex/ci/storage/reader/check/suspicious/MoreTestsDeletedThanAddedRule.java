package ru.yandex.ci.storage.reader.check.suspicious;

import java.util.Set;
import java.util.function.Consumer;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.SearchDiffQueries;

@AllArgsConstructor
@Slf4j
public class MoreTestsDeletedThanAddedRule implements CheckAnalysisRule {
    private final CiStorageDb db;

    private final int deletedPercent;
    private final int deletedMin;

    @Override
    public void analyzeIteration(CheckIterationEntity iteration, Consumer<String> addAlert) {
        var extended = iteration.getStatistics().getAllToolchain().getExtended();
        var added = extended.getAdded() != null ? extended.getAdded().getTotal() : 0;
        var deleted = extended.getDeleted() != null ? extended.getDeleted().getTotal() : 0;

        var suspicious = deleted >= deletedMin && deleted * 100 > added * deletedPercent;

        if (!suspicious) {
            return;
        }

        var deletedPredicate = SearchDiffQueries
                .getSearchDiffsFilter(iteration.getId(), DiffSearchFilters.builder()
                        .diffTypes(Set.of(Common.TestDiffType.TDT_DELETED))
                        .resultTypes(
                                Set.of(
                                        Common.ResultType.RT_TEST_SMALL,
                                        Common.ResultType.RT_TEST_SUITE_SMALL,
                                        Common.ResultType.RT_TEST_MEDIUM,
                                        Common.ResultType.RT_TEST_SUITE_MEDIUM,
                                        Common.ResultType.RT_TEST_LARGE,
                                        Common.ResultType.RT_TEST_SUITE_LARGE
                                )
                        )
                        .build()).get();

        deleted = (int) (long) db.scan().run(() -> db.testDiffs().count(deletedPredicate));
        suspicious = deleted >= deletedMin && deleted * 100 > added * deletedPercent;

        if (suspicious) {
            log.info("Suspicious {}: {} tests deleted, {} added", iteration.getId(), deleted, added);
            addAlert.accept("%d tests deleted, only %d added".formatted(deleted, added));
        }
    }
}
