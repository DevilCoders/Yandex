package ru.yandex.ci.observer.core.db.model.check;

import java.time.Instant;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.Value;
import lombok.With;

import yandex.cloud.repository.db.Table;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;

public class CheckTable extends KikimrTableCi<CheckEntity> {
    public CheckTable(QueryExecutor executor) {
        super(CheckEntity.class, executor);
    }

    public List<CheckEntity> find(String leftRevision, String rightRevision) {
        return find(leftRevision, rightRevision, Set.of());
    }

    public List<CheckEntity> find(String leftRevision, String rightRevision, Set<String> tags) {
        return find(
                YqlPredicate
                        .where("left.revision").eq(leftRevision)
                        .and("right.revision").eq(rightRevision),
                YqlView.index(CheckEntity.IDX_BY_REVISIONS)
        )
                .stream()
                .filter(check -> check.getTags().containsAll(tags))
                .collect(Collectors.toList());
    }

    public long countActive() {
        return this.count(
                CheckStatusUtils.getIsActive("status"),
                YqlView.index(CheckEntity.IDX_BY_STATUS_AND_CREATED)
        );
    }

    public Long count(Common.CheckStatus status, Instant after) {
        return this.count(
                YqlPredicate
                        .where("status").eq(status)
                        .and("created").gte(after),
                YqlView.index(CheckEntity.IDX_BY_STATUS_AND_CREATED)
        );
    }

    public List<CheckTable.RevisionsView> findAll(long gteLeftRevisionNumber, long limit, long offset) {
        var top = YqlLimit.top(limit);
        if (offset > 0) {
            top = top.withOffset(offset);
        }
        return find(
                CheckTable.RevisionsView.class,
                YqlPredicate.where("type").eq(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
                        .and("left.branch").eq(ArcBranch.trunk().getBranch())
                        .and("left.revisionNumber").gte(gteLeftRevisionNumber),
                top
        );
    }

    public List<ChecksCountStatement.CountByInterval> count(
            Instant from,
            Instant to,
            ChecksCountStatement.Interval interval,
            boolean includeIncompleteInterval,
            ChecksCountStatement.AuthorFilter authorFilter
    ) {
        var statement = new ChecksCountStatement(interval, includeIncompleteInterval, authorFilter);
        return executeOnView(statement, ChecksCountStatement.Params.of(from, to, null));
    }

    @Value
    @SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
    public static class RevisionsView implements Table.View {
        @Nonnull
        CheckEntity.Id id;

        @Nonnull
        StorageRevision left;

        @With
        @Nonnull
        StorageRevision right;

        @Nonnull
        Long diffSetId;

        public static RevisionsView of(CheckEntity check) {
            return new RevisionsView(check.getId(), check.getLeft(), check.getRight(), check.getDiffSetId());
        }
    }

}
