package ru.yandex.ci.storage.core.db.model.check_text_search;

import java.util.List;

import com.yandex.ydb.table.settings.PartitioningSettings;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.client.YdbTableHint;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckSearchEntityType;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.ydb.HintRegistry;

@Value(staticConstructor = "of")
@Table(name = "CheckTextSearch")
public class CheckTextSearchEntity implements Entity<CheckTextSearchEntity> {
    private static YdbTableHint ydbTableHint;

    static {
        HintRegistry.getInstance().hint(
                CheckTextSearchEntity.class,
                CheckEntity.NUMBER_OF_ID_PARTITIONS,
                partitions -> ydbTableHint = HintRegistry.byCheckEntityPoints(
                        partitions,
                        new PartitioningSettings().setPartitioningBySize(true)
                )
        );
    }

    CheckTextSearchEntity.Id id;

    Common.ResultType resultType;

    String displayValue;

    public static List<CheckTextSearchEntity> index(TestResult result) {
        if (result.getName().isEmpty() && result.getSubtestName().isEmpty()) {
            return List.of(indexPath(result));
        }

        if (result.getSubtestName().isEmpty()) {
            return List.of(indexPath(result), indexTestName(result));
        }

        if (result.getName().isEmpty()) {
            return List.of(indexPath(result), indexSubtestName(result));
        }

        return List.of(indexPath(result), indexTestName(result), indexSubtestName(result));
    }

    private static CheckTextSearchEntity of(
            TestResult result, CheckSearchEntityType entityType, String name
    ) {
        var lowerName = name.toLowerCase();
        return new CheckTextSearchEntity(
                CheckTextSearchEntity.Id.index(result.getId(), entityType, lowerName),
                ResultTypeUtils.toSuiteType(result.getResultType()),
                name.equals(lowerName) ? "" : name
        );
    }

    private static CheckTextSearchEntity indexPath(TestResult result) {
        return CheckTextSearchEntity.of(result, CheckSearchEntityType.CSET_PATH, result.getPath());
    }

    private static CheckTextSearchEntity indexSubtestName(TestResult result) {
        return CheckTextSearchEntity.of(
                result, CheckSearchEntityType.CSET_SUBTEST_NAME, result.getSubtestName()
        );
    }

    private static CheckTextSearchEntity indexTestName(TestResult result) {
        return CheckTextSearchEntity.of(result, CheckSearchEntityType.CSET_TEST_NAME, result.getName());
    }

    @Override
    public CheckTextSearchEntity.Id getId() {
        return id;
    }

    @Value
    @Builder
    @AllArgsConstructor
    public static class Id implements Entity.Id<CheckTextSearchEntity> {
        long checkId;

        @Column(dbType = DbType.UINT8)
        int iterationType;

        @Column(dbType = DbType.UINT8)
        int entityType;

        String value;

        @Column(dbType = DbType.UINT64)
        long suiteId;

        @Column(dbType = DbType.UINT64)
        long testId;

        public static Id index(
                TestResultEntity.Id resultId, CheckSearchEntityType entityType, String value
        ) {
            return new Id(
                    resultId.getCheckId().getId(),
                    resultId.getIterationType(),
                    entityType.getNumber(),
                    value,
                    resultId.getSuiteId(),
                    resultId.getTestId()
            );
        }
    }

    @Value
    public static class DistinctView implements yandex.cloud.repository.db.Table.View {
        Id id;

        String displayValue;

        @Value
        public static class Id {
            long checkId;

            @Column(dbType = DbType.UINT8)
            int iterationType;

            @Column(dbType = DbType.UINT8)
            int entityType;

            String value;
        }
    }
}
