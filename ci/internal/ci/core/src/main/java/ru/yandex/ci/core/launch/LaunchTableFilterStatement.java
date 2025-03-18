package ru.yandex.ci.core.launch;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.yandex.ydb.ValueProtos;
import lombok.Value;
import org.apache.commons.text.StringSubstitutor;

import yandex.cloud.binding.schema.Schema;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.statement.ResultSetReader;
import yandex.cloud.repository.kikimr.statement.Statement;
import yandex.cloud.repository.kikimr.yql.YqlType;
import yandex.cloud.repository.kikimr.yql.YqlUtils;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiYqlUtils;
import ru.yandex.lang.NonNullApi;

import static java.util.stream.Collectors.joining;

@NonNullApi
class LaunchTableFilterStatement implements Statement<LaunchTableFilter, Launch> {

    private static final EntitySchema<Launch> LAUNCH_SCHEMA =
            EntitySchema.of(Launch.class);
    private static final EntitySchema<LaunchByProcessIdAndPinned> LAUNCH_BY_PROCESS_PINNED =
            EntitySchema.of(LaunchByProcessIdAndPinned.class);
    private static final EntitySchema<LaunchByProcessIdAndArcBranch> LAUNCH_BY_PROCESS_BRANCH =
            EntitySchema.of(LaunchByProcessIdAndArcBranch.class);
    private static final EntitySchema<LaunchByProcessIdAndTag> LAUNCH_BY_PROCESS_TAG =
            EntitySchema.of(LaunchByProcessIdAndTag.class);
    private static final EntitySchema<LaunchByProcessIdAndStatus> LAUNCH_BY_PROCESS_STATUS =
            EntitySchema.of(LaunchByProcessIdAndStatus.class);

    private static final YqlType STATUS_FIELD_IN_STATUS_PROJECTION = YqlType.of(
            LAUNCH_BY_PROCESS_STATUS.getField(LaunchByProcessIdAndStatus.STATUS_FIELD)
    );
    private static final YqlType TAG_FIELD_IN_TAG_PROJECTION = YqlType.of(
            LAUNCH_BY_PROCESS_TAG.getField(LaunchByProcessIdAndTag.TAG_FIELD)
    );

    private final Map<String, ValueProtos.TypedValue> yqlParams;
    private final String yqlQuery;
    private final ResultSetReader<Launch> resultSetReader;


    LaunchTableFilterStatement(
            CiProcessId processId,
            LaunchTableFilter filter,
            int offsetLaunchNumber,
            int limit
    ) {
        if (offsetLaunchNumber <= 0) {
            offsetLaunchNumber = switch (filter.getSortDirection()) {
                case DESC -> Integer.MAX_VALUE;
                case ASC -> 0;
                default -> throw new IllegalStateException();
            };
        }
        if (limit <= 0) {
            limit = YdbUtils.RESULT_ROW_LIMIT;
        }

        var declaration = buildDeclarations(processId, offsetLaunchNumber, limit, filter);
        var orderBy = switch (filter.getSortBy()) {
            case NUMBER -> "launchNumber";
            case UNRECOGNIZED -> throw new IllegalArgumentException("Unsupported sort by" + filter.getSortBy());
        };
        var direction = filter.getSortDirection().name();
        yqlQuery = StringSubstitutor.replace(
                """
                        ${query}
                        SELECT ${columns}
                        FROM (
                            ${nested} ORDER BY processId ${direction}, ${orderBy} ${direction}
                            LIMIT $limit
                        ) AS filtered
                        JOIN `${table}` AS main
                          ON main.processId = filtered.processId
                            AND main.launchNumber = filtered.launchNumber
                        ORDER BY `processId` ${direction}, `${orderBy}` ${direction}
                        LIMIT $limit
                        """,
                Map.of(
                        "query", declaration.yqlQuery,
                        "columns", outNames(LAUNCH_SCHEMA, "main"),
                        "nested", buildNestedStatement(filter),
                        "table", LAUNCH_SCHEMA.getName(),
                        "orderBy", orderBy,
                        "direction", direction
                ));

        this.yqlParams = declaration.yqlParams;
        this.resultSetReader = new ResultSetReader<>(LAUNCH_SCHEMA);
    }


    @Override
    public String getQuery(String tablespace, YqlVersion version) {
        return YqlUtils.withTablePathPrefix(tablespace, yqlQuery);
    }

    @Override
    public String toDebugString(LaunchTableFilter launchTableFilter) {
        return yqlQuery;
    }

    @Override
    public Map<String, ValueProtos.TypedValue> toQueryParameters(LaunchTableFilter filter) {
        return yqlParams;
    }

    @Override
    public Launch readResult(List<ValueProtos.Column> columns, ValueProtos.Value value) {
        return resultSetReader.readResult(columns, value);
    }

    @Override
    public QueryType getQueryType() {
        return QueryType.SELECT;
    }


    private static Declaration buildDeclarations(
            CiProcessId processId,
            int offsetLaunchNumber,
            int limit,
            LaunchTableFilter filter
    ) {
        StringBuilder declareStmt = new StringBuilder();

        declareStmt.append("""
                DECLARE $process_id AS Utf8;
                DECLARE $launch_number AS Int32;
                DECLARE $limit AS Int32;
                """
        );

        var yqlParams = new HashMap<String, ValueProtos.TypedValue>();
        yqlParams.put("$process_id", CiYqlUtils.utf8TypedValue(processId.asString()));
        yqlParams.put("$launch_number", YqlUtils.value(Integer.class, offsetLaunchNumber));
        yqlParams.put("$limit", YqlUtils.value(Integer.class, limit));

        if (filter.getPinned() != null) {
            declareStmt.append("DECLARE $pinned AS Bool;\n");
            yqlParams.put("$pinned", YqlUtils.value(Boolean.class, filter.getPinned()));
        }
        if (filter.getBranch() != null) {
            declareStmt.append("DECLARE $branch AS Utf8;\n");
            yqlParams.put("$branch", CiYqlUtils.utf8TypedValue(filter.getBranch()));
        }

        if (!filter.getTags().isEmpty()) {
            if (filter.getTags().size() > 1) {
                declareStmt.append("DECLARE $tags AS List<Utf8>;\n");
                yqlParams.put("$tags", CiYqlUtils.listTypedValue(filter.getTags(), TAG_FIELD_IN_TAG_PROJECTION));
            } else {
                declareStmt.append("DECLARE $tags AS Utf8;\n");
                yqlParams.put("$tags", CiYqlUtils.utf8TypedValue(filter.getTags().get(0)));
            }
        }

        if (!filter.getStatuses().isEmpty()) {
            if (filter.getStatuses().size() > 1) {
                declareStmt.append("DECLARE $statuses AS List<Utf8>;\n");
                yqlParams.put("$statuses", CiYqlUtils.listTypedValue(filter.getStatuses(),
                        STATUS_FIELD_IN_STATUS_PROJECTION));
            } else {
                declareStmt.append("DECLARE $statuses AS Utf8;\n");
                yqlParams.put("$statuses", CiYqlUtils.typedValue(filter.getStatuses().get(0),
                        STATUS_FIELD_IN_STATUS_PROJECTION));
            }
        }

        return new Declaration(yqlParams, declareStmt);
    }

    private static StringBuilder buildNestedStatement(LaunchTableFilter filter) {
        StringBuilder bodyStmt = new StringBuilder();
        StringBuilder whereStmt = new StringBuilder();

        var launchCondition = switch (filter.getSortDirection()) {
            case DESC -> "<";
            case ASC -> ">";
            default -> throw new IllegalStateException();
        };
        String lastUsedTableAlias = null;
        if (filter.getPinned() != null) {
            bodyStmt.append("""
                      SELECT t1.idx_processId AS processId, t1.idx_launchNumber AS launchNumber
                      FROM `%s` AS t1
                    """.formatted(LAUNCH_BY_PROCESS_PINNED.getName()));
            whereStmt.append("""
                      WHERE t1.idx_processId = $process_id
                        AND t1.idx_pinned = $pinned
                        AND t1.idx_launchNumber %s $launch_number
                    """.formatted(launchCondition));
            lastUsedTableAlias = "t1";
        }

        if (filter.getBranch() != null) {
            if (lastUsedTableAlias == null) {
                bodyStmt.append("""
                          SELECT t2.idx_processId AS processId, t2.idx_launchNumber AS launchNumber
                          FROM `%s` AS t2
                        """.formatted(LAUNCH_BY_PROCESS_BRANCH.getName())
                );
                whereStmt.append("""
                          WHERE t2.idx_processId = $process_id
                            AND t2.idx_branch = $branch
                            AND t2.idx_launchNumber %s $launch_number
                        """.formatted(launchCondition));
            } else {
                bodyStmt.append("  JOIN `%s` AS t2 ON ".formatted(LAUNCH_BY_PROCESS_BRANCH.getName()))
                        .append(joinConditions(lastUsedTableAlias, "t2"));
                whereStmt.append("    AND t2.idx_branch = $branch\n");
            }
            lastUsedTableAlias = "t2";
        }

        if (!filter.getTags().isEmpty()) {
            /* If we have table with primary key (processId, tag) and have sql query containing 'IN",
              then YDB doesn't use 'tag' from index. Example: https://paste.yandex-team.ru/2637585
              But YDB use 'tag' field from index when we use '=' instead of 'IN': https://paste.yandex-team.ru/2637651.

              Using `FROM_AS($tags) JOIN LaunchByProcessIdAndTag ON ...` also doesn't helps, when
              we have complex query like:
              SELECT *
              FROM (
                SELECT *
                FROM_AS($tags)
                JOIN LaunchByProcessIdAndTag ON ...
              ) AS t
              ...
              */
            String eitherOrEitherInOperator = filter.getTags().size() == 1 ? "=" : "IN";
            if (lastUsedTableAlias == null) {
                bodyStmt.append("""
                          SELECT t3.idx_processId AS processId, t3.idx_launchNumber AS launchNumber
                          FROM `%s` AS t3
                        """.formatted(LAUNCH_BY_PROCESS_TAG.getName())
                );
                whereStmt.append("""
                          WHERE t3.idx_processId = $process_id
                            AND t3.idx_tag %s $tags
                            AND t3.idx_launchNumber %s $launch_number
                        """.formatted(eitherOrEitherInOperator, launchCondition)
                );
            } else {
                bodyStmt.append("  JOIN `%s` AS t3 ON ".formatted(LAUNCH_BY_PROCESS_TAG.getName()))
                        .append(joinConditions(lastUsedTableAlias, "t3"));
                whereStmt.append("    AND t3.idx_tag ").append(eitherOrEitherInOperator).append(" $tags\n");
            }
            lastUsedTableAlias = "t3";
        }

        if (!filter.getStatuses().isEmpty()) {
            String eitherOrEitherInOperator = filter.getStatuses().size() == 1 ? "=" : "IN";
            if (lastUsedTableAlias == null) {
                bodyStmt.append("""
                          SELECT t4.idx_processId AS processId, t4.idx_launchNumber AS launchNumber
                          FROM `%s` AS t4
                        """.formatted(LAUNCH_BY_PROCESS_STATUS.getName())
                );
                whereStmt.append("""
                          WHERE t4.idx_processId = $process_id
                            AND t4.idx_status %s $statuses
                            AND t4.idx_launchNumber %s $launch_number
                        """.formatted(eitherOrEitherInOperator, launchCondition)
                );
            } else {
                bodyStmt.append("  JOIN `%s` AS t4 ON ".formatted(LAUNCH_BY_PROCESS_STATUS.getName()))
                        .append(joinConditions(lastUsedTableAlias, "t4"));
                whereStmt.append("    AND t4.idx_status ").append(eitherOrEitherInOperator).append(" $statuses\n");
            }
            lastUsedTableAlias = "t4";
        }

        if (lastUsedTableAlias != null) {
            return bodyStmt.append(whereStmt);
        }

        return new StringBuilder().append("""
                  SELECT t1.processId AS processId, t1.launchNumber AS launchNumber
                  FROM `%s` AS t1
                  WHERE t1.processId = $process_id AND t1.launchNumber %s $launch_number
                """.formatted(LAUNCH_SCHEMA.getName(), launchCondition));
    }

    /**
     * Should be used in `SELECT`-statement instead of `*`
     * otherwise {@link ResultSetReader} won't be able to parse result.
     * <p>
     * The other way is creation {@link EntitySchema}
     * with own {@link yandex.cloud.binding.schema.naming.NamingStrategy}
     * but almost all implementations of it use deprecated
     * {@link yandex.cloud.binding.schema.Schema.JavaField#setName(String)}
     */
    private static String outNames(EntitySchema<Launch> schema, String tableAlias) {
        return schema.flattenFields().stream()
                .map(Schema.JavaField::getName)
                .map(CiYqlUtils::escapeFieldName)
                .map(escapedFieldName -> tableAlias + "." + escapedFieldName + " AS " + escapedFieldName)
                .collect(joining(", "));
    }

    private static String joinConditions(String sourceTableAlias, String joinedTableAlias) {
        return String.format("""
                        %1$s.idx_processId = %2$s.idx_processId
                            AND %1$s.idx_launchNumber = %2$s.idx_launchNumber
                        """,
                sourceTableAlias, joinedTableAlias
        );
    }

    @Value
    private static class Declaration {
        Map<String, ValueProtos.TypedValue> yqlParams;
        StringBuilder yqlQuery;
    }

}
