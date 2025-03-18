package ru.yandex.ci.observer.core.db.model.check;

import java.time.Instant;
import java.util.Arrays;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.yandex.ydb.ValueProtos;
import com.yandex.ydb.jdbc.impl.YdbFunctions;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.ViewSchema;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.statement.YqlStatement;
import yandex.cloud.repository.kikimr.yql.YqlPrimitiveType;
import yandex.cloud.repository.kikimr.yql.YqlType;
import ru.yandex.ci.core.db.CiYqlUtils;
import ru.yandex.ci.storage.core.CheckOuterClass;

public class ChecksCountStatement
        extends YqlStatement<ChecksCountStatement.Params, CheckEntity, ChecksCountStatement.CountByInterval> {

    private static final EntitySchema<CheckEntity> SCHEMA = EntitySchema.of(CheckEntity.class);
    private static final YqlPrimitiveType RIGHT_TIMESTAMP_FIELD = YqlType.of(SCHEMA.getField("right.timestamp"));
    private static final YqlPrimitiveType TYPE_FIELD = YqlType.of(SCHEMA.getField("type"));

    private final String intervalStartExpression;
    private final String toFieldExpression;
    private final String fromFieldExpression;
    private final String authorFilter;

    public ChecksCountStatement(
            Interval interval,
            boolean includeIncompleteInterval,
            AuthorFilter authorFilter
    ) {
        super(
                SCHEMA,
                ViewSchema.of(ChecksCountStatement.CountByInterval.class)
        );

        var truncateFunction = switch (interval) {
            case HOUR -> YdbFunctions.Udf.DateTimes.START_OF;
            case DAY -> YdbFunctions.Udf.DateTimes.START_OF_DAY;
            case WEEK -> YdbFunctions.Udf.DateTimes.START_OF_WEEK;
            case MONTH -> YdbFunctions.Udf.DateTimes.START_OF_MONTH;
            case QUARTER -> YdbFunctions.Udf.DateTimes.START_OF_QUARTER;
            case YEAR -> YdbFunctions.Udf.DateTimes.START_OF_YEAR;
        };

        intervalStartExpression = getTruncatedDateFunction(
                interval,
                truncateFunction,
                "IF(right_timestamp != cast('%s' as timestamp), right_timestamp, created)"
                        .formatted(Instant.ofEpochSecond(0))
        );
        fromFieldExpression = getTruncatedDateFunction(interval, truncateFunction, "$from");
        toFieldExpression = includeIncompleteInterval
                ? "$to"
                : getTruncatedDateFunction(interval, truncateFunction, "$to");

        this.authorFilter = switch (authorFilter) {
            case ALL -> "";
            case ONLY_ROBOTS -> getRobotCondition();
            case EXCLUDE_ROBOTS -> getNotRobotCondition();
        };
    }

    @Override
    public String getQuery(String tablespace, YqlVersion version) {
        var declarations = this.declarations(version);
        var table = this.table(tablespace);

        var countByType = Stream.of(CheckOuterClass.CheckType.values())
                .filter(it -> it != CheckOuterClass.CheckType.UNRECOGNIZED)
                // cast to uint32 cause AsDict(...) is converted to JSON and json can't store Long type value
                .map("AsTuple('%1$s', cast(COUNT_IF(type = '%1$s') as uint32))"::formatted)
                .collect(Collectors.joining(",\n"));

        var query = """
                DECLARE $from AS timestamp;
                DECLARE $to AS timestamp;
                DECLARE $types AS List<Utf8>;

                SELECT
                    AsDict(%s) AS countByType,
                """.formatted(countByType) + """
                    COUNT(*) AS count,
                    COUNT(DISTINCT author) AS authorCount,
                    intervalBegin
                FROM %s
                """.formatted(table) + """
                WHERE intervalBegin >= %s AND intervalBegin < %s
                    AND type in $types
                    %s
                """.formatted(fromFieldExpression, toFieldExpression, authorFilter) + """
                GROUP BY %s AS intervalBegin
                ORDER BY intervalBegin ASC
                """.formatted(intervalStartExpression);

        return declarations + "\n" + query;
    }

    private static String getRobotCondition() {
        return getRobotCondition("");
    }

    private static String getNotRobotCondition() {
        return getRobotCondition("NOT");
    }

    private static String getRobotCondition(String not) {
        return "AND (author " + not + " REGEXP '^(robot-.*|.*-robot)$')";
    }

    private static String getTruncatedDateFunction(Interval interval, String truncateFunction,
                                                   String timestampField) {
        return interval == Interval.HOUR
                ? "DateTime::MakeTimestamp(%s(%s, Interval(\"PT1H\")))".formatted(truncateFunction, timestampField)
                : "DateTime::MakeTimestamp(%s(%s))".formatted(truncateFunction, timestampField);
    }

    @Override
    public Map<String, ValueProtos.TypedValue> toQueryParameters(Params params) {
        return Map.of(
                "$from", CiYqlUtils.typedValue(params.getFrom(), RIGHT_TIMESTAMP_FIELD),
                "$to", CiYqlUtils.typedValue(params.getTo(), RIGHT_TIMESTAMP_FIELD),
                "$types", CiYqlUtils.listTypedValue(params.getTypes(), TYPE_FIELD)
        );
    }

    @Override
    public String toDebugString(Params params) {
        return this.getClass().getSimpleName();
    }

    @Override
    public QueryType getQueryType() {
        return QueryType.SELECT;
    }


    @Value(staticConstructor = "of")
    public static class Params {
        Instant from;
        Instant to;
        @Nullable
        Set<CheckOuterClass.CheckType> types;

        public Set<CheckOuterClass.CheckType> getTypes() {
            return Objects.requireNonNullElseGet(types, () ->
                    Arrays.stream(CheckOuterClass.CheckType.values())
                            .filter(it -> it != CheckOuterClass.CheckType.UNRECOGNIZED)
                            .collect(Collectors.toUnmodifiableSet())
            );
        }
    }

    @Value(staticConstructor = "of")
    public static class CountByInterval implements yandex.cloud.repository.db.Table.View {
        @Column(name = "intervalBegin", dbType = DbType.TIMESTAMP)
        Instant intervalBegin;

        @Column(name = "countByType", dbType = DbType.JSON, flatten = false)
        Map<CheckOuterClass.CheckType, Integer> countByType;

        @Column(name = "count", dbType = DbType.UINT64)
        long checkCount; // Count should has UINT64 type. When UINT32, database returns zero instead of actual value

        @Column(name = "authorCount", dbType = DbType.UINT64)
        long authorCount; // Count should has UINT64 type. When UINT32, database returns zero instead of actual value
    }

    public enum Interval {
        HOUR,
        DAY,
        WEEK,
        MONTH,
        QUARTER,
        YEAR
    }

    public enum AuthorFilter {
        EXCLUDE_ROBOTS,
        ONLY_ROBOTS,
        ALL
    }

}
