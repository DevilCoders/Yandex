package ru.yandex.ci.storage.reader.check.suspicious;

import java.util.Set;
import java.util.function.Consumer;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.SearchDiffQueries;

@Slf4j
public class NewOwnedFlakyRule implements CheckAnalysisRule {
    private final CiStorageDb db;

    public NewOwnedFlakyRule(CiStorageDb db) {
        this.db = db;
    }

    @Override
    public void analyzeIteration(CheckIterationEntity iteration, Consumer<String> addAlert) {
        var extended = iteration.getStatistics().getAllToolchain().getExtended();
        if (
                extended.getMutedOrEmpty().getTotal() +
                        extended.getFlakyOrEmpty().getTotal() +
                        extended.getTimeoutOrEmpty().getTotal() == 0
        ) {
            return;
        }

        var searchFilters = SearchDiffQueries
                .getSearchDiffsFilter(iteration.getId(), DiffSearchFilters.builder()
                        .diffTypes(Set.of(
                                Common.TestDiffType.TDT_TIMEOUT_BROKEN,
                                Common.TestDiffType.TDT_TIMEOUT_NEW,
                                Common.TestDiffType.TDT_FLAKY_BROKEN,
                                Common.TestDiffType.TDT_FLAKY_NEW))
                        .build());
        var predicate = searchFilters.get()
                .and("isStrongMode").eq(false)
                .and("isOwner").eq(true);

        db.scan().run(() -> {
            var suspiciousDiffsCount = db.testDiffs().count(predicate);
            if (suspiciousDiffsCount > 0) {
                log.info(
                        "Suspicious {}: {} new owned flaky or timeout tests", iteration.getId(), suspiciousDiffsCount
                );

                addAlert.accept("%d new owned flaky or timeout tests found".formatted(suspiciousDiffsCount));
            }
        });
    }
}
