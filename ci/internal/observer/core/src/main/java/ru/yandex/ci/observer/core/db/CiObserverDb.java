package ru.yandex.ci.observer.core.db;

import yandex.cloud.repository.db.TxManager;

import ru.yandex.ci.common.ydb.TransactionSupportDefault;

public interface CiObserverDb extends TxManager, CiObserverDbTables, TransactionSupportDefault {
}

