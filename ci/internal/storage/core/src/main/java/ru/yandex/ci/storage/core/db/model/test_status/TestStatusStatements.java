package ru.yandex.ci.storage.core.db.model.test_status;


import java.util.Collection;
import java.util.List;

import javax.annotation.Nullable;

import com.google.common.primitives.UnsignedLongs;

import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.statement.PredicateStatement;
import yandex.cloud.repository.kikimr.statement.Statement;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;

@SuppressWarnings("UnstableApiUsage")
public class TestStatusStatements {
    private static final EntitySchema<TestStatusEntity> SCHEMA = EntitySchema.of(TestStatusEntity.class);

    private TestStatusStatements() {

    }

    public static Statement<Collection<? extends YqlStatementPart<?>>, TestStatusEntity> search(
            String index,
            YqlStatementPart<?> filter,
            YqlStatementPart<?> orderBy,
            YqlStatementPart<?> limit,
            @Nullable TestSearch.Paging paging
    ) {
        var parts = List.of(filter, orderBy, limit);

        return new PredicateStatement<>(SCHEMA, SCHEMA, parts, TestStatusStatements::predicateFrom) {
            @Override
            public String getQuery(String tablespace, YqlVersion version) {
                var paramNames = this.resolveParamNames(filter.toYql(schema));
                if (paging == null) {
                    return declarations(version) + "\n" +
                            "SELECT " + outNames() + "\n" +
                            "FROM " + table(tablespace) + " VIEW " + index + " \n" +
                            "WHERE " + paramNames + "\n" +
                            this.resolveParamNames(orderBy.toFullYql(schema)) + "\n" +
                            this.resolveParamNames(limit.toFullYql(schema));
                }

                var testComparator = paging.isAscending() ? " >= " : " < ";
                var pathComparator = paging.isAscending() ? " > " : " < ";
                var path = escapeText(paging.getPage().getPath());
                var testId = UnsignedLongs.toString(paging.getPage().getTestId());
                var orderAndLimit = this.resolveParamNames(orderBy.toFullYql(schema)) + "\n" +
                        this.resolveParamNames(limit.toFullYql(schema)) + ";\n";

                return declarations(version) + "\n" +
                        "$first = SELECT " + outNames() + "\n" +
                        "FROM " + table(tablespace) + " VIEW `" + index + "` \n" +
                        "WHERE " + paramNames + "\n" +
                        "AND `path` = " + path + " AND `id_testId` " + testComparator + testId + "\n" +
                        orderAndLimit +
                        "$second= SELECT " + outNames() + "\n" +
                        "FROM " + table(tablespace) + " VIEW `" + index + "` \n" +
                        "WHERE " + paramNames + "\n" +
                        "AND `path`" + pathComparator + path + "\n" +
                        orderAndLimit +
                        "$union = (select * from $first UNION ALL select * from $second);\n" +
                        "select * from $union\n" + orderAndLimit;
            }

            @Override
            public QueryType getQueryType() {
                return QueryType.SELECT;
            }

            @Override
            public String toDebugString(Collection<? extends YqlStatementPart<?>> yqlStatementParts) {
                return "testStatusSearch(" + yqlStatementParts + ")";
            }
        };
    }

    protected static YqlPredicate predicateFrom(Collection<? extends YqlStatementPart<?>> parts) {
        return parts.stream()
                .filter(p -> p instanceof YqlPredicate)
                .map(YqlPredicate.class::cast)
                .reduce(YqlPredicate.alwaysTrue(), (p1, p2) -> p1.and(p2));
    }

    protected static String escapeText(String value) {
        return "'" + value.replace("'", "") + "'u";
    }
}
