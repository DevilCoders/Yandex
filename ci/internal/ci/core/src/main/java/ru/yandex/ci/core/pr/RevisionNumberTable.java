package ru.yandex.ci.core.pr;

import java.util.List;
import java.util.Optional;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.CommitId;

public class RevisionNumberTable extends KikimrTableCi<RevisionNumber> {

    public RevisionNumberTable(QueryExecutor executor) {
        super(RevisionNumber.class, executor);
    }

    public Optional<RevisionNumber> findById(ArcBranch branch, CommitId commitId) {
        return find(RevisionNumber.Id.of(branch.asString(), commitId.getCommitId()));
    }

    public Optional<RevisionNumber> findLastKnown(ArcBranch branch) {
        List<YqlStatementPart<?>> statementParts =
                List.of(YqlPredicate.where("id.branch").eq(branch.asString()),
                        YqlOrderBy.orderBy("number", YqlOrderBy.SortOrder.DESC),
                        YqlLimit.top(1));

        return find(statementParts).stream().findFirst();
    }
}
