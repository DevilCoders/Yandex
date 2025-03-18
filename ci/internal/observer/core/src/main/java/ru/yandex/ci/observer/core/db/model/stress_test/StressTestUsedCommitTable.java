package ru.yandex.ci.observer.core.db.model.stress_test;

import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;

public class StressTestUsedCommitTable extends KikimrTableCi<StressTestUsedCommitEntity> {

    public StressTestUsedCommitTable(KikimrTable.QueryExecutor executor) {
        super(StressTestUsedCommitEntity.class, executor);
    }

    public Set<String> findUsedRightRevisions(
            @Nonnull String namespace,
            long minLeftRevisionNumberIncluded,
            long maxLeftRevisionNumberIncluded
    ) {
        return find(
                StressTestUsedCommitTable.RightCommitIdView.class,
                YqlPredicate.where("id.namespace").eq(namespace)
                        .and("id.leftRevisionNumber").gte(minLeftRevisionNumberIncluded),
                YqlPredicate.where("id.namespace").eq(namespace)
                        .and("id.leftRevisionNumber").lte(maxLeftRevisionNumberIncluded),
                YqlView.index(StressTestUsedCommitEntity.IDX_NAMESPACE_COMMIT_NUMBER)
        )
                .stream()
                .map(StressTestUsedCommitTable.RightCommitIdView::getRightCommitId)
                .collect(Collectors.toSet());
    }

    public List<StressTestUsedCommitEntity> findUsedRightRevisions(List<String> rightRevisions, String namespace) {
        return find(
                YqlPredicateCi.in("id.rightCommitId", rightRevisions)
                        .and(YqlPredicate.where("id.namespace").eq(namespace))
        );
    }

    @Value
    public static class RightCommitIdView implements Table.View {
        @Nonnull
        @Column(name = "id_rightCommitId")
        String rightCommitId;

        @Nonnull
        @Column(name = "id_namespace")
        String namespace;

        @Nonnull
        @Column(name = "id_leftRevisionNumber")
        long leftRevisionNumber;
    }

}
