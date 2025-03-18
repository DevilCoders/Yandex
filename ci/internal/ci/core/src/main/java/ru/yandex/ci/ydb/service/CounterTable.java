package ru.yandex.ci.ydb.service;

import java.util.List;
import java.util.Optional;
import java.util.stream.LongStream;

import com.google.common.base.Preconditions;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class CounterTable extends KikimrTableCi<CounterEntity> {


    public CounterTable(QueryExecutor queryExecutor) {
        super(CounterEntity.class, queryExecutor);
    }

    public Optional<Long> get(String key) {
        return get(CounterEntity.DEFAULT_NAMESPACE, key);
    }

    public Optional<Long> get(String namespace, String key) {
        return find(CounterEntity.Id.of(namespace, key)).map(CounterEntity::getValue);
    }

    public long incrementAndGet(String key) {
        return incrementAndGet(CounterEntity.DEFAULT_NAMESPACE, key);
    }

    public long incrementAndGet(String namespace, String key) {
        var id = CounterEntity.Id.of(namespace, key);
        var newValue = find(id)
                .orElseGet(() -> CounterEntity.of(id, 0L))
                .increment();
        save(newValue);
        return newValue.getValue();
    }

    public List<Long> incrementAndGet(String key, int amount) {
        return incrementAndGet(CounterEntity.DEFAULT_NAMESPACE, key, amount);
    }

    public List<Long> incrementAndGet(String namespace, String key, int amount) {
        var id = CounterEntity.Id.of(namespace, key);
        var entity = find(id).orElseGet(() -> CounterEntity.of(id, 0L));
        save(entity.increment(amount));
        return LongStream.range(entity.getValue() + 1, entity.getValue() + amount + 1).boxed().toList();
    }

    public long incrementAndGetWithLowLimit(String namespace, String key, long limit) {
        Preconditions.checkArgument(limit > 0);
        var id = CounterEntity.Id.of(namespace, key);
        var newValue = find(id)
                .map(CounterEntity::increment)
                .map(c -> c.withLowerBound(limit))
                .orElseGet(() -> CounterEntity.of(id, limit));

        save(newValue);
        return newValue.getValue();
    }

    public void init(String key, long start) {
        var id = CounterEntity.Id.of(CounterEntity.DEFAULT_NAMESPACE, key);
        var counter = find(id);
        if (counter.isEmpty()) {
            save(CounterEntity.of(id, start));
        }
    }
}
