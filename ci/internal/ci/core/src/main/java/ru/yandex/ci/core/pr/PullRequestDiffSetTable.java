package ru.yandex.ci.core.pr;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;

public class PullRequestDiffSetTable extends KikimrTableCi<PullRequestDiffSet> {

    public PullRequestDiffSetTable(QueryExecutor executor) {
        super(PullRequestDiffSet.class, executor);
    }

    public Optional<PullRequestDiffSet> findById(long pullRequestId, long diffSetId) {
        return find(PullRequestDiffSet.Id.of(pullRequestId, diffSetId));
    }

    public PullRequestDiffSet getById(long pullRequestId, long diffSetId) {
        return get(PullRequestDiffSet.Id.of(pullRequestId, diffSetId));
    }

    public List<PullRequestDiffSet> findAllWithStatus(long pullRequestId, Set<PullRequestDiffSet.Status> statuses) {
        return find(List.of(
                YqlPredicate.where("id.pullRequestId").eq(pullRequestId),
                YqlPredicateCi.in("status", statuses),
                YqlOrderBy.orderBy("id.diffSetId", YqlOrderBy.SortOrder.DESC)));
    }

    public Optional<PullRequestDiffSet> findLatestByPullRequestId(long pullRequestId) {
        List<PullRequestDiffSet> result = find(List.of(
                YqlPredicate.where("id.pullRequestId").eq(pullRequestId),
                YqlOrderBy.orderBy("id.diffSetId", YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1)
        ));
        return result.isEmpty() ? Optional.empty() : Optional.of(result.get(0));
    }

    public List<Long> suggestPullRequestId(@Nullable Long idPrefix, int offset, int limit) {
        List<YqlPredicate> pullRequestIdParts = new ArrayList<>();
        if (idPrefix != null) {
            long leftBoundary = idPrefix;
            long rightBoundary = idPrefix + 1;

            for (long power = 1; power < Integer.MAX_VALUE; power *= 10) {
                /* WHERE (pullRequestId >= 11 AND pullRequestId < 12) OR
                        (pullRequestId >= 110 AND pullRequestId < 120) OR
                        (pullRequestId >= 1100 AND pullRequestId < 1200) OR
                    ... */
                pullRequestIdParts.add(
                        YqlPredicate.where("id.pullRequestId").gte(leftBoundary * power)
                                .and(YqlPredicate.where("id.pullRequestId").lt(rightBoundary * power))
                );
            }
        }

        var parts = filter(limit, offset);
        if (pullRequestIdParts.size() > 0) {
            parts.add(YqlPredicate.or(pullRequestIdParts));
        }
        parts.add(YqlOrderBy.orderBy("id.pullRequestId", YqlOrderBy.SortOrder.ASC));

        return byPullRequestId().find(parts)
                .stream()
                .map(PullRequestDiffSet.ByPullRequestId::getId)
                .map(PullRequestDiffSet.ByPullRequestId.Id::getPullRequestId)
                .toList();
    }

    @Override
    public void deleteAll() {
        byPullRequestId().deleteAll();
        super.deleteAll();
    }

    @Override
    public PullRequestDiffSet save(PullRequestDiffSet diffSet) {
        byPullRequestId().save(PullRequestDiffSet.ByPullRequestId.of(diffSet));
        return super.save(diffSet);
    }

    public KikimrTable<PullRequestDiffSet.ByPullRequestId> byPullRequestId() {
        return new KikimrTable<>(PullRequestDiffSet.ByPullRequestId.class, executor);
    }

    @Value
    public static class PullRequestIdView implements View {
        @Column(name = "pullRequestId")
        long pullRequestId;
    }
}
