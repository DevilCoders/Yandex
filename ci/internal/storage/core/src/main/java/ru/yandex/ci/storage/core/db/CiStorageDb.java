package ru.yandex.ci.storage.core.db;

import yandex.cloud.repository.db.TxManager;

import ru.yandex.ci.common.temporal.ydb.TemporalDb;
import ru.yandex.ci.common.ydb.TransactionSupportDefault;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageDb;

public interface CiStorageDb extends TxManager, CiStorageDbTables, BazingaStorageDb, TemporalDb,
        TransactionSupportDefault {
}

