package ru.yandex.ci.core.db.table;

import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.core.db.model.FavoriteProject;

public class FavoriteProjectTable extends KikimrTableCi<FavoriteProject> {

    public FavoriteProjectTable(QueryExecutor queryExecutor) {
        super(FavoriteProject.class, queryExecutor);
    }

    public static List<YqlStatementPart<?>> userFilter(String user, @Nullable ProjectFilter projectFilter) {
        var filter = filter(
                YqlPredicate.where("id.user").eq(user),
                YqlPredicateCi.in("mode", Set.of(FavoriteProject.Mode.SET, FavoriteProject.Mode.SET_AUTO))
        );
        if (projectFilter != null) {
            filter.add(
                    YqlPredicate.where("id.project").like(projectFilter.getFilter()).or(
                            YqlPredicate.where("id.project").in(projectFilter.getIncludeProjects()))
            );
        }
        return filter;
    }

    public List<String> listForUser(
            String user,
            @Nullable ProjectFilter projectFilter,
            @Nullable String offsetProject,
            int limit
    ) {
        var statementParts = filter(limit);
        statementParts.addAll(userFilter(user, projectFilter));

        if (offsetProject != null) {
            statementParts.add(YqlPredicate.where("id.project").gt(offsetProject));
        }
        statementParts.add(YqlOrderBy.orderBy("id.project"));
        return find(statementParts).stream()
                .map(FavoriteProject::getProject)
                .collect(Collectors.toList());
    }

    public long countForUser(String user, @Nullable ProjectFilter projectFilter) {
        return count(userFilter(user, projectFilter));
    }
}
