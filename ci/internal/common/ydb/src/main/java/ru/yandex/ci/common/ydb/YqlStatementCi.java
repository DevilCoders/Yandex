package ru.yandex.ci.common.ydb;

import java.util.Collection;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.yandex.ydb.ValueProtos;

import yandex.cloud.binding.expression.OrderExpression;
import yandex.cloud.binding.schema.ObjectSchema;
import yandex.cloud.binding.schema.Schema;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.db.ViewSchema;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.statement.Count;
import yandex.cloud.repository.kikimr.statement.FindInStatement;
import yandex.cloud.repository.kikimr.statement.PredicateStatement;
import yandex.cloud.repository.kikimr.statement.Statement;
import yandex.cloud.repository.kikimr.statement.YqlStatement;
import yandex.cloud.repository.kikimr.statement.YqlStatementParam;
import yandex.cloud.repository.kikimr.yql.YqlListingQuery;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;

import static java.lang.String.format;
import static java.util.Collections.singleton;
import static java.util.Collections.singletonMap;
import static java.util.stream.Collectors.joining;
import static java.util.stream.Collectors.toUnmodifiableSet;

public class YqlStatementCi {

    private YqlStatementCi() {
        //
    }

    static <PARAMS, ENTITY extends Entity<ENTITY>, RESULT> Statement<PARAMS, RESULT> findAll(
            EntitySchema<ENTITY> schema,
            Schema<RESULT> resultSchema,
            Collection<? extends Entity.Id<ENTITY>> keys,
            @Nullable OrderExpression<ENTITY> orderBy) {
        return new SimpleFindInStatement<>(schema, resultSchema, keys, orderBy);
    }


    static <T extends Entity<T>, V extends View> Statement<Collection<? extends YqlStatementPart<?>>, V> groupBy(
            EntitySchema<T> schema,
            ViewSchema<V> view,
            String selectColumns,
            String groupByColumns,
            Collection<YqlStatementPart<?>> parts) {
        return new PredicateStatement<>(schema, view, parts, YqlStatementCi::predicateFrom) {
            @Override
            public String getQuery(String tablespace, YqlVersion version) {

                return declarations(version)
                        + "SELECT " + selectColumns + ", COUNT(*) AS count"
                        + " FROM " + table(tablespace)
                        + " " +
                        mergeParts(parts.stream())
                                .sorted(Comparator.comparing(YqlStatementPart::getPriority))
                                .map(sp -> sp.toFullYql(schema))
                                .map(this::resolveParamNames)
                                .collect(Collectors.joining(" "));
            }

            @Override
            public QueryType getQueryType() {
                return QueryType.SELECT;
            }

            @Override
            public String toDebugString(Collection<? extends YqlStatementPart<?>> yqlStatementParts) {
                return "select (" + selectColumns + ", COUNT(*) AS count) group by (" +
                        groupByColumns + ", " + parts + ")";
            }
        };
    }

    static <T extends Entity<T>, V extends View> Statement<Collection<? extends YqlStatementPart<?>>, V> aggregate(
            EntitySchema<T> schema,
            ViewSchema<V> view,
            String selectColumns,
            Collection<YqlStatementPart<?>> parts) {
        return new PredicateStatement<>(schema, view, parts, YqlStatementCi::predicateFrom) {
            @Override
            public String getQuery(String tablespace, YqlVersion version) {

                return declarations(version)
                        + "SELECT " + selectColumns
                        + " FROM " + table(tablespace)
                        + " " +
                        mergeParts(parts.stream())
                                .sorted(Comparator.comparing(YqlStatementPart::getPriority))
                                .map(sp -> sp.toFullYql(schema))
                                .map(this::resolveParamNames)
                                .collect(Collectors.joining(" "));
            }

            @Override
            public QueryType getQueryType() {
                return QueryType.SELECT;
            }

            @Override
            public String toDebugString(Collection<? extends YqlStatementPart<?>> yqlStatementParts) {
                return "select (" + selectColumns + ") aggregate (" + parts + ")";
            }
        };
    }

    static <T extends Entity<T>> Statement<Collection<? extends YqlStatementPart<?>>, Count> countDistinct(
            EntitySchema<T> schema,
            String distinctColumn,
            Collection<YqlStatementPart<?>> parts) {
        return new PredicateStatement<>(schema, ObjectSchema.of(Count.class), parts, YqlStatementCi::predicateFrom) {
            @Override
            public String getQuery(String tablespace, YqlVersion version) {
                return declarations(version)
                        + "SELECT COUNT(distinct " + distinctColumn + ") AS count"
                        + " FROM " + table(tablespace)
                        + " " +
                        mergeParts(parts.stream())
                                .sorted(Comparator.comparing(YqlStatementPart::getPriority))
                                .map(sp -> sp.toFullYql(schema))
                                .map(this::resolveParamNames)
                                .collect(Collectors.joining(" "));
            }

            @Override
            public QueryType getQueryType() {
                return QueryType.SELECT;
            }

            @Override
            public String toDebugString(Collection<? extends YqlStatementPart<?>> yqlStatementParts) {
                return "count(" + parts + ")";
            }
        };
    }

    public static YqlPredicate predicateFrom(Collection<? extends YqlStatementPart<?>> parts) {
        return parts.stream()
                .filter(p -> p instanceof YqlPredicate)
                .map(YqlPredicate.class::cast)
                .reduce(YqlPredicate.alwaysTrue(), (p1, p2) -> p1.and(p2));
    }

    static class TopInGroups<T extends Entity<T>, RESULT>
            extends PredicateStatement<Collection<? extends YqlStatementPart<?>>, T, RESULT> {

        protected final @Nonnull
        Collection<? extends YqlStatementPart<?>> parts;
        private final List<String> partitionsBy;
        private final YqlOrderBy orderBy;
        private final int groupLimit;

        protected TopInGroups(@Nonnull EntitySchema<T> schema,
                              @Nonnull Schema<RESULT> outSchema,
                              @Nonnull Collection<? extends YqlStatementPart<?>> parts,
                              List<String> partitionsBy,
                              YqlOrderBy orderBy,
                              int groupLimit
        ) {
            super(schema, outSchema, parts, YqlStatement::predicateFrom);

            this.parts = parts;
            this.partitionsBy = partitionsBy;
            this.orderBy = orderBy;
            this.groupLimit = groupLimit;
        }

        @Override
        public String getQuery(String tablespace, YqlVersion version) {
            String declarations = this.declarations(version);

            Predicate<YqlStatementPart<?>> isOrder = p -> p.getType().equals("OrderBy");
            var globalOrder = parts.stream()
                    .filter(isOrder)
                    .findFirst()
                    .map(o -> o.toFullYql(schema))
                    .orElse("");

            var queryParts = mergeParts(parts.stream().filter(isOrder.negate()))
                    .sorted(Comparator.comparing(YqlStatementPart::getPriority))
                    .map((sp) -> sp.toFullYql(this.schema))
                    .map(this::resolveParamNames).collect(joining(" "));

            var partitionBy = partitionsBy.stream()
                    .map(f -> schema.findField(f).get().getName())
                    .collect(joining(", "));

            var outNames = this.outNames();

            return declarations + "SELECT " + outNames + " FROM" +
                    " (SELECT " + outNames + ", ROW_NUMBER() OVER w AS rank" +
                    "   FROM " + this.table(tablespace) + " " + queryParts +
                    "   WINDOW w AS (PARTITION BY " + partitionBy + " " + this.orderBy.toFullYql(schema) + ")" +
                    " ) WHERE rank <= " + groupLimit + " " + globalOrder;
        }

        @Override
        public String toDebugString(Collection<? extends YqlStatementPart<?>> yqlStatementParts) {
            return "topInGroups(%s) BY %s ORDER BY %s GROUP LIMIT %d".formatted(
                    yqlStatementParts, partitionsBy, orderBy.getKeys(), groupLimit);
        }

        @Override
        public QueryType getQueryType() {
            return QueryType.SELECT;
        }
    }

    static class SimpleFindInStatement<IN, T extends Entity<T>, RESULT> extends FindInStatement<IN, T, RESULT> {

        @Nullable
        private final OrderExpression<T> orderBy;

        protected SimpleFindInStatement(
                EntitySchema<T> schema,
                Schema<RESULT> resultSchema,
                Iterable<? extends Entity.Id<T>> ids,
                @Nullable OrderExpression<T> orderBy) {
            super(schema, resultSchema, ids);
            this.orderBy = orderBy;
            this.validateOrderByFields();
        }

        @Override
        protected String declarations(YqlVersion version) {
            String tuple = getParams().stream()
                    .map(p -> String.format("%s%s", p.getType().getYqlTypeName(), p.isOptional() ? "?" : ""))
                    .collect(joining(","));
            return version.getDeclaration(listName, "List<Tuple<" + tuple + ">>");
        }

        @Override
        public String getQuery(String tablespace, YqlVersion version) {
            var params = getParams();
            var keys = getParams().stream()
                    .map(param -> escape(param.getName()))
                    .collect(Collectors.joining(",", "", params.size() == 1 ? "," : ""));

            var columns = this.resultSchema.flattenFields().stream()
                    .map(Schema.JavaField::getName)
                    .map(this::escape)
                    .collect(Collectors.joining(", "));

            var orderBySql = orderBy != null
                    ? YqlListingQuery.toYqlOrderBy(orderBy).toFullYql(schema)
                    : "";

            return "PRAGMA AnsiInForEmptyOrNullableItemsCollections;\n" +
                    declarations(version) +
                    "SELECT " + columns + "\n" +
                    "FROM " + table(tablespace) + "\n" +
                    "WHERE (" + keys + ") in " + listName + "\n" +
                    orderBySql;
        }

        @Override
        @SuppressWarnings("unchecked")
        public Map<String, ValueProtos.TypedValue> toQueryParameters(IN params) {
            List<YqlStatementParam> yqlParams = getParams();

            var tupleTypeBuilder = ValueProtos.TupleType.newBuilder();
            yqlParams.forEach(param -> tupleTypeBuilder.addElements(getYqlType(param.getType(), param.isOptional())));

            var keys = (params instanceof Collection ? ((Collection<IN>) params) : singleton(params));
            ValueProtos.Value.Builder tupleBuilder = keys.stream()
                    .map(flattenInputVariables())
                    .map(fieldValues -> yqlParams.stream()
                            .map(p -> getYqlValue(p.getType(), fieldValues.get(p.getName())))
                            .collect(itemsCollector))
                    .collect(itemsCollector);
            return singletonMap(
                    listName,
                    ValueProtos.TypedValue.newBuilder()
                            .setType(ValueProtos.Type.newBuilder()
                                    .setListType(ValueProtos.ListType.newBuilder()
                                            .setItem(ValueProtos.Type.newBuilder().setTupleType(tupleTypeBuilder))
                                            .build())
                                    .build())
                            .setValue(tupleBuilder)
                            .build()
            );
        }

        @Override
        public String toDebugString(IN in) {
            return "simpleFindIn(" + toDebugParams(in) + " order by " + orderBy + ")";
        }

        private void validateOrderByFields() {
            if (schema.equals(resultSchema)) {
                return;
            }
            if (orderBy == null) {
                return;
            }

            Set<String> resultColumns = resultSchema.flattenFields().stream()
                    .map(Schema.JavaField::getName).collect(toUnmodifiableSet());
            List<String> missingColumns = orderBy.getKeys().stream()
                    .map(OrderExpression.SortKey::getField)
                    .flatMap(Schema.JavaField::flatten)
                    .map(Schema.JavaField::getName)
                    .filter(column -> !resultColumns.contains(column))
                    .toList();

            if (!missingColumns.isEmpty()) {
                throw new IllegalArgumentException(
                        format("Result schema of '%s' does not contain field(s): " +
                                        "[%s] by which the result is ordered: %s",
                                resultSchema.getType().getSimpleName(), String.join(", ", missingColumns), orderBy));
            }
        }
    }
}
