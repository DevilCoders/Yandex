package ru.yandex.ci.storage.core.db.model.storage_statistics;

import java.util.List;
import java.util.stream.Collectors;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class StorageStatisticsTable extends KikimrTableCi<StorageStatisticsEntity> {
    public StorageStatisticsTable(QueryExecutor executor) {
        super(StorageStatisticsEntity.class, executor);
    }

    public List<StorageStatisticsEntity> getAll() {
        return this.readTable().collect(Collectors.toList());

    }
}
