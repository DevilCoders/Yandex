package ru.yandex.ci.common.ydb;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.Lists;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import yandex.cloud.binding.expression.FilterExpression;
import yandex.cloud.binding.expression.OrderExpression;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.db.Tx;
import yandex.cloud.repository.db.ViewSchema;
import yandex.cloud.repository.db.bulk.BulkParams;
import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.db.statement.Changeset;
import yandex.cloud.repository.kikimr.statement.Statement;
import yandex.cloud.repository.kikimr.statement.YqlStatement;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import ru.yandex.ci.util.FieldAccessor;
import ru.yandex.ci.util.Retryable;

import static java.util.Collections.emptyList;

@Slf4j
public class KikimrTableCi<T extends Entity<T>> {

    static {
        PrometheusMetricsFilter.disableTxMetrics();
    }

    private static final FieldAccessor<YqlLimit> YQL_LIMIT_OFFSET = FieldAccessor.of(YqlLimit.class, "offset");

    protected final QueryExecutor executor;
    private final KikimrTableDelegate<T> delegate;
    private final boolean updateProjection;

    public KikimrTableCi(Class<T> type, QueryExecutor executor) {
        this.delegate = new KikimrTableDelegate<>(type, executor);
        this.executor = executor;
        this.updateProjection = KikimrProjectionCI.class.isAssignableFrom(type);
    }

    public String getTableName() {
        return EntitySchema.of(getType()).getName();
    }

    public Class<T> getType() {
        return delegate.getType();
    }

    public <Params> List<T> execute(Statement<Params, T> statement, @Nullable Params params) {
        return delegate.postLoad(executor.execute(statement, params));
    }

    public <PARAMS, RESULT> List<RESULT> executeOnView(Statement<PARAMS, RESULT> statement, @Nullable PARAMS params) {
        return executor.execute(statement, params);
    }

    //

    /**
     * Check if record exists in database by primary key
     *
     * @param id primary key
     * @return true if record exists
     */
    public boolean exists(Entity.Id<T> id) {
        return findInternal(id) != null;
    }

    /**
     * Try to find a record in database by primary key
     *
     * @param id primary key
     * @return optional value
     */
    public Optional<T> find(Entity.Id<T> id) {
        return Optional.ofNullable(findInternal(id));
    }

    /**
     * Get a record from database by primary key or raise {@link NoSuchElementException}
     *
     * @param id primary key
     * @return record
     */
    public T get(Entity.Id<T> id) {
        var result = findInternal(id);
        if (result == null) {
            throw new NoSuchElementException("Unable to find key [" + id + "] in table [" + getTableName() + "]");
        } else {
            return result;
        }
    }

    public Stream<T> readTable() {
        return readTable(ReadTableParams.getDefault());
    }

    public <ID extends Entity.Id<T>> Stream<ID> readTableIds() {
        return readTableIds(ReadTableParams.getDefault());
    }

    public <ID extends Entity.Id<T>> Stream<T> readTable(ReadTableParams<ID> params) {
        return delegate.readTable(params);
    }


    public <ID extends Entity.Id<T>> Stream<ID> readTableIds(ReadTableParams<ID> params) {
        return delegate.readTableIds(params);
    }

    @Nullable
    protected T findInternal(Entity.Id<T> id) {
        return delegate.find(id);
    }

    @SuppressWarnings("unchecked")
    public <ID extends Entity.Id<T>> List<T> find(Set<ID> ids) {
        return splitBySet("find", ids, delegate::find);
    }

    public <V extends View> List<V> find(
            Class<V> viewType,
            YqlStatementPart<?> part,
            YqlStatementPart<?>... otherParts) {
        return find(viewType, toList(part, otherParts));
    }

    public <V extends View> List<V> find(
            Class<V> viewType,
            Collection<? extends YqlStatementPart<?>> parts) {
        return splitByFetch("find for " + viewType, parts, list -> delegate.find(viewType, list));
    }

    public final List<T> find(YqlStatementPart<?> part, YqlStatementPart<?>... otherParts) {
        return find(toList(part, otherParts));
    }

    public List<T> find(Collection<? extends YqlStatementPart<?>> parts) {
        return splitByFetch("find", parts, delegate::find, Entity::getId);
    }

    public <V extends View> List<V> findDistinct(Class<V> viewType, Collection<? extends YqlStatementPart<?>> parts) {
        return splitByFetch("findDistinct", parts, list -> delegate.findDistinct(viewType, list));
    }

    public <ID extends Entity.Id<T>> List<ID> findIds(Set<ID> ids) {
        return splitBySet("findIds", ids, delegate::findIds);
    }

    public <ID extends Entity.Id<T>> List<ID> findIds(YqlStatementPart<?> part, YqlStatementPart<?>... otherParts) {
        return findIds(toList(part, otherParts));
    }

    public <ID extends Entity.Id<T>> List<ID> findIds(Collection<? extends YqlStatementPart<?>> parts) {
        return splitByFetch("findIds", parts, list -> executor.execute(YqlStatement.findIds(getType(), list), list));
    }

    public <V extends View, ID extends Entity.Id<T>> List<V> find(Class<V> viewType, Set<ID> ids) {
        return splitBySet("findIds by " + viewType, ids, set -> delegate.find(viewType, set));
    }

    public <V extends View, ID extends Entity.Id<T>> Optional<V> find(Class<V> viewType, ID id) {
        return Optional.ofNullable(delegate.find(viewType, id));
    }

    public <V extends View, ID extends Entity.Id<T>> V get(Class<V> viewType, ID id) {
        var result = delegate.find(viewType, id);
        if (result == null) {
            throw new NoSuchElementException("Unable to find key [" + id + "] in table [" + getTableName() + "]");
        }
        return result;
    }

    public List<T> findAll() {
        return delegate.findAll();
    }

    public Stream<T> streamAll(int batchSize) {
        return delegate.streamAll(batchSize);
    }

    public long countAll() {
        return delegate.countAll();
    }

    public T save(T t) {
        if (updateProjection) {
            findInternal(t.getId()); // Update for projection
        }
        return delegate.save(t);
    }

    public void save(Collection<? extends T> entities) {
        for (T entity : entities) {
            save(entity);
        }
    }

    @SafeVarargs
    public final void save(T first, T... rest) {
        save(first);
        for (var value : rest) {
            save(value);
        }
    }

    public void delete(Entity.Id<T> id) {
        delegate.delete(id);
    }

    public <ID extends Entity.Id<T>> void delete(Set<ID> ids) {
        for (ID id : ids) {
            delete(id);
        }
    }

    public void deleteAll() {
        delegate.deleteAll();
    }

    public void bulkUpsert(List<T> input, BulkParams params) {
        delegate.bulkUpsert(input, params);
    }

    public void bulkUpsert(List<T> input, int batchLimit) {
        if (input.isEmpty()) {
            return;
        }

        for (var batch : Lists.partition(input, batchLimit)) {
            bulkUpsert(batch, BulkParams.DEFAULT);
        }
    }

    public void bulkUpsertWithRetries(List<T> input, int batchLimit, Consumer<Throwable> onRetry) {
        if (input.isEmpty()) {
            return;
        }

        for (var batch : Lists.partition(input, batchLimit)) {
            Retryable.retryUntilInterruptedOrSucceeded(
                    () -> delegate.bulkUpsert(batch, BulkParams.DEFAULT), onRetry, true, 1, 2
            );
        }
    }

    public void update(Entity.Id<T> id, Changeset changeset) {
        delegate.update(id, changeset);
    }

    public <ID extends Entity.Id<T>> void updateIn(Collection<ID> ids, Changeset changeset) {
        delegate.updateIn(ids, changeset);
    }

    // Does not support #splitByFetch!
    public <VIEW extends View> List<VIEW> topInGroups(
            Class<VIEW> viewType,
            List<String> partitionBy,
            YqlOrderBy orderBy,
            Collection<? extends YqlStatementPart<?>> queryParts,
            int groupLimit
    ) {
        var statement = new YqlStatementCi.TopInGroups<>(
                EntitySchema.of(getType()),
                ViewSchema.of(viewType),
                queryParts,
                partitionBy,
                orderBy,
                groupLimit
        );
        return this.executor.execute(statement, queryParts);
    }

    public long count(List<YqlStatementPart<?>> partsList) {
        return executor.execute(YqlStatement.count(getType(), false, partsList), partsList).get(0).getCount();
    }

    public long count(YqlStatementPart<?>... parts) {
        return delegate.count(parts);
    }

    /**
     * Поиск количества уникальных значений столбца
     *
     * @param field     поле, по которому нужно сделать `count(distinct)`
     * @param partsList список параметров
     * @return количество найденных строк
     */
    public long countDistinct(String field, List<YqlStatementPart<?>> partsList) {
        var statement = YqlStatementCi.countDistinct(EntitySchema.of(getType()), field, partsList);
        return this.executor.execute(statement, partsList).get(0).getCount();
    }

    /**
     * Поиск с учетом группировки (по переданному набору строк) и выводом количества найденных строк в поле `count`
     *
     * @param view      view класс
     * @param columns   список полей, которые нужно передать в конструкцию `group by`
     * @param partsList список параметров
     * @param <V>       тип view
     * @return список строк
     */
    public <V extends View> List<V> groupBy(Class<V> view, List<String> columns, List<YqlStatementPart<?>> partsList) {
        return groupBy(view, columns, columns, partsList);
    }

    /**
     * Поиск с учетом группировки (по переданному набору строк) и выводом количества найденных строк в поле `count`
     *
     * @param view          view класс
     * @param selectColumns список полей, которые будут подставлены в `select`
     * @param groupColumns  список полей, которые нужно передать в конструкцию `group by`
     * @param partsList     список параметров
     * @param <V>           тип view
     * @return список строк
     */
    public <V extends View> List<V> groupBy(
            Class<V> view,
            List<String> selectColumns,
            List<String> groupColumns,
            List<YqlStatementPart<?>> partsList) {

        var joinedGroupColumns = String.join(", ", groupColumns);
        var joinedSelectColumns = String.join(", ", selectColumns);

        var allParts = new ArrayList<>(partsList);
        allParts.add(new YqlRawGroupBy(joinedGroupColumns));

        return splitByFetch("groupBy", allParts,
                parts -> {
                    var statement = YqlStatementCi.groupBy(EntitySchema.of(getType()), ViewSchema.of(view),
                            joinedSelectColumns, joinedGroupColumns, allParts);
                    return this.executor.execute(statement, allParts);
                });
    }

    /**
     * Агрегация найденных строк по заданым функциям в списке полей
     *
     * @param view          view класс для агрегированных данных
     * @param selectColumns список полей, которые будут подставлены в `select`
     * @param partsList     список параметров
     * @param <V>           тип view
     * @return Строка с агрегированной информацией
     */
    public <V extends View> Optional<V> aggregate(
            Class<V> view,
            List<String> selectColumns,
            List<YqlStatementPart<?>> partsList) {
        var joinedSelectColumns = String.join(", ", selectColumns);

        var statement = YqlStatementCi.aggregate(
                EntitySchema.of(getType()), ViewSchema.of(view), joinedSelectColumns, partsList
        );

        return this.executor.execute(statement, partsList).stream().findFirst();
    }

    //

    protected static List<YqlStatementPart<?>> filter(long limit, YqlStatementPart<?>... statements) {
        var parts = filter(limit);
        parts.addAll(List.of(statements));
        return parts;
    }

    protected static List<YqlStatementPart<?>> filter(long limit) {
        return filter(limit, 0);
    }

    protected static List<YqlStatementPart<?>> filter(long limit, long offset) {
        var filter = limit(limit, offset);
        if (!filter.isEmpty()) {
            return filter(filter);
        } else {
            return filter();
        }
    }

    protected static List<YqlStatementPart<?>> filter(YqlStatementPart<?>... statements) {
        var result = new ArrayList<YqlStatementPart<?>>();
        if (statements.length > 0) {
            result.addAll(List.of(statements));
        }
        return result;
    }

    private static YqlLimit limit(long limit, long offset) {
        Preconditions.checkArgument(limit <= YdbUtils.RESULT_ROW_LIMIT,
                "YDB Limit %s must be <= %s", limit, YdbUtils.RESULT_ROW_LIMIT);
        if (limit > 0 && offset > 0) {
            return YqlLimit.top(limit).withOffset(offset);
        } else if (limit > 0) {
            return YqlLimit.top(limit);
        } else if (offset > 0) {
            return YqlLimit.empty().withOffset(offset);
        } else {
            return YqlLimit.empty();
        }
    }

    /**
     * Make sure the list contains 0 or 1 elements, raise {@link IllegalStateException} otherwise
     *
     * @param values list of values
     * @return first element of array or empty; raise {@link IllegalStateException} if array has 2 or more elements
     */
    protected static <V> Optional<V> single(List<V> values) {
        Preconditions.checkState(
                values.size() <= 1,
                "Expected single or empty result, fetched %s",
                values.size()
        );
        return values.isEmpty()
                ? Optional.empty()
                : Optional.of(values.get(0));
    }

    private <K, V> List<V> splitBySet(String method, Set<K> set, Function<Set<K>, List<V>> action) {
        if (set.size() <= YdbUtils.RESULT_ROW_LIMIT || isScan()) {
            return action.apply(set);
        } else {
            var result = new ArrayList<V>(YdbUtils.RESULT_ROW_LIMIT);
            var tmp = new HashSet<K>(YdbUtils.RESULT_ROW_LIMIT);
            for (K next : set) {
                tmp.add(next);
                if (tmp.size() == YdbUtils.RESULT_ROW_LIMIT) {
                    result.addAll(action.apply(tmp));
                    tmp.clear();
                }
            }
            if (!tmp.isEmpty()) {
                result.addAll(action.apply(tmp));
            }
            log.info("{}, #{}, split by set {} -> {}", getTableName(), method, set.size(), result.size());
            return result;
        }
    }

    private <V> List<V> splitByFetch(
            String method,
            Collection<? extends YqlStatementPart<?>> parts,
            Function<Collection<? extends YqlStatementPart<?>>, List<V>> action) {
        return splitByFetch(method, parts, action, Function.identity());
    }

    /**
     * <p>
     * Поиск и извлечение более 1000 строк в случае, когда <b>нельзя</b> использовать scan query;
     * например при использовании вторичных индексов.
     * </p>
     *
     * <p>
     * Работает не очень эффективно, т.к. использует offset для постраничной выборки.
     * </p>
     *
     * @param parts список фильтров
     * @return список (потенциально состоящий из более чем 1000 строк)
     */
    private <K, V> List<V> splitByFetch(
            String method,
            Collection<? extends YqlStatementPart<?>> parts,
            Function<Collection<? extends YqlStatementPart<?>>, List<V>> action,
            Function<V, K> distinctFunction) {
        if (isScan()) {
            return action.apply(parts);
        }

        boolean hasOrderBy = false;

        long offset = 0;
        long selectLimit = YdbUtils.SELECT_LIMIT;

        List<YqlStatementPart<?>> actualParts = new ArrayList<>(parts.size());
        for (var part : parts) {
            if (part instanceof YqlLimit limit) {
                if (limit.size() > 0) {
                    if (limit.size() <= YdbUtils.RESULT_ROW_LIMIT) {
                        return action.apply(parts);
                    }
                    selectLimit = limit.size();
                }
                try {
                    offset = (int) (long) YQL_LIMIT_OFFSET.handler(limit).invoke();
                } catch (Throwable e) {
                    throw new RuntimeException("Unable to access field", e);
                }
            } else {
                if (YqlOrderBy.TYPE.equals(part.getType())) {
                    hasOrderBy = true;
                }
                actualParts.add(part);
            }
        }

        Preconditions.checkState(selectLimit <= YdbUtils.SELECT_LIMIT,
                "Select limit cannot exceed %s. If you need more rows, use scan() or readTable()",
                YdbUtils.SELECT_LIMIT);

        var orderBy = hasOrderBy
                ? filter()
                : filter(YqlOrderBy.orderBy("id"));

        var allRows = new ArrayList<V>();
        var lastRows = List.<V>of();
        do {
            // ORM не поддерживает условие вида 'gt <сложный ключ>', хотя YDB с этим работает без проблем
            var limit = selectLimit < YdbUtils.SELECT_LIMIT
                    ? Math.min(YdbUtils.RESULT_ROW_LIMIT, Math.max(0, selectLimit - allRows.size()))
                    : YdbUtils.RESULT_ROW_LIMIT;
            if (limit == 0) {
                break; // nothing to fetch
            }

            var filter = filter(limit, offset);
            filter.addAll(actualParts);
            filter.addAll(orderBy);

            lastRows = action.apply(filter);

            allRows.addAll(lastRows);
            offset += lastRows.size();

            Preconditions.checkState(allRows.size() <= selectLimit,
                    "Fetched too much rows: %s. If you need more rows, use scan() or readTable()", offset);
        } while (lastRows.size() == YdbUtils.RESULT_ROW_LIMIT);

        if (allRows.size() < YdbUtils.RESULT_ROW_LIMIT) {
            return allRows;
        } else {
            // Чтение просто по offset-у может привести к неприятным последствиям типа наличия дубликатов
            // Пытаемся как-то от этого защититься
            var result = StreamEx.of(allRows).distinct(distinctFunction).toList();
            log.info("{}, #{}, split fetch {} -> {}", getTableName(), method, allRows.size(), result.size());
            return result;
        }
    }

    private static boolean isScan() {
        var tx = Tx.Current.get().getRepositoryTransaction();
        if (tx instanceof KikimrRepositoryCi.KikimrRepositoryTransactionCi<?> txCi) {
            return txCi.getOptions().isScan();
        } else {
            return false;
        }
    }

    @SafeVarargs
    private static <E> List<E> toList(E first, E... rest) {
        var result = new ArrayList<E>(rest.length + 1);
        result.add(first);
        Collections.addAll(result, rest);
        return result;
    }

    //

    private static class KikimrTableDelegate<T extends Entity<T>> extends KikimrTable<T> {

        private final QueryExecutor executor;

        KikimrTableDelegate(Class<T> type, QueryExecutor executor) {
            super(type, executor);
            this.executor = executor;
        }

        @Override
        public List<T> postLoad(List<T> list) {
            return super.postLoad(list);
        }

        @Override
        protected <V extends View, ID extends Entity.Id<T>> List<V> find(
                Class<V> viewType,
                Set<ID> ids,
                @Nullable FilterExpression<T> filter,
                OrderExpression<T> orderBy) {
            if (ids.isEmpty()) {
                return emptyList();
            }
            if (filter != null) {
                return super.find(viewType, ids, filter, orderBy);
            }
            // Выполняет простую операцию `in` вместо join-а по таблице
            var statement = YqlStatementCi.findAll(EntitySchema.of(getType()), ViewSchema.of(viewType), ids, orderBy);
            return this.executor.execute(statement, ids);
        }

        @Override
        public <ID extends Entity.Id<T>> List<T> findUncached(
                Set<ID> ids,
                @Nullable FilterExpression<T> filter,
                OrderExpression<T> orderBy
        ) {
            if (ids.isEmpty()) {
                return emptyList();
            }
            if (filter != null) {
                return super.findUncached(ids, filter, orderBy);
            }
            // Выполняет простую операцию `in` вместо join-а по таблице
            var statement = YqlStatementCi.findAll(EntitySchema.of(getType()), ViewSchema.of(getType()), ids, orderBy);
            return this.executor.execute(statement, ids);
        }
    }

}
