package ru.yandex.ci.storage.core.db.model.test_diff;

import java.util.Collection;
import java.util.List;

import yandex.cloud.binding.schema.Schema;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.statement.PredicateStatement;
import yandex.cloud.repository.kikimr.statement.Statement;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_text_search.CheckTextSearchEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.index.TestDiffBySuiteEntity;

public class DiffsStatement {
    private static final EntitySchema<CheckTextSearchEntity> SEARCH_SCHEMA = EntitySchema.of(
            CheckTextSearchEntity.class
    );

    private static final EntitySchema<TestDiffBySuiteEntity> BY_SUITE_SCHEMA = EntitySchema.of(
            TestDiffBySuiteEntity.class
    );

    private DiffsStatement() {

    }

    public static <
            ENTITY extends Entity<ENTITY>
            > Statement<Collection<? extends YqlStatementPart<?>>, ENTITY> diffTextSearch(
            Class<ENTITY> type,
            YqlStatementPart<?> filter,
            YqlStatementPart<?> orderBy,
            YqlStatementPart<?> limit,
            Common.CheckSearchEntityType searchEntityType,
            String value
    ) {
        var schema = EntitySchema.of(type);

        return diffTextSearch(
                schema, schema, filter, orderBy, limit, searchEntityType, escapeText(value)
        );
    }

    private static <
            ENTITY extends Entity<ENTITY>, RESULT
            > Statement<Collection<? extends YqlStatementPart<?>>, RESULT> diffTextSearch(
            EntitySchema<ENTITY> schema,
            Schema<RESULT> resultSchema,
            YqlStatementPart<?> filter,
            YqlStatementPart<?> orderBy,
            YqlStatementPart<?> limit,
            Common.CheckSearchEntityType searchEntityType,
            String testName
    ) {
        var parts = List.of(filter, orderBy, limit);

        return new PredicateStatement<>(schema, resultSchema, parts, DiffsStatement::predicateFrom) {
            @Override
            public String getQuery(String tablespace, YqlVersion version) {
                return declarations(version) + "\n" +
                        "PRAGMA AnsiInForEmptyOrNullableItemsCollections;\n" +
                        "$suiteIds = select DISTINCT(id_suiteId)\n" +
                        "FROM " + escape(tablespace + SEARCH_SCHEMA.getName()) + "\n" +
                        "WHERE  `id_checkId` = $pred_0_id_checkId\n" +
                        "AND `id_iterationType` = $pred_1_id_iterationType\n" +
                        "AND `id_entityType` = " + searchEntityType.getNumber() + "\n" +
                        "AND `id_value` = '" + testName + "'u;\n" +
                        "$diffs = select (id_checkId, id_iterationType, id_resultType, id_toolchain, id_path, " +
                        "id_suiteId, id_testId, id_iterationNumber)\n" +
                        "FROM " + escape(tablespace + BY_SUITE_SCHEMA.getName()) + "\n" +
                        "WHERE id_checkId = $pred_0_id_checkId\n" +
                        "AND `id_iterationType` = $pred_1_id_iterationType\n" +
                        "AND id_toolchain = $pred_2_id_toolchain\n" +
                        "AND id_suiteId IN $suiteIds\n" +
                        "AND id_testId = id_suiteId;\n" +
                        "\n" +
                        "SELECT " + outNames() + "\n" +
                        "FROM " + table(tablespace) + "\n" +
                        "WHERE (id_checkId, id_iterationType, id_resultType, id_toolchain, " +
                        "id_path, id_suiteId, id_testId, id_iterationNumber) IN $diffs\n" +
                        "AND " + this.resolveParamNames(filter.toYql(schema)) + "\n" +
                        this.resolveParamNames(orderBy.toFullYql(schema)) + "\n" +
                        this.resolveParamNames(limit.toFullYql(schema));
            }

            @Override
            public QueryType getQueryType() {
                return QueryType.SELECT;
            }

            @Override
            public String toDebugString(Collection<? extends YqlStatementPart<?>> yqlStatementParts) {
                return "diffTextSearch(" + yqlStatementParts + ")";
            }
        };
    }

    public static <
            ENTITY extends Entity<ENTITY>, JOIN_ENTITY extends Entity<JOIN_ENTITY>
            > Statement<Collection<? extends YqlStatementPart<?>>, ENTITY> diffTextSearch(
            Class<ENTITY> type,
            Class<JOIN_ENTITY> joinType,
            YqlStatementPart<?> filter,
            YqlStatementPart<?> orderBy,
            YqlStatementPart<?> limit,
            String testName,
            String subtestName
    ) {
        var schema = EntitySchema.of(type);
        return diffTextSearch(
                schema, schema, filter, orderBy, limit,
                escapeText(testName), escapeText(subtestName)
        );
    }

    private static <
            ENTITY extends Entity<ENTITY>, RESULT
            > Statement<Collection<? extends YqlStatementPart<?>>, RESULT> diffTextSearch(
            EntitySchema<ENTITY> schema,
            Schema<RESULT> resultSchema,
            YqlStatementPart<?> filter,
            YqlStatementPart<?> orderBy,
            YqlStatementPart<?> limit,
            String testName,
            String subtestName
    ) {
        var parts = List.of(filter, orderBy, limit);

        return new PredicateStatement<>(schema, resultSchema, parts, DiffsStatement::predicateFrom) {
            @Override
            public String getQuery(String tablespace, YqlVersion version) {
                return declarations(version) + "\n" +
                        "PRAGMA AnsiInForEmptyOrNullableItemsCollections;\n" +
                        "$suiteIdsFirst = select DISTINCT(id_suiteId)\n" +
                        "FROM " + escape(tablespace + SEARCH_SCHEMA.getName()) + "\n" +
                        "WHERE  `id_checkId` = $pred_0_id_checkId\n" +
                        "AND `id_iterationType` = $pred_1_id_iterationType\n" +
                        "AND `id_entityType` = " + Common.CheckSearchEntityType.CSET_TEST_NAME.getNumber() + "\n" +
                        "AND `id_value` = '" + testName + "'u;\n" +
                        "$suiteIds = select DISTINCT(id_suiteId)\n" +
                        "FROM " + escape(tablespace + SEARCH_SCHEMA.getName()) + "\n" +
                        "WHERE  `id_checkId` = $pred_0_id_checkId\n" +
                        "AND `id_iterationType` = $pred_1_id_iterationType\n" +
                        "AND `id_entityType` = " + Common.CheckSearchEntityType.CSET_SUBTEST_NAME.getNumber() + "\n" +
                        "AND `id_value` = '" + subtestName + "'u\n" +
                        "AND id_suiteId IN $suiteIdsFirst;\n" +
                        "$diffs = select (id_checkId, id_iterationType, id_resultType, id_toolchain, id_path, " +
                        "id_suiteId, id_testId, id_iterationNumber)\n" +
                        "FROM " + escape(tablespace + BY_SUITE_SCHEMA.getName()) + "\n" +
                        "WHERE id_checkId = $pred_0_id_checkId\n" +
                        "AND `id_iterationType` = $pred_1_id_iterationType\n" +
                        "AND id_toolchain = $pred_2_id_toolchain\n" +
                        "AND id_suiteId IN $suiteIds\n" +
                        "AND id_testId = id_suiteId;\n" +
                        "\n" +
                        "SELECT " + outNames() + "\n" +
                        "FROM " + table(tablespace) + "\n" +
                        "WHERE (id_checkId, id_iterationType, id_resultType, id_toolchain, " +
                        "id_path, id_suiteId, id_testId, id_iterationNumber) IN $diffs\n" +
                        "AND " + this.resolveParamNames(filter.toYql(schema)) + "\n" +
                        this.resolveParamNames(orderBy.toFullYql(schema)) + "\n" +
                        this.resolveParamNames(limit.toFullYql(schema));
            }

            @Override
            public QueryType getQueryType() {
                return QueryType.SELECT;
            }

            @Override
            public String toDebugString(Collection<? extends YqlStatementPart<?>> yqlStatementParts) {
                return "diffTextSearch(" + yqlStatementParts + ")";
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
        return value.replace("'", "");
    }
}
