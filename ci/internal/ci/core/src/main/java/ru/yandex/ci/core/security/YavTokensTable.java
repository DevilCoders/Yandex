package ru.yandex.ci.core.security;

import java.nio.file.Path;
import java.util.Optional;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.arc.ArcRevision;

public class YavTokensTable extends KikimrTableCi<YavToken> {

    public YavTokensTable(QueryExecutor executor) {
        super(YavToken.class, executor);
    }

    public Optional<YavToken> findExisting(Path configPath, ArcRevision revision) {
        return single(
                find(YqlPredicate
                                .where("configPath").eq(configPath.toString())
                                .and("delegationCommitId").eq(revision.getCommitId()),
                        YqlView.index(YavToken.IDX_PATH_COMMIT),
                        YqlOrderBy.orderBy("created", YqlOrderBy.SortOrder.DESC),
                        YqlLimit.top(1)
                ));
    }
}
