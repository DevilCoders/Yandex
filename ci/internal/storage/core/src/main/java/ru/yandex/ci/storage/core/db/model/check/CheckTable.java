package ru.yandex.ci.storage.core.db.model.check;

import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;

import static ru.yandex.ci.storage.core.CheckOuterClass.CheckType.BRANCH_POST_COMMIT;
import static ru.yandex.ci.storage.core.CheckOuterClass.CheckType.TRUNK_POST_COMMIT;

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

    public Long count(CheckStatus status, Instant after) {
        return this.count(
                YqlPredicate
                        .where("status").eq(status)
                        .and("created").gte(after),
                YqlView.index(CheckEntity.IDX_BY_STATUS_AND_CREATED)
        );
    }

    public List<CheckEntity> findPostCommits(String rightRevision) {
        return find(
                YqlPredicate
                        .where("right.revision").eq(rightRevision)
                        .and("type").in(Set.of(TRUNK_POST_COMMIT, BRANCH_POST_COMMIT)),
                YqlView.index(CheckEntity.IDX_BY_RIGHT_REVISION)
        );
    }

    public List<CheckEntity> getInArchiveState(
            CheckOuterClass.ArchiveState state, Instant createdTill, int limit
    ) {
        if (limit <= 0) {
            return List.of();
        }

        return this.find(
                YqlPredicate.where("archiveState").eq(state).and("created").lte(createdTill),
                YqlView.index(CheckEntity.IDX_BY_ARCHIVE_STATE_AND_CREATED),
                YqlOrderBy.orderBy("archiveState", "created"),
                YqlLimit.top(limit)
        );
    }

    public Optional<CheckEntity> findLastCheckByPullRequest(long pullRequestId) {
        return this.find(
                YqlPredicate.where("pullRequestId").eq(pullRequestId),
                YqlView.index(CheckEntity.IDX_BY_PULL_REQUEST_ID_AND_CREATED),
                YqlOrderBy.orderBy(
                        new YqlOrderBy.SortKey("pullRequestId", YqlOrderBy.SortOrder.DESC),
                        new YqlOrderBy.SortKey("created", YqlOrderBy.SortOrder.DESC)
                ),
                YqlLimit.top(1)
        ).stream().findFirst();
    }
}
